// -*- C++ -*-
//
// Copyright (C) 2001, 2004 Johnny Lai
// Monash University, Melbourne, Australia
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
    @file: IPv6PPPInterface.h

    @brief Implements the PPP network transport by implementing the prototype
    NetworkInterface module
*/

#ifndef IPV6PPPINTERFACE_H
#define IPV6PPPINTERFACE_H

#include <omnetpp.h>

#include "LinkLayerModule.h"

class PPP6Frame;
class InterfaceEntry;

/**
 * PPP interface
 */
class IPv6PPPInterface: public LinkLayerModule
{
public:
  Module_Class_Members(IPv6PPPInterface, LinkLayerModule, 0);

  virtual void initialize();
  //XXX virtual void activity();  not used, to be removed ? --AV
  virtual void handleMessage(cMessage* theMsg);

  // adds interface entry into InterfaceTable
  InterfaceEntry *registerInterface();

/* XXX  ie->interfaceToken() is used instead --AV
* unsigned int lowInterfaceId();
* unsigned int highInterfaceId();
*/

protected:
  virtual PPP6Frame* receiveFromUpperLayer(cMessage* msg) const;
  virtual void sendToUpperLayer(PPP6Frame* frame);

protected:
/* XXX  ie->interfaceToken() is used instead --AV
* unsigned int interfaceID[2];
*/
  cMessage* waitTmr;
  cMessage* curMessage;
};

#endif
