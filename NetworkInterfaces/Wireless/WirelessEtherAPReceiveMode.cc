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
  @file WirelessEtherAPReceiveMode.cc
  @brief Source file for WEAPReceiveMode

  @author    Steve Woon
  Eric Wu
*/

#include "sys.h" // Dout
#include "debug.h" // Dout
#include "config.h"

#include "WirelessAccessPoint.h"
#include "WirelessEtherAssociationReceiveMode.h"
#include "WirelessEtherAPReceiveMode.h"
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

WEAPReceiveMode* WEAPReceiveMode::_instance = 0;

WEAPReceiveMode* WEAPReceiveMode::instance()
{
  if (_instance == 0)
    _instance = new WEAPReceiveMode;

  return _instance;
}

void WEAPReceiveMode::handleAuthentication(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessAccessPoint* apMod = static_cast<WirelessAccessPoint*>(mod);

  WirelessEtherManagementFrame* authentication =
      static_cast<WirelessEtherManagementFrame*>(signal->encapsulatedMsg());

  if(apMod->isFrameForMe(static_cast<WirelessEtherBasicFrame*>(authentication)))
  {
    AuthenticationFrameBody* aFrameBody =
      static_cast<AuthenticationFrameBody*>(authentication->decapsulate());

    // Check if its an authentication request for "open system"
    if(aFrameBody->getSequenceNumber() == 1)
    {
      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
           << apMod->fullPath() << "\n"
           << " ----------------------------------------------- \n"
           << " Authentication received by: "
           << apMod->macAddressString() << "\n"
           << " from " << authentication->getAddress2() << " \n"
           << " Sequence No.: " << aFrameBody->getSequenceNumber() << "\n"
           << " Status Code: " << aFrameBody->getStatusCode() << "\n"
           << " ----------------------------------------------- \n");

      // send ACK to confirm the transmission has been sucessful
      WirelessEtherBasicFrame* ack = apMod->
        createFrame(FT_CONTROL, ST_ACK,
                    MACAddress6(apMod->macAddressString().c_str()),
                    authentication->getAddress2());
      WESignalData* ackSignal = encapsulateIntoWESignalData(ack);
      sendAck(apMod, ackSignal);
      //delete ack;
      changeState = false;

      apMod->addIface(authentication->getAddress2(), RM_AUTHRSP_ACKWAIT);
      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << mod->fullPath() << " " << mod->simTime()
         << " Setting iface: " << authentication->getAddress2()
         << " RM: " << RM_AUTHRSP_ACKWAIT);

      // TODO: check supported rates, etc we set sucessful for now
      apMod->setIfaceStatus(authentication->getAddress2(), SC_SUCCESSFUL);

      WirelessEtherBasicFrame* auth = apMod->
        createFrame(FT_MANAGEMENT, ST_AUTHENTICATION,
                    apMod->address,
                    authentication->getAddress2());

      FrameBody* authFrameBody = apMod->createFrameBody(auth);
      auth->encapsulate(authFrameBody);
      WESignalData* responseSignal = encapsulateIntoWESignalData(auth);
      apMod->outputBuffer.push_back(responseSignal);
      //delete auth;
    }
      delete aFrameBody;
  }
}

void WEAPReceiveMode::handleAssociationRequest(WirelessAccessPoint* mod, WESignalData* signal)
{
  WirelessEtherManagementFrame* associationRequest =
      static_cast<WirelessEtherManagementFrame*>(signal->encapsulatedMsg());

  AssociationRequestFrameBody* aRFrameBody =
    static_cast<AssociationRequestFrameBody*>
    (associationRequest->decapsulate());

  if((mod->isFrameForMe(static_cast<WirelessEtherBasicFrame*>(associationRequest)))
     &&(mod->ssid == aRFrameBody->getSSID()))
  {
    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << mod->fullPath() << "\n"
         << " ----------------------------------------------- \n"
         << " Association Request received by: "
         << mod->macAddressString() << "\n"
         << " from " << associationRequest->getAddress2() << " \n"
         << " SSID: " << aRFrameBody->getSSID() << "\n"
         << " ----------------------------------------------- \n");
    for ( size_t i = 0; i < aRFrameBody->getSupportedRatesArraySize(); i++ )
      Dout(dc::wireless_ethernet|flush_cf,
           " Supported Rates: \n "
           << " [" << i << "] \t rate: "
           << aRFrameBody->getSupportedRates(i).rate << " \n"
           << " \t supported?" << aRFrameBody->getSupportedRates(i).supported
           << "\n");

    // send ACK to confirm the transmission has been sucessful
    WirelessEtherBasicFrame* ack = mod->
      createFrame(FT_CONTROL, ST_ACK,
                  MACAddress6(mod->macAddressString().c_str()),
                  associationRequest->getAddress2());
    WESignalData* ackSignal = encapsulateIntoWESignalData(ack);
    sendAck(mod, ackSignal);
    //delete ack;
    changeState = false;

    WirelessEtherInterface iface = mod->findIfaceByMAC(MACAddress6(associationRequest->getAddress2()));
    WESignalData* responseSignal;

    if(iface.receiveMode == RM_ASSOCIATION)
    {
      mod->addIface(associationRequest->getAddress2(), RM_ASSRSP_ACKWAIT);

      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << mod->fullPath() << " " << mod->simTime()
         << " Setting iface: " << associationRequest->getAddress2()
         << " RM: " << RM_ASSRSP_ACKWAIT);

      // TODO: check supported rates, etc we set sucessful for now
      mod->setIfaceStatus(associationRequest->getAddress2(), SC_SUCCESSFUL);

      WirelessEtherBasicFrame* assResponse = mod->
        createFrame(FT_MANAGEMENT, ST_ASSOCIATIONRESPONSE,
                    MACAddress6(mod->macAddressString().c_str()),
                    associationRequest->getAddress2());
      FrameBody* assResponseFrameBody = mod->createFrameBody(assResponse);
      assResponse->encapsulate(assResponseFrameBody);
      responseSignal = encapsulateIntoWESignalData(assResponse);
      //delete assResponse;

#ifdef EWU_MACBRIDGE
      cMessage * dataPkt = new cMessage;
      dataPkt->setKind(WE_MAC_BRIDGE_REGISTER);

      cPar* msAddressPar = new cPar;
      msAddressPar->setStringValue(associationRequest->getAddress2().stringValue());
      dataPkt->addPar(msAddressPar);

      WirelessEtherBasicFrame* sendFrame = mod->
         createFrame(FT_DATA, ST_DATA, mod->macAddressString().c_str(), WE_BROADCAST_ADDRESS);
      sendFrame->encapsulate(dataPkt);
      mod->send(sendFrame, mod->inputQueueOutGate());
#endif // EWU_MAC_BRIDGE
    }
    else // send deauthentication
    {
      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
                << mod->fullPath() <<" Sending De-authent to "
                << associationRequest->getAddress2()
                << " Not in RM_ASSOCIATION: " << iface.receiveMode );

      WirelessEtherBasicFrame* deAuthentication = mod->
        createFrame(FT_MANAGEMENT, ST_DEAUTHENTICATION,
                    MACAddress6(mod->macAddressString().c_str()),
                    associationRequest->getAddress2());
      FrameBody* deAuthFrameBody = mod->createFrameBody(deAuthentication);
      deAuthentication->encapsulate(deAuthFrameBody);
      responseSignal = encapsulateIntoWESignalData(deAuthentication);
      //delete deAuthentication;
    }
    mod->outputBuffer.push_back(responseSignal);
  }
  delete aRFrameBody;
}

void WEAPReceiveMode::handleReAssociationRequest(WirelessAccessPoint* mod, WESignalData* signal)
{
  WirelessEtherManagementFrame* reAssociationRequest =
      static_cast<WirelessEtherManagementFrame*>(signal->encapsulatedMsg());

  ReAssociationRequestFrameBody* rARFrameBody =
    static_cast<ReAssociationRequestFrameBody*>(reAssociationRequest->decapsulate());

  WirelessEtherInterface iface = mod->findIfaceByMAC(MACAddress6(reAssociationRequest->getAddress2()));

  if((mod->isFrameForMe(static_cast<WirelessEtherBasicFrame*>(reAssociationRequest)))
     &&(mod->ssid == rARFrameBody->getSSID()))
  {
    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << mod->fullPath() << "\n"
         << " ----------------------------------------------- \n"
         << " Re-association Request received by: "
         << mod->macAddressString() << "\n"
         << " from " << reAssociationRequest->getAddress2() << " \n"
         << " SSID: " << rARFrameBody->getSSID() << "\n"
         << " ----------------------------------------------- \n");
    for ( size_t i = 0; i < rARFrameBody->getSupportedRatesArraySize(); i++ )
      Dout(dc::wireless_ethernet|flush_cf,
           " Supported Rates: \n "
           << " [" << i << "] \t rate: "
           << rARFrameBody->getSupportedRates(i).rate << " \n"
           << " \t supported?" << rARFrameBody->getSupportedRates(i).supported
           << "\n");

    // send ACK to confirm the transmission has been sucessfull
    WirelessEtherBasicFrame* ack = mod->
      createFrame(FT_CONTROL, ST_ACK,
                  MACAddress6(mod->macAddressString().c_str()),
                  reAssociationRequest->getAddress2());
    WESignalData* ackSignal = encapsulateIntoWESignalData(ack);
    sendAck(mod, ackSignal);
    //delete ack;
    changeState = false;

    WirelessEtherInterface iface = mod->findIfaceByMAC(MACAddress6(reAssociationRequest->getAddress2()));
    WESignalData* responseSignal;

    if(iface.receiveMode == RM_ASSOCIATION)
    {
      mod->addIface(reAssociationRequest->getAddress2(), RM_ASSRSP_ACKWAIT);
      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << mod->fullPath() << " " << mod->simTime()
         << " Setting iface: " << reAssociationRequest->getAddress2()
         << " RM: " << RM_ASSRSP_ACKWAIT);

      // TODO: check supported rates, etc we set sucessful for now
      mod->setIfaceStatus(reAssociationRequest->getAddress2(), SC_SUCCESSFUL);

      WirelessEtherBasicFrame* reAssResponse = mod->
        createFrame(FT_MANAGEMENT, ST_REASSOCIATIONRESPONSE,
                    MACAddress6(mod->macAddressString().c_str()),
                    reAssociationRequest->getAddress2());
      FrameBody* reAssResponseFrameBody = mod->createFrameBody(reAssResponse);
      reAssResponse->encapsulate(reAssResponseFrameBody);
      WESignalData *responseSignal = encapsulateIntoWESignalData(reAssResponse);
      //delete reAssResponse;
    }
    else // send deauthentication
    {
      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
        << mod->fullPath() <<" Sending De-authent to "
                << reAssociationRequest->getAddress2()
        << " Not in RM_ASSOCIATION (Re): "<< iface.receiveMode);
      WirelessEtherBasicFrame* deAuthentication = mod->
        createFrame(FT_MANAGEMENT, ST_DEAUTHENTICATION,
                    MACAddress6(mod->macAddressString().c_str()),
                    reAssociationRequest->getAddress2());
      FrameBody* deAuthFrameBody = mod->createFrameBody(deAuthentication);
      deAuthentication->encapsulate(deAuthFrameBody);
      responseSignal = encapsulateIntoWESignalData(deAuthentication);
      //delete deAuthentication;
    }
    mod->outputBuffer.push_back(responseSignal);
  }
  delete rARFrameBody;
}

void WEAPReceiveMode::handleProbeRequest(WirelessAccessPoint* mod, WESignalData* signal)
{
  WirelessEtherManagementFrame* probeRequest =
      static_cast<WirelessEtherManagementFrame*>(signal->encapsulatedMsg());

  ProbeRequestFrameBody* pRFrameBody =
    static_cast<ProbeRequestFrameBody*>(probeRequest->decapsulate());

  assert(pRFrameBody);

  // Check if frame is for me and ssid matches AP's ssid or ssid is
  // broadcast

  if( (mod->isFrameForMe(static_cast<WirelessEtherBasicFrame*>(probeRequest)))
      &&
      ((pRFrameBody->getSSID() == "") ||
       ( mod->ssid == pRFrameBody->getSSID())))
  {
    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << mod->fullPath() << "\n"
         << " ----------------------------------------------- \n"
         << " Probe Request received by: " << mod->macAddressString() << "\n"
         << " from " << probeRequest->getAddress2() << " \n"
         << " SSID: " << pRFrameBody->getSSID() << "\n"
         << " ----------------------------------------------- \n");
    for ( size_t i = 0; i < pRFrameBody->getSupportedRatesArraySize(); i++ )
      Dout(dc::wireless_ethernet|flush_cf,
           " Supported Rates: \n "
           << " [" << i << "] \t rate: "
           << pRFrameBody->getSupportedRates(i).rate << " \n"
           << " \t supported?" << pRFrameBody->getSupportedRates(i).supported
           << "\n");

    // TODO: check supported rates; we send probe response
    // regardlessly for now

    WirelessEtherBasicFrame* probeResponse =
      mod->createFrame(FT_MANAGEMENT, ST_PROBERESPONSE, mod->address,
                       MACAddress6(probeRequest->getAddress2()));
    FrameBody* responseFrameBody = mod->createFrameBody(probeResponse);
    probeResponse->encapsulate(responseFrameBody);
    WESignalData* responseSignal = encapsulateIntoWESignalData(probeResponse);
    mod->outputBuffer.push_back(responseSignal);
    //delete probeResponse;

    //static_cast<WirelessEtherStateReceive*>(mod->currentState())->
    //  changeNextState(mod);
  }

  delete pRFrameBody;
}

void WEAPReceiveMode::handleAck(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessAccessPoint* apMod = static_cast<WirelessAccessPoint*>(mod);

  WirelessEtherBasicFrame* ack =
      static_cast<WirelessEtherBasicFrame*>(signal->encapsulatedMsg());


   Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << apMod->fullPath()  << " "<< mod->simTime()
         << " ACK for " << MACAddress6(mod->macAddressString().c_str()) );

  // GD find latest outputBuffer entry and check whether we're
  // waiting for that...

  if(ack->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
  {


    mod->getTmrMessage(WIRELESS_SELF_AWAITACK)->cancel();
    WirelessEtherBasicFrame* outf =
      static_cast<WirelessEtherBasicFrame*>((*(apMod->outputBuffer.begin()))->encapsulatedMsg());


    WirelessEtherInterface iface = apMod->findIfaceByMAC(MACAddress6(outf->getAddress1()));

   Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << apMod->fullPath()  << " "<< mod->simTime()
         << " ACK from " << outf->getAddress1() );
    // GD Hack: assuming that the output queue holds Message for
    //          which we receive ack (no bad nodes, no acks lost??)
    if(iface.receiveMode == RM_AUTHRSP_ACKWAIT)
    {

      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << apMod->fullPath()  << " " << mod->simTime()
         << " Setting Iface  "<< iface.address
         << " RM: "<< iface.receiveMode
         << "->RM_ASSOCIATION ("<< RM_ASSOCIATION <<")");

      apMod->addIface(iface.address, RM_ASSOCIATION);
    }
    // GD Hack: assuming that the output queue holds Message for
    //          which we receive ack (no bad nodes, no acks lost??)
    if(iface.receiveMode == RM_ASSRSP_ACKWAIT)
    {

      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << apMod->fullPath()  << " " << mod->simTime()
         << " Setting iface  "<< iface.address
         << " RM: "<< iface.receiveMode
         << "->RM_DATA ("<< RM_DATA <<")");

      apMod->addIface(iface.address, RM_DATA);
    }

    finishFrameTx(mod);
    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << apMod->fullPath() << "\n"
         << " ----------------------------------------------- \n"
         << " ACK received by: " << apMod->macAddressString() << "\n"
         << " ----------------------------------------------- \n");
      changeState = false;
  }

  if ( mod->currentState() == WirelessEtherStateAwaitACKReceive::instance())
    changeState = false;
}

void WEAPReceiveMode::handleData(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessAccessPoint* apMod = static_cast<WirelessAccessPoint*>(mod);

  WirelessEtherDataFrame* data =
      static_cast<WirelessEtherDataFrame*>(signal->encapsulatedMsg());

  // Check that data frame is for this ap and the source is another ap
  if(
      (apMod->isFrameForMe(static_cast<WirelessEtherBasicFrame*>(data)))
      && ((data->getFrameControl().toDS == true)&&(data->getFrameControl().fromDS == false))
    )
  {
    WirelessEtherInterface iface = apMod->findIfaceByMAC(MACAddress6(data->getAddress2()));

    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
             << apMod->fullPath() << "\n"
             << " ----------------------------------------------- \n"
             << " Data packet received from: " << data->getAddress2()
             << "\n"
             << " ----------------------------------------------- \n");

    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << apMod->fullPath() << "\n"
         << " ----------------------------------------------- \n"
         << " Packet from WirelessEthernet card: \n"
         << " Address 1 - " << data->getAddress1()
         << " \n Address 2 - " << data->getAddress2()
         << " \n Address 3 - " << data->getAddress3()
         << " \n" << data << "\n"
         << " ----------------------------------------------- \n");

    // send ACK to confirm the transmission has been sucessful
    WirelessEtherBasicFrame* ack = apMod->
      createFrame(FT_CONTROL, ST_ACK,
                  MACAddress6(apMod->macAddressString().c_str()),
                  data->getAddress2());
    WESignalData* ackSignal = encapsulateIntoWESignalData(ack);
    sendAck(apMod, ackSignal);
    changeState = false;
    //delete ack;

    // check if SA is associated and redirect data to actual DA
    WirelessEtherInterface chkIface =
      apMod->findIfaceByMAC(data->getAddress2());

    // check if MS is authenticated
    if ((chkIface == UNSPECIFIED_WIRELESS_ETH_IFACE)||(chkIface.receiveMode == RM_AUTHRSP_ACKWAIT))
    {
      // send deauthentication
      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
        << mod->fullPath() <<" Sending De-authent to "
                << data->getAddress2()
        << " in RM_AUTHRSP_ACKWAIT ");

      WirelessEtherBasicFrame* deAuthentication = mod->
          createFrame(FT_MANAGEMENT, ST_DEAUTHENTICATION,
                    MACAddress6(mod->macAddressString().c_str()),
                    data->getAddress2());
      FrameBody* deAuthFrameBody = mod->createFrameBody(deAuthentication);
      deAuthentication->encapsulate(deAuthFrameBody);
      WESignalData* deAuthSignal = encapsulateIntoWESignalData(deAuthentication);
      apMod->outputBuffer.push_back(deAuthSignal);
      //delete deAuthentication;
    }
    // check if MS is associated
    else if((chkIface.receiveMode == RM_ASSOCIATION)||(chkIface.receiveMode == RM_ASSRSP_ACKWAIT))
    {
      // send disassociation if not associated
      WirelessEtherBasicFrame* disAssociation = mod->
          createFrame(FT_MANAGEMENT, ST_DISASSOCIATION,
                    MACAddress6(mod->macAddressString().c_str()),
                    data->getAddress2());
      FrameBody* disAssFrameBody = mod->createFrameBody(disAssociation);
      disAssociation->encapsulate(disAssFrameBody);
      WESignalData* disAssSignal = encapsulateIntoWESignalData(disAssociation);
      apMod->outputBuffer.push_back(disAssSignal);
      //delete disAssociation;
    }
    // MS is permitted to send class 3 frames
    else
    {
            apMod->usedBW.sampleTotal += data->length();
      // renew expiry time for entry
            apMod->addIface(data->getAddress2(), RM_DATA);
      // create data frame to be forwarded
      WirelessEtherBasicFrame* sendFrame = apMod->
         createFrame(FT_DATA, ST_DATA, data->getAddress2(), data->getAddress3());
      sendFrame->encapsulate(data->decapsulate());

    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << apMod->fullPath() << "\n"
         << " ----------------------------------------------- \n"
         << " Packet to Bridge: \n"
         << " Address 1 - " << static_cast<WirelessEtherDataFrame*>(sendFrame)->getAddress1()
         << " \n Address 2 - " << static_cast<WirelessEtherDataFrame*>(sendFrame)->getAddress2()
         << " \n Address 3 - " << static_cast<WirelessEtherDataFrame*>(sendFrame)->getAddress3()
         << "\n ----------------------------------------------- \n");

      // forward packet to both wired and wireless LAN if the destination is a broadcast address
      if ( sendFrame->getAddress1() == MACAddress6(WE_BROADCAST_ADDRESS))
      {
          // sendFrame is used by the bridge, so cant delete it.
          apMod->send(sendFrame, apMod->inputQueueOutGate());
          WESignalData* sendFrameSignal = encapsulateIntoWESignalData((cMessage*)sendFrame->dup());
          apMod->outputBuffer.push_back(sendFrameSignal);
          Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
             << apMod->fullPath() << "\n"
             << " ----------------------------------------------- \n"
             << " Packet forwarded to the MAC: " << sendFrame->getAddress1()
             << " \n and forwarded to the bridge \n"
             << " \n"
             << " ----------------------------------------------- \n");
      }
      else
      {
        // check if DA is associated
        WirelessEtherInterface chkDestIface =
            apMod->findIfaceByMAC(data->getAddress3());

        if ( chkDestIface == UNSPECIFIED_WIRELESS_ETH_IFACE )
          {
          // forward to bridge
          // sendFrame is used by the bridge, so cant delete it.
          apMod->send(sendFrame, apMod->inputQueueOutGate());
          Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
               << apMod->fullPath() << "\n"
               << " ----------------------------------------------- \n"
               << " Packet forwarded to the bridge \n"
               << " ----------------------------------------------- \n");
          }
          else
          {
          // renew timeout value for destination, since it is
          // receiving data
                    apMod->addIface(data->getAddress3(), RM_DATA);
          WESignalData* sendFrameSignal = encapsulateIntoWESignalData(sendFrame);
          apMod->outputBuffer.push_back(sendFrameSignal);
          Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
               << apMod->fullPath() << "\n"
               << " ----------------------------------------------- \n"
               << " Packet forwarded to the MAC: " << sendFrame->getAddress1() << " \n"
               << " ----------------------------------------------- \n");
          //delete sendFrame;
        }
      }
    }
  }
}
