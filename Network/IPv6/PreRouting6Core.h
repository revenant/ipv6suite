// -*- C++ -*-
//
// Copyright (C) 2001, 2003 CTIE, Monash University
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
    @file PreRouting6Core.h
    @brief Header file for PreRouting Module
    @author Johnny Lai
    @date 27/08/01
*/

#ifndef PREROUTING6CORE_H
#define PREROUTING6CORE_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

class IPv6ForwardCore;

/**
   @class PreRouting6Core
   @brief IPv6 implementation of PreRouting.  Unit Tests actually run here
 */
class PreRouting6Core: public cSimpleModule
{
public:
  Module_Class_Members(PreRouting6Core, cSimpleModule, 0);

  virtual void initialize();
  virtual void finish();
  virtual void handleMessage(cMessage* theMsg);
private:
  simtime_t delay;
  bool hasHook;
  unsigned int ctrIP6InReceive;
  cMessage* waitTmr;
  cMessage* curPacket;
  ///Arriving packets are placed in queue first if another packet is awaiting
  ///processing
  cQueue waitQueue;
  ::IPv6ForwardCore* forwardMod;

};

#endif //PREROUTING6CORE_H
