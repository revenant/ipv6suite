// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/PHY/Mobility/Attic/MobilityRandomWP.cc,v 1.2 2005/02/10 01:15:48 andras Exp $
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
	@file MobilityRandomWP.cc
	@brief Header file for MobilityRandomWP

	Responsibilities:
        - mobility handling

  Initially implemented in PHYWirelessModule (now obsolete and
  evolved into PHYSimple), but taken out since it makes more sense
  as a seperate entity rather than part of the physical layer

  @author Eric Wu, Steve Woon
*/

#include <cassert>

#include "MobilityRandomWP.h"
#include "WorldProcessor.h"
#include "MobileEntity.h"
#include "randomWP.h"

Define_Module_Like( MobilityRandomWP, MobilityHandler );

// const int TMR_WIRELESSMOVE = 3111;

void MobilityRandomWP::initialize(int stage)
{
  MobilityHandler::initialize(stage);

  if ( stage == 1 )
  {
    wproc->parseRandomWPInfo(this);

    randomWP = new RandomWP;
    randomWP->moveInterval = moveInterval;
    randomWP->minSpeed = minSpeed;
    randomWP->maxSpeed  = maxSpeed;
    randomWP->distance = distance;
    randomWP->minX = minX;
    randomWP->maxX = maxX;
    randomWP->minY = minY;
    randomWP->maxY = maxY;
    randomWP->pauseTime = pauseTime;

    int x = mobileEntity->position().x;
    int y = mobileEntity->position().y;

//    double time = randomWP->randomWaypoint(x, y);
    double time = 5; // dodgey! but we stick for now; we do this because we want to make sure that mn estbalishes connection with its ha

    selfMovingNotifier = new cMessage("move");
    selfMovingNotifier->setKind(TMR_WIRELESSMOVE);
    selfMovingNotifier->addPar("x") = x;
    selfMovingNotifier->addPar("y") = y;
    scheduleAt(simTime() + time, selfMovingNotifier);
  }
}

void MobilityRandomWP::handleMessage(cMessage* msg)
{
  mobileEntity->setPosition(msg->par("x"),  msg->par("y"));

  // update the display string of the net node module
  mobileEntity->setDispPosition(mobileEntity->position().x,
                                  mobileEntity->position().y);

  int x = mobileEntity->position().x;
  int y = mobileEntity->position().y;

  double time = randomWP->randomWaypoint(x, y);

  delete msg;

  selfMovingNotifier = new cMessage;
  selfMovingNotifier->setKind(TMR_WIRELESSMOVE);
  selfMovingNotifier->addPar("x") = x;
  selfMovingNotifier->addPar("y") = y;

  scheduleAt(simTime() + time, selfMovingNotifier);

}

void MobilityRandomWP::finish()
{}
