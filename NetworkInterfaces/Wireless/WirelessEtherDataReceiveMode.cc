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

#include "sys.h" // Dout
#include "debug.h" // Dout

#include <iostream>
#include <iomanip>
#include <omnetpp.h>

#include "WirelessEtherDataReceiveMode.h"
#include "cTTimerMessageCB.h"
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

WEDataReceiveMode* WEDataReceiveMode::_instance = 0;

WEDataReceiveMode* WEDataReceiveMode::instance()
{
  if (_instance == 0)
    _instance = new WEDataReceiveMode;

  return _instance;
}

void WEDataReceiveMode::handleAssociationResponse(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessEtherManagementFrame* associationResponse =
      static_cast<WirelessEtherManagementFrame*>(signal->encapsulatedMsg());

  if(associationResponse->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
  {
    AssociationResponseFrameBody* aRFrameBody =
      static_cast<AssociationResponseFrameBody*>(associationResponse->decapsulate());

    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << mod->fullPath() << "\n"
         << " ----------------------------------------------- \n"
         << " Association Response received by: "
         << mod->macAddressString() << "\n"
         << " from " << associationResponse->getAddress2() << " \n"
         << " Status Code: " << aRFrameBody->getStatusCode() << "\n"
         << " ----------------------------------------------- \n");
    for ( size_t i = 0; i < aRFrameBody->getSupportedRatesArraySize(); i++ )
      Dout(dc::wireless_ethernet|flush_cf,
           " Supported Rates: \n "
           << " [" << i << "] \t rate: "
           << aRFrameBody->getSupportedRates(i).rate << " \n"
           << " \t supported?"
           << aRFrameBody->getSupportedRates(i).supported
           << "\n");

    // send ACK to confirm the transmission has been sucessful
    WirelessEtherBasicFrame* ack = mod->
      createFrame(FT_CONTROL, ST_ACK,
                  MACAddress6(mod->macAddressString().c_str()),
                  associationResponse->getAddress2());
    WESignalData* ackSignal = encapsulateIntoWESignalData(ack);
    sendAck(mod, ackSignal);
    //delete ack;
    changeState = false;

    if(mod->associateAP.address == associationResponse->getAddress2())
    {
      mod->associateAP.address = associationResponse->getAddress2();
      mod->associateAP.channel = signal->channel();
      mod->associateAP.rxpower = signal->power();
      mod->associateAP.associated = true;
      // already in data receive mode so dont need to switch to it

      // Stop the association timeout timer
      cTimerMessage* assTmr = mod->getTmrMessage(TMR_ASSTIMEOUT);
      assert(assTmr);
      if(assTmr->isScheduled())
      {
        assTmr->cancel();
      }

      // Send upper layer signal of success
      mod->sendSuccessSignal();

#ifdef EWU_L2TRIGGER // Link up trigger ( movement detection for MIPv6 )
      ////dc.precision(6);
      assert(mod->getLayer2Trigger() && !mod->getLayer2Trigger()->isScheduled());
        mod->getLayer2Trigger()->reschedule(mod->simTime() + SELF_SCHEDULE_DELAY);
        Dout(dc::mipv6|dc::mobile_move, mod->fullPath()<<" Link-Up trigger signalled " << (mod->simTime() + SELF_SCHEDULE_DELAY));
#endif // EWU_L2TRIGGER
    }
    // TODO: need to check supported rates and status code
    delete aRFrameBody;
  }
}

void WEDataReceiveMode::handleReAssociationResponse(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessEtherManagementFrame* reAssociationResponse =
      static_cast<WirelessEtherManagementFrame*>(signal->encapsulatedMsg());

  if(reAssociationResponse->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
  {
    ReAssociationResponseFrameBody* rARFrameBody =
      static_cast<ReAssociationResponseFrameBody*>(reAssociationResponse->decapsulate());

    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << mod->fullPath() << "\n"
         << " ----------------------------------------------- \n"
         << " Reassociation Response received by: "
         << mod->macAddressString() << "\n"
         << " from " << reAssociationResponse->getAddress2() << " \n"
         << " Status Code: " << rARFrameBody->getStatusCode() << "\n"
         << " ----------------------------------------------- \n");
    for ( size_t i = 0; i < rARFrameBody->getSupportedRatesArraySize(); i++ )
      Dout(dc::wireless_ethernet|flush_cf,
           " Supported Rates: \n "
           << " [" << i << "] \t rate: "
           << rARFrameBody->getSupportedRates(i).rate << " \n"
           << " \t supported?"
           <<    rARFrameBody->getSupportedRates(i).supported
           << "\n");

    // send ACK to confirm the transmission has been sucessful
    WirelessEtherBasicFrame* ack = mod->
      createFrame(FT_CONTROL, ST_ACK,
                  MACAddress6(mod->macAddressString().c_str()),
                  reAssociationResponse->getAddress2());
    WESignalData* ackSignal = encapsulateIntoWESignalData(ack);
    sendAck(mod, ackSignal);
    //delete ack;
    changeState = false;

    if(mod->associateAP.address == reAssociationResponse->getAddress3())
    {
      mod->associateAP.address = reAssociationResponse->getAddress2();
      mod->associateAP.channel = signal->channel();
      mod->associateAP.rxpower = signal->power();
      mod->associateAP.associated = true;
      // already in data receive mode so dont need to switch to it
    }
    // TODO: need to check supported rates and status code
    delete rARFrameBody;
  }
}

void WEDataReceiveMode::handleDeAuthentication(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessEtherManagementFrame* deAuthentication =
      static_cast<WirelessEtherManagementFrame*>(signal->encapsulatedMsg());

  if(deAuthentication->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
  {
    DeAuthenticationFrameBody* deAuthFrameBody =
      static_cast<DeAuthenticationFrameBody*>(deAuthentication->decapsulate());

    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << mod->fullPath() << "\n"
         << " ----------------------------------------------- (WEDataReceiveMode::handleDeAuthentication)\n"
         << " De-authentication received by: "
         << mod->macAddressString() << "\n"
         << " from " << deAuthentication->getAddress2() << " \n"
         << " Reason Code: " << deAuthFrameBody->getReasonCode() << "\n"
         << " ----------------------------------------------- \n");

    // TODO: may need to handle reason code as well
    // send ACK to confirm the transmission has been sucessful
    WirelessEtherBasicFrame* ack = mod->
      createFrame(FT_CONTROL, ST_ACK,
                  MACAddress6(mod->macAddressString().c_str()),
                  deAuthentication->getAddress2());
    WESignalData* ackSignal = encapsulateIntoWESignalData(ack);
    sendAck(mod, ackSignal);
    //delete ack;
    changeState = false;

    if(mod->associateAP.address == deAuthentication->getAddress3())
      mod->restartScanning();

    delete deAuthFrameBody;
  }
}

void WEDataReceiveMode::handleDisAssociation(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessEtherManagementFrame* disAssociation =
      static_cast<WirelessEtherManagementFrame*>(signal->encapsulatedMsg());

  if(disAssociation->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
  {
    DisAssociationFrameBody* disAssFrameBody =
      static_cast<DisAssociationFrameBody*>(disAssociation->decapsulate());

    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << mod->fullPath() << "\n"
         << " ----------------------------------------------- \n"
         << " Dis-association received by: "
         << mod->macAddressString() << "\n"
         << " from " << disAssociation->getAddress2() << " \n"
         << " Reason Code: " << disAssFrameBody->getReasonCode() << "\n"
         << " ----------------------------------------------- \n");

    // TODO: may need to handle reason code as well
    // send ACK to confirm the transmission has been sucessful
    WirelessEtherBasicFrame* ack = mod->
      createFrame(FT_CONTROL, ST_ACK,
                  MACAddress6(mod->macAddressString().c_str()),
                  disAssociation->getAddress2());
    WESignalData* ackSignal = encapsulateIntoWESignalData(ack);
    sendAck(mod, ackSignal);
    //delete ack;
    changeState = false;

    if(mod->associateAP.address == disAssociation->getAddress3())
    {
      mod->associateAP.associated = false;

      mod->_currentReceiveMode = WEAuthenticationReceiveMode::instance();
      // may need to initiate association again or re-scan
    }

    delete disAssFrameBody;
  }
}

void WEDataReceiveMode::handleAck(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessEtherBasicFrame* ack =
      static_cast<WirelessEtherBasicFrame*>(signal->encapsulatedMsg());

  if(ack->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
  {
    // Since both interfaces have the same MAC Address in a dual interface node, you may
    // process an ACK which is meant for the other interface. Condition checks that you
    // are expecting an ACK before processing it.
    if( mod->currentState() == WirelessEtherStateAwaitACKReceive::instance())
    {
      mod->getTmrMessage(WIRELESS_SELF_AWAITACK)->cancel();

      finishFrameTx(mod);
      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
           << mod->fullPath() << "\n"
           << " ----------------------------------------------- \n"
           << " ACK received by: " << mod->macAddressString() << "\n"
           << " ----------------------------------------------- \n");

      changeState = false;
    }
  }

  if ( mod->currentState() == WirelessEtherStateAwaitACKReceive::instance())
    changeState = false;
}

void WEDataReceiveMode::handleData(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessEtherDataFrame* data =
      static_cast<WirelessEtherDataFrame*>(signal->encapsulatedMsg());

  if((mod->isFrameForMe(data))
      &&(mod->associateAP.address == data->getAddress2()) && (mod->address != data->getAddress3()))
  {
    mod->sendToUpperLayer(data);

    if(data->getAddress1() != MACAddress6(WE_BROADCAST_ADDRESS))
    {
      // send ACK to confirm the transmission has been sucessful
      WirelessEtherBasicFrame* ack = mod->
          createFrame(FT_CONTROL, ST_ACK,
                    MACAddress6(mod->macAddressString().c_str()),
                      data->getAddress2());
      WESignalData* ackSignal = encapsulateIntoWESignalData(ack);
      sendAck(mod, ackSignal);
      //delete ack;
      changeState = false;
    }

    mod->associateAP.channel = signal->channel();
    mod->associateAP.rxpower = signal->power();

    mod->signalStrength->updateAverage(signal->power());

    if(mod->signalStrength->getAverage() <= mod->getHOThreshPower())
    {
      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << mod->simTime() << " " << mod->fullPath() << " Active scan due to data threshold too low.");
      mod->restartScanning();
      mod->linkdownTime = mod->simTime();

      if(mod->l2LinkDownRecorder)
      {
        assert(!mod->l2LinkDownRecorder->isScheduled());
        Loki::Field<0> (mod->l2LinkDownRecorder->args) = mod->simTime();
        mod->l2LinkDownRecorder->reschedule(mod->simTime() + SELF_SCHEDULE_DELAY);
      }
    }
  }
}

// Handle beacon solely to monitor signal strength of currently associated AP
void WEDataReceiveMode::handleBeacon(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessEtherManagementFrame* beacon =
    static_cast<WirelessEtherManagementFrame*>(signal->encapsulatedMsg());

  if(   (mod->isFrameForMe(beacon))
        &&(mod->associateAP.address == beacon->getAddress3()) )
  {
    BeaconFrameBody* beaconBody =
      static_cast<BeaconFrameBody*>(beacon->decapsulate());

    mod->associateAP.rxpower = signal->power();
    mod->signalStrength->updateAverage(signal->power());
      mod->associateAP.estAvailBW = (beaconBody->getHandoverParameters()).estAvailBW;
    mod->associateAP.errorPercentage = (beaconBody->getHandoverParameters()).avgErrorRate;
    mod->associateAP.avgBackoffTime = (beaconBody->getHandoverParameters()).avgBackoffTime;
    mod->associateAP.avgWaitTime = (beaconBody->getHandoverParameters()).avgWaitTime;

    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << mod->fullPath() << "\n"
         << " ----------------------------------------------- \n"
         << " Beacon received by: "
         << mod->macAddressString() << "\n"
         << " from " << beacon->getAddress2() << " \n"
         << " assAP avg. rxpower: " << mod->signalStrength->getAverage() << "\n"
         << " ho-thresh power: " << mod->getHOThreshPower() << "\n"
         << " ----------------------------------------------- \n");
    if(mod->signalStrength->getAverage() <= mod->getHOThreshPower())
    {
      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << mod->simTime() << " " << mod->fullPath() << " Active scan due to beacon threshold too low ");
      mod->restartScanning();
      mod->linkdownTime = mod->simTime();

      if(mod->l2LinkDownRecorder)
      {
        assert(!mod->l2LinkDownRecorder->isScheduled());
        Loki::Field<0> (mod->l2LinkDownRecorder->args) = mod->simTime();
        mod->l2LinkDownRecorder->reschedule(mod->simTime() + SELF_SCHEDULE_DELAY);
      }
    }
  } // endif
}
