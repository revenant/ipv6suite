// -*- C++ -*-
// Copyright (C) 2001, 2004 Monash University, Melbourne, Australia
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
	@file MobilityHandler.h
	@brief Header file for MobilityHandler.h

	An 'abstract' class for all network interface classes

	@author Eric Wu
*/

#ifndef MOBILITY_HANDLER_H
#define MOBILITY_HANDLER_H

#include <omnetpp.h>

// timer message ID
extern const int TMR_WIRELESSMOVE;

namespace XMLConfiguration
{
  class IPv6XMLParser;
  class IPv6XMLWrapManager;
  class XMLOmnetParser;
}

class MobileEntity;
class WorldProcessor;

class MobilityHandler : public cSimpleModule
{
 public:
  Module_Class_Members(MobilityHandler, cSimpleModule, 0);
  virtual void initialize(int stage);  
  virtual void handleMessage(cMessage* msg);
  virtual void finish();

  MobileEntity* getEntity() const { return mobileEntity; }

  virtual int  numInitStages() const  {return 2;}

 protected:
  void initiateMoveScheduler(void);

  MobileEntity* mobileEntity;
  simtime_t elapsedTime;
  cMessage* selfMovingNotifier;

  WorldProcessor* wproc;

};

#endif
