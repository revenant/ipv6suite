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
    @file EtherModuleBridge.h
    @brief Definition file for EtherModuleBridge

    simple implementation of ethernet interface in Bridge

    @author Eric Wu
*/

#ifndef __ETHERMODULE_BRIDGE__
#define __ETHERMODULE_BRIDGE__

#include <omnetpp.h>
#include "EtherModuleAP.h"

class EtherModuleBridge : public EtherModuleAP
{
 public:
  Module_Class_Members(EtherModuleBridge, EtherModuleAP, 0);

  virtual void initialize(int stage);
  virtual void handleMessage(cMessage* msg);
  virtual void finish(void);

  virtual int numInitStages(void) const { return 2; }

  // frames from bridge module
  virtual bool receiveData(std::auto_ptr<cMessage> msg);

  // send packet to other layer besides physical layer
  virtual bool sendData(EtherFrame6* frame);
};

#endif // __ETHERMODULE_BRIDGE__
