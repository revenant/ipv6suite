// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/ICMPv6Combine.cc,v 1.1 2005/02/09 06:15:57 andras Exp $
//
// Copyright (C) 2001 CTIE, Monash University 
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

/**
   @file ICMPv6Combine.cc
   @brief Combine ICMPv6Core and NeighbourDiscovery 
     -ICMP output messages into one gate   
     -errors and notifications into one gate
   @date 26.09.01
*/

#include <omnetpp.h>

#include <string>

#include "ICMPv6Combine.h"


using std::string;

Define_Module( ICMPv6Combine );

void ICMPv6Combine::handleMessage(cMessage* msg)
{
  static const string xcoreSendOut("xcoreSendOut");
  static const string xcoreErrorOut("xcoreErrorOut");
  static const string xndSendOut("xndSendOut");
  static const string xndErrorOut("xndErrorOut");
  static const string MLDSendIn("MLDsendIn");

  for (;;)
  {
    if (xcoreSendOut == msg->arrivalGate()->name() ||
        xndSendOut == msg->arrivalGate()->name() ||
        MLDSendIn == msg->arrivalGate()->name())
    {
      send(msg, "sendOut");
      break;
    }

    if (xcoreErrorOut == msg->arrivalGate()->name() ||
        xndErrorOut == msg->arrivalGate()->name())
    {
      send(msg, "errorOut");
      break;
    }
    
    ev << "Unknown incoming gate "<<msg->arrivalGate()->name();
    break;

  } //end forever loop emulating switch statement
}
