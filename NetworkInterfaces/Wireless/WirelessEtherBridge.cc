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
	@file WirelessEtherBridge.cc
	@brief Definition file for WirelessEtherBridge

	simple implementation of Wireless bridge module

	Bridges data frames from Wired ethernet to Wireless ethernet, and vice
  versa.

	@author Eric Wu
*/

#include "sys.h"
#include "debug.h"

#include <cmath>
#include <boost/random.hpp>

#include <string>



#include "WirelessEtherBridge.h"
#include "opp_utils.h"

#include "EtherFrame.h"
#include "WirelessEtherFrame_m.h"
#include "LinkLayerModule.h"
#include "EtherModuleAP.h"
#include "WirelessAccessPoint.h"
#include "WirelessEtherSignal.h"
#include "IPv6PPPAPInterface.h"
#include "PPPFrame.h"

Define_Module(WirelessEtherBridge);

void WirelessEtherBridge::initialize(int stage)
{
  if (stage == 0)
  {
    MAC_address addr;

    addr.high = OPP_Global::generateInterfaceId() & 0xFFFFFF;
    addr.low = OPP_Global::generateInterfaceId() & 0xFFFFFF;

    address.set(addr);
  }
  else if (stage == 1)
  {
    cMessage* macNotifier = new cMessage("WIRELESS_AP_NOTIFY_MAC");
    macNotifier->setKind(MK_PACKET);
    cPar* mac = new cPar("MAC_ADDRESS");
    mac->setStringValue(address.stringValue());
    macNotifier->addPar(mac);
    send(static_cast<cMessage*>(macNotifier->dup()), "apOut");

    //int numOfDSs = OPP_Global::findNetNodeModule(this)->gateSize("in");
    // gateSize() is an omnetpp version 3.0 function
    int numOfDSs = parentModule()->gate("in")->size();

    for ( int i = 0; i < numOfDSs; i++)
      send(static_cast<cMessage*>(macNotifier->dup()), "dsOut", i);

    delete macNotifier;
  }
}

void WirelessEtherBridge::handleMessage(cMessage* msg)
{
  LinkLayerModule* llmod = dynamic_cast<LinkLayerModule*>(simulation.module(msg->senderModuleId()));
  assert(llmod);

  if ( std::string(msg->name()) == "PROTOCOL_NOTIFIER")
  {
    macPortMap.insert( MACPortMap::value_type(llmod, msg->arrivalGateId()));
  }
  else
  {
    switch(llmod->getInterfaceType())
    {
      case PR_WETHERNET:
      {
        // only one WirelessAccessPoint module is allowed in the bridge; DS module
        // cannot be hooked with another wireless Ethernet module
        assert(std::string(msg->arrivalGate()->name()) == "apIn");

        WirelessAccessPoint* macMod = dynamic_cast<WirelessAccessPoint*>(llmod);
        assert(macMod);

        WirelessEtherBasicFrame* frame = dynamic_cast<WirelessEtherBasicFrame*>(msg);
        assert(frame);

        MACPortMap::iterator it;
        for ( it = macPortMap.begin(); it != macPortMap.end(); it++ )
        {
          if (it->first->getInterfaceType() != PR_WETHERNET)
          {
            cMessage* destMessage = translateFrame(frame, it->first->getInterfaceType());
            if (destMessage)
              send(destMessage, getOutputPort(it->first));
          }
        }
      }
      break;

      case PR_ETHERNET:
      {
        EtherModuleAP* macMod = polymorphic_downcast<EtherModuleAP*>(llmod);
        assert(macMod);

        EtherFrame* frame = polymorphic_downcast<EtherFrame*>(msg);
        assert(frame);

        LinkLayerModule* destMod = findMacByAddress(frame->destAddrString());

        if (destMod || std::string(frame->destAddrString()) == ETH_BROADCAST_ADDRESS)
        {
          // send to wireless access point
          cMessage* destMessage = translateFrame(frame, PR_WETHERNET);
          send(destMessage, "apOut");

          if ( destMod && std::string(frame->destAddrString()) != ETH_BROADCAST_ADDRESS )
            macMod->addMacEntry(std::string(frame->srcAddrString()));
        }
      }
      break;

      case PR_PPP:
      {

        IPv6PPPAPInterface* macMod = polymorphic_downcast<IPv6PPPAPInterface*>(llmod);
        assert(macMod);

        PPPFrame* frame = polymorphic_downcast<PPPFrame*>(msg);
        LinkLayerModule* destMod = findMacByAddress(frame->destAddr);

        if (destMod || frame->destAddr == ETH_BROADCAST_ADDRESS)
        {
          // send to wireless access point
          cMessage* destMessage = translateFrame(frame, PR_WETHERNET);
          send(destMessage, "apOut");

//          if ( destMod && frame->destAddr != ETH_BROADCAST_ADDRESS )
//            macMod->addMacEntry(frame->srcAddr);
        }


      }
      break;
    }
  }

  delete msg;
}

void WirelessEtherBridge::finish(void)
{}

LinkLayerModule* WirelessEtherBridge::findMacByAddress(std::string addr)
{
  MACPortMap::iterator it;

  for ( it = macPortMap.begin(); it != macPortMap.end(); it++ )
  {
    switch(it->first->getInterfaceType())
    {
      case PR_WETHERNET:
      {
        WirelessAccessPoint* macMod = dynamic_cast<WirelessAccessPoint*>(it->first);
        assert(macMod);

        if ( macMod->findIfaceByMAC(MACAddress(addr.c_str())) != UNSPECIFIED_WIRELESS_ETH_IFACE )
          return macMod;
      }
      break;

/*      case PR_ETHERNET:
      {
        EtherModuleAP* macMod = dynamic_cast<EtherModuleAP*>(it->first);
        assert(macMod);

        EtherModuleAP::NeighbourMacList::iterator nit;

        for ( nit = macMod->ngbrMacList.begin(); nit != macMod->ngbrMacList.end(); nit++)
        {
          if ( *nit == addr )
            return macMod;
        }
      }
      break;*/
    }
  }
  return 0;
}

int WirelessEtherBridge::getOutputPort(LinkLayerModule* llmod)
{
  MACPortMap::iterator it = macPortMap.find(llmod);

  if ( it == macPortMap.end() )
    return -1;

  // There is often a pair of gates between the modules (input/output).
  // The output gate id is 1+ of the input gate id.
  return it->second + 1;
}

cMessage* WirelessEtherBridge::translateFrame(cPacket* frame, int destProtocol)
{
  cMessage* signal = 0;

  switch(frame->protocol())
  {
    case PR_WETHERNET:
    {
      WirelessEtherDataFrame* srcFrame = static_cast<WirelessEtherDataFrame*>(frame);
      std::string srcAddr(srcFrame->getAddress3());
      std::string destAddr(srcFrame->getAddress1());

      switch(destProtocol)
      {
        case PR_ETHERNET:
        {
          EtherFrame* destFrame = new EtherFrame;
          destFrame->setSrcAddress(MACAddress(srcAddr.c_str()));
          destFrame->setDestAddress(MACAddress(destAddr.c_str()));
          destFrame->setProtocol(PR_ETHERNET);

          cMessage* data = srcFrame->decapsulate();

          cPacket* dupData = static_cast<cPacket*>(data->dup());

          destFrame->encapsulate(dupData);
          destFrame->setName(dupData->name());

          delete data;

          signal = new EtherSignalData(destFrame);
          signal->setName(destFrame->name());

          Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
               << fullPath() << " \n"
               << " ---------------------------------------------------- \n"
               << " Packet forward from WirelessEthernet to Ethernet: \n"
               << " Dest MAC: " << destAddr.c_str() <<"\n"
               << " ---------------------------------------------------- \n");

          ///@warning Dodgy WESignalData dups frames in ctor
          delete destFrame;
        }
        break;
        case PR_PPP:
        {
          signal = new PPPFrame;
          cMessage* data = srcFrame->decapsulate();
          cPacket* dupData = static_cast<cPacket*>(data->dup());
          signal->encapsulate(dupData);
          signal->setName(dupData->name());
          delete data;
          Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
               << fullPath() << " \n"
               << " Packet forward from WirelessEthernet to PPP: \n");
        }
        break;
        default:
          assert(false);
          break;
      }
    }
    break;

    case PR_ETHERNET:
    {
      EtherFrame* srcFrame = static_cast<EtherFrame*>(frame);
      std::string srcAddr(srcFrame->srcAddrString());
      std::string destAddr(srcFrame->destAddrString());

      switch(destProtocol)
      {
        case PR_WETHERNET:
        {
          WirelessEtherDataFrame* destFrame = new WirelessEtherDataFrame;
          destFrame->setAddress1(MACAddress(destAddr.c_str())); // dest addr
          destFrame->setAddress2(MACAddress(address)); // ap addr
          destFrame->setAddress3(MACAddress(srcAddr.c_str())); // src addr
          destFrame->getFrameControl().protocolVer = 0;
          destFrame->getFrameControl().type = FT_DATA;
          destFrame->getFrameControl().subtype = ST_DATA;
          destFrame->getFrameControl().toDS = false;
          destFrame->getFrameControl().fromDS = true;
          destFrame->getFrameControl().retry = false;
          destFrame->setLength(FL_FRAMECTRL + FL_DURATIONID + FL_ADDR1 +
                               FL_ADDR2 +  FL_ADDR3 + FL_ADDR4 + FL_SEQCTRL +
                               FL_FCS);
          destFrame->setProtocol(PR_WETHERNET);

          cMessage* data = srcFrame->decapsulate();
          cPacket* dupData = static_cast<cPacket*>(data->dup());
          destFrame->encapsulate(dupData);
          destFrame->setName(dupData->name());

          delete data;

          signal = new WESignalData(destFrame);
          signal->setName(destFrame->name());

          Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
               << fullPath() << " \n"
               << " ---------------------------------------------------- \n"
               << " Packet forward from Ethernet to Wireless Ethernet: \n"
               << " Address1 (DEST): " <<destFrame->getAddress1() <<"\n"
               << " Address2 (AP): " <<destFrame->getAddress2() <<"\n"
               << " Address3 (SRC): " <<destFrame->getAddress3() <<"\n"
               << " ---------------------------------------------------- \n");

          delete destFrame;
        }
        break;

        default:
          assert(false);
        break;
      }
    }
    break;

    case PR_PPP:
    {
      PPPFrame* srcFrame = static_cast<PPPFrame*>(frame);

      switch(destProtocol)
      {
        case PR_WETHERNET:
        {
          WirelessEtherDataFrame* destFrame = new WirelessEtherDataFrame;
          destFrame->setAddress1(MACAddress(srcFrame->destAddr.c_str())); // dest addr
          destFrame->setAddress2(MACAddress(address)); // ap addr
          destFrame->getFrameControl().protocolVer = 0;
          destFrame->getFrameControl().type = FT_DATA;
          destFrame->getFrameControl().subtype = ST_DATA;
          destFrame->getFrameControl().toDS = false;
          destFrame->getFrameControl().fromDS = true;
          destFrame->getFrameControl().retry = false;
          destFrame->setLength(FL_FRAMECTRL + FL_DURATIONID + FL_ADDR1 +
                               FL_ADDR2 +  FL_ADDR3 + FL_ADDR4 + FL_SEQCTRL +
                               FL_FCS);
          destFrame->setProtocol(PR_WETHERNET);

          cMessage* data = srcFrame->decapsulate();
          destFrame->encapsulate(static_cast<cPacket*>(data->dup()));
          delete data;

          signal = new WESignalData(destFrame);
          signal->setName(destFrame->name());

          Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
               << fullPath() << " \n"
               << " ---------------------------------------------------- \n"
               << " Packet forward from PPP to Wireless Ethernet: \n"
               << " Address1 (DEST): " <<destFrame->getAddress1() <<"\n"
               << " Address2 (AP): " <<destFrame->getAddress2() <<"\n"
               << " No Address3 (SRC) as PPP : "
               << " ---------------------------------------------------- \n");

          delete destFrame;
        }
        break;
        default:
          assert(false);
          break;
      }
      break;
    }
    break;
    default:
      assert(false);
      break;
  }

  cMessage* internalNotifier = 0;

  if (signal)
  {
    internalNotifier = new cMessage;
    internalNotifier->setKind(MK_PACKET);
    internalNotifier->encapsulate(signal);
  }

  return internalNotifier;
}
