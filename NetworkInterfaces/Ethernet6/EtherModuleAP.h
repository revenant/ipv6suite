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
    @file EtherModuleAP.h
    @brief Header file for EtherModuleAP

    simple implementation of ethernet interface in AP

    @author Eric Wu
*/

// XXX what's this, and why is it called like this? --AV

#ifndef __ETHERMODULE_AP__
#define __ETHERMODULE_AP__

#include <omnetpp.h>
#include <list>
#include "EtherModule.h"

class MACAddress6;
class WirelessEtherBridge;

class EtherModuleAP : public EtherModule
{
  friend class WirelessEtherBridge;

 public:
  Module_Class_Members(EtherModuleAP, EtherModule, 0);

  virtual void initialize(int stage);
  virtual void handleMessage(cMessage* msg);
  virtual void finish();

  virtual int numInitStages() const { return 2; }

  // frames from bridge module
  virtual bool receiveData(std::auto_ptr<cMessage> msg);

  // send packet to other layer besides physical layer
  virtual bool sendData(EtherFrame6* frame);

 private:
  void addMacEntry(std::string addr);

 private:
  typedef std::list<std::string> NeighbourMacList;
  NeighbourMacList ngbrMacList;


};

#endif
