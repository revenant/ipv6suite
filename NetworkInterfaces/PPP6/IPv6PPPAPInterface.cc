// -*- C++ -*-
// Copyright (C) 2004 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file   IPv6PPPAPInterface.cc
 * @author Johnny Lai
 * @date   22 Jul 2004
 * 
 * @brief  Implementation of IPv6PPPAPInterface
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"    
#include "debug.h"

#include <boost/cast.hpp>

#include "IPv6PPPAPInterface.h"

#include "Messages.h"
#include "MACAddress.h"
#include "PPPFrame.h"

Define_Module_Like(IPv6PPPAPInterface, NetworkInterface);

int IPv6PPPAPInterface::numInitStages() const
{
  return 2;
}

///Deferring mac address generation. WirelessEtherBridge will do this for us
void IPv6PPPAPInterface::initialize(int stage)
{
  if (stage == 0)
  {
    LinkLayerModule::initialize();

    setIface_name(PR_PPP);
    iface_type = PR_PPP;

    waitTmr = new cMessage("IPv6PPPInterfaceWait");
    curMessage = 0;
  }
  else if (stage == 1)
  {
    interfaceID[0] = interfaceID[1] = 0;

    //AP bridge apecific setup
    cMessage* protocolNotifier = new cMessage("PROTOCOL_NOTIFIER");
    protocolNotifier->setKind(MK_PACKET);
    send(protocolNotifier, inputQueueOutGate());
  }
}

void IPv6PPPAPInterface::finish()
{
}

void IPv6PPPAPInterface::handleMessage(cMessage* msg)
{
  if (lowInterfaceId() == 0 && highInterfaceId() == 0)
  {
    // needed for AP bridge
    if ( std::string(msg->name()) == "WIRELESS_AP_NOTIFY_MAC")
    {
      MACAddress addrObj;
      addrObj.set(static_cast<cPar*>(msg->parList().get(0))->stringValue());
      MAC_address addr = static_cast<MAC_address>(addrObj);
      interfaceID[0] = addr.high;
      interfaceID[1] = addr.low;
    }

    delete msg;
    return;
  }
  IPv6PPPInterface::handleMessage(msg);
}

int IPv6PPPAPInterface::inputQueueOutGate() const
{ 
  return findGate("ipInputQueueOut"); 
}

  // frames from bridge module
PPPFrame* IPv6PPPAPInterface::receiveFromUpperLayer(cMessage* msg)
{
  PPPFrame* ret = boost::polymorphic_downcast<PPPFrame*>(msg->decapsulate());
  assert(ret);
  delete msg;
  return ret;  
}

  // send packet to upper layer
void IPv6PPPAPInterface::sendToUpperLayar(PPPFrame* frame)
{
  assert(frame);
  send(frame->dup(), inputQueueOutGate());  
}
