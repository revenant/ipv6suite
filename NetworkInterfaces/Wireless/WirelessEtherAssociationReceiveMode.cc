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
    @file WirelessEtherAssociationReceiveMode.cc
    @brief Source file for WEAssociationReceiveMode

    @author    Steve Woon
          Eric Wu
*/

#include "sys.h"
#include "debug.h"

#include "WirelessEtherAssociationReceiveMode.h"
#include <iostream>
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

#include "WirelessEtherDataReceiveMode.h"
#include "WirelessEtherAScanReceiveMode.h"
#include "WirelessEtherPScanReceiveMode.h"
#include "WEQueue.h"
#include "cTimerMessage.h" //link up trigger
#include "TimerConstants.h" // SELF_SCHEDULE_DELAY
#include "IPv6Utils.h"
#include "opp_utils.h"
#include "NotificationBoard.h" //for rtp handover l2 up signal
#include "NotifierConsts.h"


WEAssociationReceiveMode *WEAssociationReceiveMode::_instance = 0;

WEAssociationReceiveMode *WEAssociationReceiveMode::instance()
{
    if (_instance == 0)
        _instance = new WEAssociationReceiveMode;

    return _instance;
}

void WEAssociationReceiveMode::handleAuthentication(WirelessEtherModule *mod, WESignalData *signal)
{
    WirelessEtherManagementFrame *authentication =
        static_cast<WirelessEtherManagementFrame *>(signal->encapsulatedMsg());

    if (authentication->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
    {
        AuthenticationFrameBody *aFrameBody =
            static_cast<AuthenticationFrameBody *>(authentication->decapsulate());

        // Check if its an authentication response for "open system"
        if (aFrameBody->getSequenceNumber() == 2)
        {
            wEV  << mod->fullPath() << "\n"
                 << " ----------------------------------------------- \n"
                 << " Authentication Response received by: "
                 << mod->macAddressString() << "\n"
                 << " from " << authentication->getAddress2() << " \n"
                 << " Sequence No.: " << aFrameBody->getSequenceNumber() << "\n"
                 << " Status Code: " << aFrameBody->getStatusCode() << "\n"
                 << " ----------------------------------------------- \n";

            mod->associateAP.address = authentication->getAddress2();
            mod->associateAP.channel = signal->channelNum();
            mod->associateAP.rxpower = signal->power();
            mod->associateAP.associated = false;

            // TODO: need to check status code

            // send ACK to confirm the transmission has been sucessful
            WirelessEtherBasicFrame *ack = mod->createFrame(FT_CONTROL, ST_ACK,
                                                            MACAddress6(mod->macAddressString().c_str()),
                                                            AC_VO,
                                                            authentication->getAddress2());
            scheduleAck(mod, ack);
            changeState = false;

            // send association request frame
            WirelessEtherBasicFrame *assRequest = mod->createFrame(FT_MANAGEMENT, ST_ASSOCIATIONREQUEST,
                                                                   MACAddress6(mod->macAddressString().
                                                                               c_str()), AC_VO,
                                                                   authentication->getAddress2());

            FrameBody *assRequestFrameBody = mod->createFrameBody(assRequest);
            assRequest->encapsulate(assRequestFrameBody);
            mod->outputQueue->insertFrame(assRequest, mod->simTime());

            // Stop the authentication timeout timer
            if (mod->authTimeoutNotifier->isScheduled())
                mod->cancelEvent(mod->authTimeoutNotifier);

            // Start the association timeout timer
            mod->reschedule(mod->assTimeoutNotifier, mod->simTime() + (mod->associationTimeout * TU));
        }
        delete aFrameBody;
    }
}

void WEAssociationReceiveMode::handleDeAuthentication(WirelessEtherModule *mod, WESignalData *signal)
{
    WirelessEtherManagementFrame *deAuthentication =
        static_cast<WirelessEtherManagementFrame *>(signal->encapsulatedMsg());

    if (deAuthentication->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
    {

        wEV  << mod->fullPath() << "\n"
             << " ----------------------------------------------- (WEAssociationReceiveMode::handleDeAuthentication) \n"
             << " De-authentication received by: " << mod->macAddressString() << "\n"
             << " from " << deAuthentication->getAddress2() << " \n"
             << " Reason Code: " << (static_cast <DeAuthenticationFrameBody *>(deAuthentication->decapsulate()))->getReasonCode() << "\n"
             << " ----------------------------------------------- \n";

        // TODO: may need to handle reason code as well
        // send ACK to confirm the transmission has been sucessful
        WirelessEtherBasicFrame *ack = mod->createFrame(FT_CONTROL, ST_ACK,
                                                        MACAddress6(mod->macAddressString().c_str()), AC_VO,
                                                        deAuthentication->getAddress2());
        scheduleAck(mod, ack);
        changeState = false;

        if (mod->associateAP.address == deAuthentication->getAddress3())
        {
            mod->associateAP.address = MAC_ADDRESS_UNSPECIFIED_STRUCT;
            mod->associateAP.channel = INVALID_CHANNEL;
            mod->associateAP.rxpower = INVALID_POWER;
            mod->associateAP.associated = false;
            mod->restartScanning();
        }
    }
}

void WEAssociationReceiveMode::handleAssociationResponse(WirelessEtherModule *mod, WESignalData *signal)
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
                 << " \t supported?" << aRFrameBody->getSupportedRates(i).supported << "\n";

        // send ACK to confirm the transmission has been sucessful
        WirelessEtherBasicFrame *ack = mod->createFrame(FT_CONTROL, ST_ACK,
                                                        MACAddress6(mod->macAddressString().c_str()), AC_VO,
                                                        associationResponse->getAddress2());
        scheduleAck(mod, ack);
        changeState = false;

        if (mod->associateAP.address == associationResponse->getAddress2())
        {
            Dout(dc::mobile_move, mod->simTime() << " " << OPP_Global::nodeName(mod) << " associated to: "
                 << associationResponse->getAddress2() << " on chan: " << signal->channelNum()
                 << " sig strength: " << signal->power());
	    std::ostream& os = IPv6Utils::printRoutingInfo(false, 0, "", false);
	    os <<OPP_Global::nodeName(mod) <<" "<<simulation.simTime()<< " associated to: "
	       << associationResponse->getAddress2() << " on chan: " 
	       << signal->channelNum()<< " sig strength: " << signal->power()<<"\n";
            //std::cout<<mod->simTime()<<" "<<mod->fullPath()<<" associated to: " <<associationResponse->getAddress2()<<" on chan: "<<signal->channelNum()<<" sig strength: "<<signal->power()<<std::endl;

            mod->associateAP.address = associationResponse->getAddress2();
            mod->associateAP.channel = signal->channelNum();
            mod->associateAP.rxpower = signal->power();
            mod->associateAP.associated = true;
	    mod->changeReceiveMode(WEDataReceiveMode::instance());
            mod->makeOfflineBufferAvailable();
            mod->handoverTarget.valid = false;

            // Stop the association timeout timer
            if (mod->assTimeoutNotifier->isScheduled())
                mod->cancelEvent(mod->assTimeoutNotifier);

            // Send upper layer signal of success
            mod->sendSuccessSignal();

            if (mod->linkdownTime != 0)
            {
                mod->totalDisconnectedTime += mod->simTime() - mod->linkdownTime;

                cMessage *linkUpTimeMsg = new cMessage;
                linkUpTimeMsg->setTimestamp();
                linkUpTimeMsg->setKind(LinkUP);
                mod->sendDirect(linkUpTimeMsg,
                                0, OPP_Global::findModuleByName(mod, "mobility"), "l2TriggerIn");
		mod->nb->fireChangeNotification(NF_L2_ASSOCIATED);

                mod->linkdownTime = 0;
            }
//* XXX maybe something like this would be useful:
	    if (ev.isGUI())
	    {
	      mod->bubble("Handover completed!");
	      mod->parentModule()->bubble("Handover completed!");
	      mod->parentModule()->parentModule()->bubble("Handover completed!");
	    }

            // Link up trigger ( movement detection for MIPv6 )
            if ( mod->linkUpTrigger() )
            {
              if (mod->getLayer2Trigger(LinkUP))
	      {
              assert(!mod->getLayer2Trigger(LinkUP)->isScheduled());
              mod->getLayer2Trigger(LinkUP)->reschedule(mod->simTime() + SELF_SCHEDULE_DELAY);
              Dout(dc::mipv6 | dc::mobile_move,
                   mod->fullPath() << "Link-Up trigger signalled (Assoc) " << mod->associateAP.channel);
              }
            }
        }
        // TODO: need to check supported rates and status code
        delete aRFrameBody;
    }
}

void WEAssociationReceiveMode::handleReAssociationResponse(WirelessEtherModule *mod, WESignalData *signal)
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

        if (mod->associateAP.address == reAssociationResponse->getAddress2())
        {
            mod->associateAP.address = reAssociationResponse->getAddress2();
            mod->associateAP.channel = signal->channelNum();
            mod->associateAP.rxpower = signal->power();
            mod->associateAP.associated = true;
            mod->changeReceiveMode(WEDataReceiveMode::instance());
            mod->makeOfflineBufferAvailable();
        }

        if (mod->linkUpTrigger())       //  movement detection for MIPv6
        {
            assert(mod->getLayer2Trigger(LinkUP) && !mod->getLayer2Trigger(LinkUP)->isScheduled());
            mod->getLayer2Trigger(LinkUP)->reschedule(mod->simTime() + SELF_SCHEDULE_DELAY);
            Dout(dc::mipv6 | dc::mobile_move,
                 mod->fullPath() << "Link-Up trigger signalled (Reassoc) " << mod->associateAP.channel);
        }

        // TODO: need to check supported rates and status code
        delete rARFrameBody;
    }
}

void WEAssociationReceiveMode::handleProbeResponse(WirelessEtherModule *mod, WESignalData *signal)
{
    WirelessEtherManagementFrame *probeResponse =
        check_and_cast<WirelessEtherManagementFrame *>(signal->encapsulatedMsg());

    if (probeResponse->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
    {
        ProbeResponseFrameBody *probeResponseBody =
            check_and_cast<ProbeResponseFrameBody *>(probeResponse->decapsulate());

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
        WirelessEtherBasicFrame *ack = mod->
            createFrame(FT_CONTROL, ST_ACK, MACAddress6(mod->macAddressString().c_str()), AC_VO,
                        probeResponse->getAddress2());
        scheduleAck(mod, ack);
        changeState = false;

        delete probeResponseBody;
    }
}

void WEAssociationReceiveMode::handleAck(WirelessEtherModule *mod, WESignalData *signal)
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
