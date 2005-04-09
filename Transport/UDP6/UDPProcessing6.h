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



#ifndef UDPPROCESSING6_H
#define UDPPROCESSING6_H

#include <map>
#include <omnetpp.h>
#include "IPProtocolId_m.h"

class UDPAppInterfacePacket;

//Octets Src/Dest port(4) + length + checksum
extern const unsigned int UDP_HEADER_SIZE;

/**
 * @class UDPProcessing6
 *
 * @brief Simple module to multiplex packets from UDP apps and demultiplex them
 * from the network layer via UDP port no.
 *
 * @note Assuming applications listen on all interfaces/IP addresses as
 * boundPorts maps ports to gate ids
 */

class UDPProcessing6: public cSimpleModule
{
public:
  typedef unsigned short UDPPort;
  typedef int GateID;
  typedef std::map<UDPPort, GateID> UDPApplicationPorts;
  friend class UDPProcessing6Test;

  Module_Class_Members(UDPProcessing6, cSimpleModule, 0);

  ///@name Overidden cSimpleModule functions
  //@{
  virtual void initialize();

  virtual void finish();

  virtual void handleMessage(cMessage* msg);
  //@}


protected:

private:
  void processApplicationMsg(UDPAppInterfacePacket* appIntPkt);
  void processNetworkMsg(cMessage *pkt);
  bool bind(cMessage* msg);
  bool isBound(UDPPort p);

  UDPApplicationPorts boundPorts;
  unsigned int ctrUdpOutDgrams, ctrUdpInDgrams;
};


#endif /* UDPPROCESSING6_H */

