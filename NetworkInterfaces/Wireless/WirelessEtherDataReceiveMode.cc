// -*- C++ -*-
// Copyright (C) 2001 Monash University, Melbourne, Australia
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

/*
  @file WirelessEtherDataReceiveMode.cc
  @brief Source file for WEDataReceiveMode

  @author    Steve Woon
  Eric Wu
*/

#include "sys.h"
#include "debug.h"

#include <iostream>
#include <iomanip>
#include <omnetpp.h>

#include "WirelessEtherDataReceiveMode.h"
#include "WirelessEtherState.h"
#include "WirelessEtherModule.h"
#include "WirelessEtherSignal_m.h"
#include "WirelessEtherFrame_m.h"
#include "WirelessEtherFrameBody_m.h"

#include "WirelessEtherStateIdle.h"
#include "WirelessEtherStateSend.h"
#include "WirelessEtherStateBackoff.h"
#include "WirelessEtherStateReceive.h"
#include "WirelessEtherStateAwaitACKReceive.h"
#include "WirelessEtherStateBackoffReceive.h"
#include "WirelessEtherStateBackoff.h"

#include "WirelessEtherAScanReceiveMode.h"
#include "WirelessEtherAuthenticationReceiveMode.h"
#include "WirelessEtherAssociationReceiveMode.h" //noauth requires this
#include "cTimerMessage.h" //link up trigger
#include "TimerConstants.h" // SELF_SCHEDULE_DELAY
#include "opp_utils.h"
#include "NotificationBoard.h" //for rtp handover l2 up signal
#include "NotifierConsts.h"

void sendLinkDownMsg(cSimpleModule* mod);

WEDataReceiveMode *WEDataReceiveMode::_instance = 0;

WEDataReceiveMode *WEDataReceiveMode::instance()
{
    if (_instance == 0)
        _instance = new WEDataReceiveMode;

    return _instance;
}

void WEDataReceiveMode::handleAssociationResponse(WirelessEtherModule *mod, WESignalData *signal)
{
    WirelessEtherManagementFrame *associationResponse =
        static_cast<WirelessEtherManagementFrame *>(signal->encapsulatedMsg());

    if (associationResponse->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
    {
        AssociationResponseFrameBody *aRFrameBody =
            static_cast<AssociationResponseFrameBody *>(associationResponse->decapsulate());

        wEV  << mod->fullPath() << "\n"
             << " ----------------------------------------------- \n"
             << " Association Response received by: "
             << mod->macAddressString() << "\n"
             << " from " << associationResponse->getAddress2() << " \n"
             << " Status Code: " << aRFrameBody->getStatusCode() << "\n"
             << " ----------------------------------------------- \n";
        for (size_t i = 0; i < aRFrameBody->getSupportedRatesArraySize(); i++)
            wEV  << " Supported Rates: \n "
                 << " [" << i << "] \t rate: "
                 << aRFrameBody->getSupportedRates(i).rate << " \n"
                 << " \t supported?"
                 << aRFrameBody->getSupportedRates(i).supported
                 << "\n";

        // send ACK to confirm the transmission has been sucessful
        WirelessEtherBasicFrame *ack = mod->createFrame(FT_CONTROL, ST_ACK,
                                                        MACAddress6(mod->macAddressString().c_str()), AC_VO,
                                                        associationResponse->getAddress2());
        scheduleAck(mod, ack);
        // delete ack;
        changeState = false;

        if (mod->associateAP.address == associationResponse->getAddress2())
        {
            mod->associateAP.address = associationResponse->getAddress2();
            mod->associateAP.channel = signal->channelNum();
            mod->associateAP.rxpower = signal->power();
            mod->associateAP.associated = true;
            // already in data receive mode so dont need to switch to it

            // Stop the association timeout timer
            if (mod->assTimeoutNotifier->isScheduled())
                mod->cancelEvent(mod->assTimeoutNotifier);

            // Send upper layer signal of success
            mod->sendSuccessSignal();

            if ( mod->linkUpTrigger() )
            {
              if (mod->getLayer2Trigger())
	      {
              assert ( !mod->getLayer2Trigger()->isScheduled() );
              mod->getLayer2Trigger()->reschedule(mod->simTime() + SELF_SCHEDULE_DELAY);
              Dout(dc::mipv6 | dc::mobile_move,
                   mod->fullPath() << " Link-Up trigger signalled " << (mod->simTime() + SELF_SCHEDULE_DELAY));
	      mod->nb->fireChangeNotification(NF_L2_ASSOCIATED);
              }
            }
        }
        // TODO: need to check supported rates and status code
        delete aRFrameBody;
    }
}

void WEDataReceiveMode::handleReAssociationResponse(WirelessEtherModule *mod, WESignalData *signal)
{
    WirelessEtherManagementFrame *reAssociationResponse =
        static_cast<WirelessEtherManagementFrame *>(signal->encapsulatedMsg());

    if (reAssociationResponse->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
    {
        ReAssociationResponseFrameBody *rARFrameBody =
            static_cast<ReAssociationResponseFrameBody *>(reAssociationResponse->decapsulate());

        wEV  << mod->fullPath() << "\n"
             << " ----------------------------------------------- \n"
             << " Reassociation Response received by: "
             << mod->macAddressString() << "\n"
             << " from " << reAssociationResponse->getAddress2() << " \n"
             << " Status Code: " << rARFrameBody->getStatusCode() << "\n"
             << " ----------------------------------------------- \n";
        for (size_t i = 0; i < rARFrameBody->getSupportedRatesArraySize(); i++)
            wEV  << " Supported Rates: \n "
                 << " [" << i << "] \t rate: "
                 << rARFrameBody->getSupportedRates(i).rate << " \n"
                 << " \t supported?" << rARFrameBody->getSupportedRates(i).supported << "\n";

        // send ACK to confirm the transmission has been sucessful
        WirelessEtherBasicFrame *ack = mod->createFrame(FT_CONTROL, ST_ACK,
                                                        MACAddress6(mod->macAddressString().c_str()), AC_VO,
                                                        reAssociationResponse->getAddress2());
        scheduleAck(mod, ack);
        // delete ack;
        changeState = false;

        if (mod->associateAP.address == reAssociationResponse->getAddress3())
        {
            mod->associateAP.address = reAssociationResponse->getAddress2();
            mod->associateAP.channel = signal->channelNum();
            mod->associateAP.rxpower = signal->power();
            mod->associateAP.associated = true;
            // already in data receive mode so dont need to switch to it
        }
        // TODO: need to check supported rates and status code
        delete rARFrameBody;
    }
}

void WEDataReceiveMode::handleDeAuthentication(WirelessEtherModule *mod, WESignalData *signal)
{
    WirelessEtherManagementFrame *deAuthentication =
        static_cast<WirelessEtherManagementFrame *>(signal->encapsulatedMsg());

    if (deAuthentication->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
    {
        DeAuthenticationFrameBody *deAuthFrameBody =
            static_cast<DeAuthenticationFrameBody *>(deAuthentication->decapsulate());

        wEV  << mod->fullPath() << "\n"
             << " ----------------------------------------------- (WEDataReceiveMode::handleDeAuthentication)\n"
             << " De-authentication received by: " << mod->macAddressString() << "\n"
             << " from " << deAuthentication->getAddress2() << " \n"
             << " Reason Code: " << deAuthFrameBody->getReasonCode() << "\n"
             << " ----------------------------------------------- \n";

        // TODO: may need to handle reason code as well
        // send ACK to confirm the transmission has been sucessful
        WirelessEtherBasicFrame *ack = mod->createFrame(FT_CONTROL, ST_ACK,
                                                        MACAddress6(mod->macAddressString().c_str()), AC_VO,
                                                        deAuthentication->getAddress2());
        scheduleAck(mod, ack);
        // delete ack;
        changeState = false;

        if (mod->associateAP.address == deAuthentication->getAddress3())
            mod->restartScanning();

        delete deAuthFrameBody;
    }
}

void WEDataReceiveMode::handleDisAssociation(WirelessEtherModule *mod, WESignalData *signal)
{
    WirelessEtherManagementFrame *disAssociation =
        static_cast<WirelessEtherManagementFrame *>(signal->encapsulatedMsg());

    if (disAssociation->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
    {
        DisAssociationFrameBody *disAssFrameBody =
            static_cast<DisAssociationFrameBody *>(disAssociation->decapsulate());

        wEV  << mod->fullPath() << "\n"
             << " ----------------------------------------------- \n"
             << " Dis-association received by: " << mod->macAddressString() << "\n"
             << " from " << disAssociation->getAddress2() << " \n"
             << " Reason Code: " << disAssFrameBody->getReasonCode() << "\n"
             << " ----------------------------------------------- \n";

        // TODO: may need to handle reason code as well
        // send ACK to confirm the transmission has been sucessful
        WirelessEtherBasicFrame *ack = mod->createFrame(FT_CONTROL, ST_ACK,
                                                        MACAddress6(mod->macAddressString().c_str()), AC_VO,
                                                        disAssociation->getAddress2());
        scheduleAck(mod, ack);
        // delete ack;

        if (mod->associateAP.address == disAssociation->getAddress3())
        {
            mod->associateAP.associated = false;
            if (!mod->noAuthentication())
              mod->changeReceiveMode(WEAuthenticationReceiveMode::instance());
            else
            {
              mod->associateAP.channel = signal->channelNum();
              mod->associateAP.rxpower = signal->power();
              mod->changeReceiveMode(WEAssociationReceiveMode::instance());
            }
            changeState = false;
            // may need to initiate association again or re-scan
        }

        delete disAssFrameBody;
    }
}

void WEDataReceiveMode::handleAck(WirelessEtherModule *mod, WESignalData *signal)
{
    WirelessEtherBasicFrame *ack = static_cast<WirelessEtherBasicFrame *>(signal->encapsulatedMsg());

    if (ack->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
    {
        // Since both interfaces have the same MAC Address in a dual interface node, you may
        // process an ACK which is meant for the other interface. Condition checks that you
        // are expecting an ACK before processing it.
        if (mod->currentState() == WirelessEtherStateAwaitACKReceive::instance())
        {
            mod->cancelEvent(mod->awaitAckTimer);

            finishFrameTx(mod);
            wEV  << mod->fullPath() << "\n"
                 << " ----------------------------------------------- \n"
                 << " ACK received by: " << mod->macAddressString() << "\n"
                 << " ----------------------------------------------- \n";

            changeState = false;
        }
    }

    if (mod->currentState() == WirelessEtherStateAwaitACKReceive::instance())
        changeState = false;
}

void WEDataReceiveMode::handleData(WirelessEtherModule *mod, WESignalData *signal)
{
  WirelessEtherDataFrame *data = static_cast<WirelessEtherDataFrame *>(signal->encapsulatedMsg());

    if ((mod->isFrameForMe(data))
        && (mod->associateAP.address == data->getAddress2()) && (mod->address != data->getAddress3())
        && ((mod->associateAP.receivedSequence != data->getSequenceControl().sequenceNumber)
            || (!data->getFrameControl().retry)))
    {
        mod->associateAP.receivedSequence = data->getSequenceControl().sequenceNumber;

        // statistics collection
        mod->noOfRxStat++;
        mod->RxDataBWStat += (double) data->length() / 1000000;
        mod->RxFrameSizeStat->collect(data->length() / 8);
        mod->avgRxFrameSizeStat->collect(data->length() / 8);
        if (mod->statsVec)
            mod->InstRxFrameSizeVec->record(data->length() / 8);

        mod->sendToUpperLayer(data);

        if (data->getAddress1() != MACAddress6(WE_BROADCAST_ADDRESS))
        {
            // send ACK to confirm the transmission has been sucessful
            WirelessEtherBasicFrame *ack = mod->createFrame(FT_CONTROL, ST_ACK,
                                                            MACAddress6(mod->macAddressString().c_str()),
                                                            AC_VO,
                                                            data->getAddress2());
            scheduleAck(mod, ack);

            changeState = false;
        }

        mod->associateAP.channel = signal->channelNum();
        mod->associateAP.rxpower = signal->power();

        mod->signalStrength->updateAverage(signal->power());

        if (mod->signalStrength->getAverage() <= mod->getHOThreshPower())
        {
            wEV << currentTime() << " " << mod->fullPath() << " Active scan due to data threshold too low.\n";
            mod->restartScanning();
        }
    }
}

// Handle beacon solely to monitor signal strength of currently associated AP
void WEDataReceiveMode::handleBeacon(WirelessEtherModule *mod, WESignalData *signal)
{
    WirelessEtherManagementFrame *beacon =
        static_cast<WirelessEtherManagementFrame *>(signal->encapsulatedMsg());

    if ((mod->isFrameForMe(beacon)) && (mod->associateAP.address == beacon->getAddress3()))
    {
        BeaconFrameBody *beaconBody = static_cast<BeaconFrameBody *>(beacon->decapsulate());

        mod->associateAP.rxpower = signal->power();
        mod->signalStrength->updateAverage(signal->power());
        mod->associateAP.estAvailBW = (beaconBody->getHandoverParameters()).estAvailBW;
        mod->associateAP.errorPercentage = (beaconBody->getHandoverParameters()).avgErrorRate;
        mod->associateAP.avgBackoffTime = (beaconBody->getHandoverParameters()).avgBackoffTime;
        mod->associateAP.avgWaitTime = (beaconBody->getHandoverParameters()).avgWaitTime;

        wEV  << mod->fullPath() << "\n"
             << " ----------------------------------------------- \n"
             << " Beacon received by: " << mod->macAddressString() << "\n"
             << " from " << beacon->getAddress2() << " \n"
             << " assAP avg. rxpower: " << mod->signalStrength->getAverage() << "\n"
             << " ho-thresh power: " << mod->getHOThreshPower() << "\n"
             << " ----------------------------------------------- \n";
        if (mod->signalStrength->getAverage() <= mod->getHOThreshPower())
        {
            wEV << currentTime() << " " << mod->fullPath() << " Active scan due to beacon threshold too low \n";
            mod->restartScanning();
        }
        delete beaconBody;
    }
}

void WEDataReceiveMode::handleProbeResponse(WirelessEtherModule *mod, WESignalData *signal)
{
    WirelessEtherManagementFrame *probeResponse =
        check_and_cast<WirelessEtherManagementFrame *>(signal->encapsulatedMsg());

    if (probeResponse->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
    {
        ProbeResponseFrameBody *probeResponseBody =
            static_cast<ProbeResponseFrameBody *>(probeResponse->decapsulate());

        assert(probeResponseBody);
        wEV  << mod->fullPath() << "\n"
             << " ----------------------------------------------- \n"
             << " Probe Response received by: " << mod->macAddressString() << "\n"
             << " from " << probeResponse->getAddress2() << " \n"
             << " SSID: " << probeResponseBody->getSSID() << "\n"
             << " channel: " << probeResponseBody->getDSChannel() << "\n"
             << " rxpower: " << signal->power() << "\n"
             << " ----------------------------------------------- \n";

        // send ACK
        WirelessEtherBasicFrame *ack =
            mod->createFrame(FT_CONTROL, ST_ACK, MACAddress6(mod->macAddressString().c_str()),
                             AC_VO, probeResponse->getAddress2());
        scheduleAck(mod, ack);
        changeState = false;

        delete probeResponseBody;
    }
}
