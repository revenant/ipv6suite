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

#include <sys.h> // Dout
#include "debug.h" // Dout

#include "WirelessEtherAssociationReceiveMode.h"
#include "WirelessEtherState.h"
#include "WirelessEtherModule.h"
#include "WirelessEtherSignal.h"
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

#include "cTTimerMessageCB.h"

WEAssociationReceiveMode* WEAssociationReceiveMode::_instance = 0;

WEAssociationReceiveMode* WEAssociationReceiveMode::instance()
{
  if (_instance == 0)
    _instance = new WEAssociationReceiveMode;

  return _instance;
}

void WEAssociationReceiveMode::handleAuthentication(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessEtherManagementFrame* authentication =
      static_cast<WirelessEtherManagementFrame*>(signal->data());

  if(authentication->getAddress1() == MACAddress(mod->macAddressString().c_str()))
  {
    AuthenticationFrameBody* aFrameBody =
      static_cast<AuthenticationFrameBody*>(authentication->decapsulate());

    // Check if its an authentication response for "open system"
    if(aFrameBody->getSequenceNumber() == 2)
    {
      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
           << mod->fullPath() << "\n"
           << " ----------------------------------------------- \n"
           << " Authentication Response received by: "
           << mod->macAddressString() << "\n"
           << " from " << authentication->getAddress2() << " \n"
           << " Sequence No.: " << aFrameBody->getSequenceNumber() << "\n"
           << " Status Code: " << aFrameBody->getStatusCode() << "\n"
           << " ----------------------------------------------- \n");

      mod->associateAP.address = authentication->getAddress2();
      mod->associateAP.channel = signal->channel();
      mod->associateAP.rxpower = signal->power();
      mod->associateAP.associated = false;

      // TODO: need to check status code

      // send ACK to confirm the transmission has been sucessful
      WirelessEtherBasicFrame* ack = mod->
        createFrame(FT_CONTROL, ST_ACK,
                    MACAddress(mod->macAddressString().c_str()),
                    authentication->getAddress2());
      WESignalData* ackSignal = new WESignalData(ack);
      sendAck(mod, ackSignal);
      delete ack;
      changeState = false;

      //send association request frame
      WirelessEtherBasicFrame* assRequest = mod->
          createFrame(FT_MANAGEMENT, ST_ASSOCIATIONREQUEST,
                    MACAddress(mod->macAddressString().c_str()),
                    authentication->getAddress2());

      FrameBody* assRequestFrameBody = mod->createFrameBody(assRequest);
      assRequest->encapsulate(assRequestFrameBody);
      WESignalData* requestSignal = new WESignalData(assRequest);
      mod->outputBuffer.push_back(requestSignal);
      delete assRequest;

      // Stop the authentication timeout timer
      cTimerMessage* authTmr = mod->getTmrMessage(TMR_AUTHTIMEOUT);
      assert(authTmr);
      if(authTmr->isScheduled())
      {
        authTmr->cancel();
      }

      // Start the association timeout timer
      cTimerMessage* assTmr = mod->getTmrMessage(TMR_ASSTIMEOUT);
      assert(assTmr);
      if(assTmr->isScheduled())
      {
        assTmr->cancel();
      }
      assTmr->reschedule(mod->simTime() + (mod->associationTimeout * TU));
    }
    delete aFrameBody;
  }
}

void WEAssociationReceiveMode::handleDeAuthentication(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessEtherManagementFrame* deAuthentication =
      static_cast<WirelessEtherManagementFrame*>(signal->data());

  if(deAuthentication->getAddress1() == MACAddress(mod->macAddressString().c_str()))
  {

    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << mod->fullPath() << "\n"
         << " ----------------------------------------------- (WEAssociationReceiveMode::handleDeAuthentication) \n"
         << " De-authentication received by: "
         << mod->macAddressString() << "\n"
         << " from " << deAuthentication->getAddress2() << " \n"
         << " Reason Code: "
         << (static_cast<DeAuthenticationFrameBody*>(deAuthentication->decapsulate()))
         ->getReasonCode() << "\n"
         << " ----------------------------------------------- \n");

    // TODO: may need to handle reason code as well
    // send ACK to confirm the transmission has been sucessful
    WirelessEtherBasicFrame* ack = mod->
      createFrame(FT_CONTROL, ST_ACK,
                  MACAddress(mod->macAddressString().c_str()),
                  deAuthentication->getAddress2());
    WESignalData* ackSignal = new WESignalData(ack);
    sendAck(mod, ackSignal);
       delete ack;
    changeState = false;

    if(mod->associateAP.address == deAuthentication->getAddress3())
    {
      mod->associateAP.address = MAC_ADDRESS_UNSPECIFIED_STRUCT;
      mod->associateAP.channel = INVALID_CHANNEL;
      mod->associateAP.rxpower = INVALID_POWER;
      mod->associateAP.associated = false;
      if(mod->activeScan)
        mod->_currentReceiveMode = WEAScanReceiveMode::instance();
      else
        mod->_currentReceiveMode = WEPScanReceiveMode::instance();
      // TODO: call the function inside the WAScanReceiveMode
    }
  }
}

void WEAssociationReceiveMode::handleAssociationResponse(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessEtherManagementFrame* associationResponse =
      static_cast<WirelessEtherManagementFrame*>(signal->data());

  if(associationResponse->getAddress1() == MACAddress(mod->macAddressString().c_str()))
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
                  MACAddress(mod->macAddressString().c_str()),
                  associationResponse->getAddress2());
    WESignalData* ackSignal = new WESignalData(ack);
    sendAck(mod, ackSignal);
    delete ack;
    changeState = false;

    if(mod->associateAP.address == associationResponse->getAddress2())
    {
      std::cout<<mod->simTime()<<" "<<mod->fullPath()<<" associated to: "<<associationResponse->getAddress2()<<" on chan: "<<signal->channel()<<" sig strength: "<< signal->power()<<std::endl;
      mod->associateAP.address = associationResponse->getAddress2();
      mod->associateAP.channel = signal->channel();
      mod->associateAP.rxpower = signal->power();
      mod->associateAP.associated = true;
      mod->_currentReceiveMode = WEDataReceiveMode::instance();
      mod->makeOfflineBufferAvailable();
      mod->handoverTarget.valid = false;

      // Stop the association timeout timer
      cTimerMessage* assTmr = mod->getTmrMessage(TMR_ASSTIMEOUT);
      assert(assTmr);
      if(assTmr->isScheduled())
      {
        assTmr->cancel();
      }

      // Send upper layer signal of success
      mod->sendSuccessSignal();

      if (mod->linkdownTime != 0)
      {
        mod->l2HODelay->record(mod->simTime() - mod->linkdownTime);
        mod->linkdownTime = 0;

        L2DelayTmr* l2delayRecorder = mod->getLayer2DelayRecorder();
        if(l2delayRecorder)
        {
          assert(!l2delayRecorder->isScheduled());
          Loki::Field<0> (l2delayRecorder->args) = mod->simTime();
          l2delayRecorder->reschedule(mod->simTime() + SELF_SCHEDULE_DELAY);
        }
      }

      mod->noOfFailedTx = 0;
      mod->noOfSuccessfulTx = 0;

#ifdef EWU_L2TRIGGER // Link up trigger ( movement detection for MIPv6 )
      assert(mod->getLayer2Trigger(LinkUP) && !mod->getLayer2Trigger(LinkUP)->isScheduled());
        mod->getLayer2Trigger(LinkUP)->reschedule(mod->simTime() + SELF_SCHEDULE_DELAY);
        Dout(dc::mipv6|dc::mobile_move, mod->fullPath()<<"Link-Up trigger signalled (Assoc) "<<  mod->associateAP.channel);
#endif // EWU_L2TRIGGER
    }
    // TODO: need to check supported rates and status code
    delete aRFrameBody;
  }
}

void WEAssociationReceiveMode::handleReAssociationResponse(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessEtherManagementFrame* reAssociationResponse =
      static_cast<WirelessEtherManagementFrame*>(signal->data());

  if(reAssociationResponse->getAddress1() == MACAddress(mod->macAddressString().c_str()))
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
                  MACAddress(mod->macAddressString().c_str()),
                  reAssociationResponse->getAddress2());
    WESignalData* ackSignal = new WESignalData(ack);
    sendAck(mod, ackSignal);
       delete ack;
    changeState = false;

    if(mod->associateAP.address == reAssociationResponse->getAddress2())
    {
      mod->associateAP.address = reAssociationResponse->getAddress2();
      mod->associateAP.channel = signal->channel();
      mod->associateAP.rxpower = signal->power();
      mod->associateAP.associated = true;
      mod->_currentReceiveMode = WEDataReceiveMode::instance();
            mod->makeOfflineBufferAvailable();
    }


#ifdef EWU_L2TRIGGER // Link up trigger ( movement detection for MIPv6 )
      assert(mod->getLayer2Trigger(LinkUP) && !mod->getLayer2Trigger(LinkUP)->isScheduled());
        mod->getLayer2Trigger(LinkUP)->reschedule(mod->simTime() + SELF_SCHEDULE_DELAY);
        Dout(dc::mipv6|dc::mobile_move, mod->fullPath()<<"Link-Up trigger signalled (Reassoc) "<<  mod->associateAP.channel);
#endif // EWU_L2TRIGGER

    // TODO: need to check supported rates and status code
    delete rARFrameBody;
  }
}

void WEAssociationReceiveMode::handleAck(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessEtherBasicFrame* ack =
      static_cast<WirelessEtherBasicFrame*>(signal->data());

  if(ack->getAddress1() == MACAddress(mod->macAddressString().c_str()))
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
