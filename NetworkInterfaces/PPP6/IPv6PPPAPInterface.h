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
 * @file IPv6PPPAPInterface.h
 * @author Johnny Lai
 * @date 22 Jul 2004
 *
 * @brief Definition of class IPv6PPPAPInterface
 *
 */

#ifndef IPV6PPPAPINTERFACE_H
#define IPV6PPPAPINTERFACE_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

#ifndef IPV6PPPINTERFACE_H
#include "IPv6PPPInterface.h"
#endif

/**
 * @class IPv6PPPAPInterface
 *
 * @brief
 *
 * detailed description
 */

class IPv6PPPAPInterface: public IPv6PPPInterface
{
public:
  friend class WirelessEtherBridge;

  Module_Class_Members(IPv6PPPAPInterface, IPv6PPPInterface, 0);

  ///@name Overidden cSimpleModule functions
  //@{

  virtual int numInitStages() const;

  virtual void initialize(int stageNo);

  virtual void finish();

  virtual void handleMessage(cMessage* msg);
  //@}

  // frames from bridge module
  virtual PPP6Frame* receiveFromUpperLayer(cMessage* msg);

  // send packet to upper layer
  virtual void sendToUpperLayar(PPP6Frame* frame);

  // output gate of the Input Queue in upper layer
  int inputQueueOutGate() const;
protected:


private:
};


#endif /* IPV6PPPAPINTERFACE_H */

