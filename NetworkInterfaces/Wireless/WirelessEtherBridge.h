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
	@file WirelessEtherBridge.h
	@brief Header file for WirelessEtherBridge
	
    simple implementation of bridge for access point

	@author Eric Wu
*/

#ifndef __WIRELESS_ETHERBRIDGE__
#define __WIRELESS_ETHERBRIDGE__

#include <map>

#include <omnetpp.h>
#include "WirelessEtherModule.h"
#include "MACAddress.h"

class LinkLayerModule;

// the key is the input port index and the value is the protocol information
// to the corresponding MAC address
typedef std::map<LinkLayerModule*, int> MACPortMap;

class WirelessEtherBridge : public cSimpleModule
{
 public:
  Module_Class_Members(WirelessEtherBridge, cSimpleModule, 0);
  
  virtual void initialize(int stage);
  virtual void handleMessage(cMessage* msg);
  virtual void finish(void);
  virtual int  numInitStages() const  {return 2;}

  const unsigned int* macAddress(void);
  std::string macAddressString(void);

 private:
  int getOutputPort(LinkLayerModule* llmod);
  LinkLayerModule* findMacByAddress(std::string addr);
  cMessage* translateFrame(cPacket* frame, int destProt);
  
 protected:
  MACAddress address;
  MACPortMap macPortMap;
};

#endif // __WIRELESS_ETHERBRIDGE__
