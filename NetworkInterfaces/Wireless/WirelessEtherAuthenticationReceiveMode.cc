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
    @file WirelessEtherAuthenticationReceiveMode.cc
    @brief Source file for WEAuthenticationReceiveMode

    @author    Steve Woon
          Eric Wu
*/

#include <sys.h> // Dout
#include "debug.h" // Dout

#include "WirelessEtherAssociationReceiveMode.h"
#include "WirelessEtherAuthenticationReceiveMode.h"
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

WEAuthenticationReceiveMode* WEAuthenticationReceiveMode::_instance = 0;

WEAuthenticationReceiveMode* WEAuthenticationReceiveMode::instance()
{
  if (_instance == 0)
    _instance = new WEAuthenticationReceiveMode;

  return _instance;
}

void WEAuthenticationReceiveMode::handleAuthentication(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessEtherManagementFrame* authentication =
      static_cast<WirelessEtherManagementFrame*>(signal->encapsulatedMsg());

  if(authentication->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
  {
    AuthenticationFrameBody* aFrameBody =
      static_cast<AuthenticationFrameBody*>(authentication->decapsulate());

    // Check if its an authentication response for "open system"
    if(aFrameBody->getSequenceNumber() == 2)
    {
      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
           << mod->fullPath() << "\n"
           << " ----------------------------------------------- \n"
           << " Authentication received by: "
           << mod->macAddressString() << "\n"
           << " from " << authentication->getAddress2() << " \n"
           << " Sequence No.: " << aFrameBody->getSequenceNumber() << "\n"
           << " Status Code: " << aFrameBody->getStatusCode() << "\n"
           << " ----------------------------------------------- \n");

      mod->associateAP.address = authentication->getAddress2();
      mod->associateAP.channel = signal->channel();
      mod->associateAP.rxpower = signal->power();
      mod->associateAP.associated = false;
      mod->_currentReceiveMode = WEAssociationReceiveMode::instance();

      // send ACK to confirm the transmission has been sucessful
      WirelessEtherBasicFrame* ack = mod->
        createFrame(FT_CONTROL, ST_ACK,
                    MACAddress6(mod->macAddressString().c_str()),
                    authentication->getAddress2());
      WESignalData* ackSignal = encapsulateIntoWESignalData(ack);
      scheduleAck(mod, ackSignal);
      //delete ack;
      changeState = false;

      // TODO: need to check status code
      //send association request frame
      WirelessEtherBasicFrame* assRequest = mod->
          createFrame(FT_MANAGEMENT, ST_ASSOCIATIONREQUEST,
                    MACAddress6(mod->macAddressString().c_str()),
                    authentication->getAddress2());
      FrameBody* assRequestFrameBody = mod->createFrameBody(assRequest);
      assRequest->encapsulate(assRequestFrameBody);
      WESignalData* requestSignal = encapsulateIntoWESignalData(assRequest);
      mod->outputBuffer.push_back(requestSignal);
      //delete assRequest;

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
      assTmr->reschedule(mod->simTime() + (mod->associationTimeout * TU ));
    }
    delete aFrameBody;
  }
}

void WEAuthenticationReceiveMode::handleProbeResponse(WirelessEtherModule* mod, WESignalData *signal)
{
  WirelessEtherManagementFrame* probeResponse =
  	static_cast<WirelessEtherManagementFrame*>(signal->data());
	
  if (probeResponse->getAddress1() == MACAddress(mod->macAddressString().c_str()))
  {
  	ProbeResponseFrameBody* probeResponseBody =
      static_cast<ProbeResponseFrameBody*>(probeResponse->decapsulate());
	
    assert(probeResponseBody);
    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " 
         << mod->fullPath() << "\n"
         << " ----------------------------------------------- \n"
         << " Probe Response received by: " << mod->macAddressString() << "\n"
         << " from " << probeResponse->getAddress2() << " \n"
         << " SSID: " << probeResponseBody->getSSID() << "\n"
         << " channel: " << probeResponseBody->getDSChannel() << "\n"
         << " rxpower: " << signal->power() << "\n"
         << " ----------------------------------------------- \n");

    // send ACK
    WirelessEtherBasicFrame* ack = mod->
      createFrame(FT_CONTROL, ST_ACK, MACAddress(mod->macAddressString().c_str()),
                  probeResponse->getAddress2());
    WESignalData* ackSignal = new WESignalData(ack);
    scheduleAck(mod, ackSignal);
    delete ack;
    changeState = false;

    delete probeResponseBody;
  } // endif
}

void WEAuthenticationReceiveMode::handleAck(WirelessEtherModule* mod, WESignalData* signal)
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
