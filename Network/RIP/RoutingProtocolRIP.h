// -*- C++ -*-
//
// Copyright (C) 2003 Johnny Lai
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
 * @file RoutingProtocolRIP.h
 * @author Johnny Lai
 * @date 25 Jul 2003
 *
 * @brief Definition of class RoutingProtocolRIP
 *
 * @test see RoutingProtocolRIPTest
 *
 */

#ifndef ROUTINGPROTOCOLRIP_H
#define ROUTINGPROTOCOLRIP_H 1

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

/**
 * @class RoutingProtocolRIP
 *
 * @brief Implementation of RIP for IPv6 based on RFC2453 RIPv2
 *
 * detailed description
 */

class RoutingProtocolRIP: public cSimpleModule
{
 public:
  friend class RoutingProtocolRIPTest;

  Module_Class_Members(RoutingProtocolRIP, cSimpleModule, 0);

  ///@name Overidden cSimpleModule functions
  //@{
  int numInitStages() const;

  virtual void initialize(int stageNo);

  virtual void finish();

  virtual void handleMessage(cMessage* msg);
  //@}

 protected:

 private:

};

namespace RoutingProtocol
{
  const unsigned int RIPPort = 520;
}


#endif /* ROUTINGPROTOCOLRIP_H */

