// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/PHY/Mobility/Attic/MobilityRandomWalk.cc,v 1.2 2005/02/10 01:15:48 andras Exp $
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
	@file MobilityRandomWalk.cc
	@brief Header file for MobilityRandomWalk

	Responsibilities:
        - mobility handling

  Initially implemented in PHYWirelessModule (now obsolete and
  evolved into PHYSimple), but taken out since it makes more sense
  as a seperate entity rather than part of the physical layer

  @author Eric Wu, Steve Woon
*/

#include <cassert>

#include "MobilityRandomWalk.h"
#include "WorldProcessor.h"
#include "MobileEntity.h"
#include "randomWalk.h"

Define_Module_Like( MobilityRandomWalk, MobilityHandler );

// const int TMR_WIRELESSMOVE = 3111;

void MobilityRandomWalk::initialize(int stage)
{
  MobilityHandler::initialize(stage);

  if ( stage == 1 )
  {
    wproc->parseRandomWPInfo(this);
    moveKind = true; // only allow rebounding movement for random walk

    randomWalk = new RandomWalk;
    randomWalk->moveInterval = moveInterval;
    randomWalk->minSpeed = minSpeed;
    randomWalk->maxSpeed  = maxSpeed;
    randomWalk->distance = distance;
    randomWalk->moveKind = moveKind;
    randomWalk->minX = minX;
    randomWalk->maxX = maxX;
    randomWalk->minY = minY;
    randomWalk->maxY = maxY;

    int x = mobileEntity->position().x;
    int y = mobileEntity->position().y;

    double time = randomWalk->randomWalk(x, y);

    selfMovingNotifier = new cMessage("move");
    selfMovingNotifier->setKind(TMR_WIRELESSMOVE);
    selfMovingNotifier->addPar("x") = x;
    selfMovingNotifier->addPar("y") = y;
    scheduleAt(simTime() + time, selfMovingNotifier);
  }
}

void MobilityRandomWalk::handleMessage(cMessage* msg)
{
  mobileEntity->setPosition(msg->par("x"),  msg->par("y"));

  // update the display string of the net node module
  mobileEntity->setDispPosition(mobileEntity->position().x,
                                mobileEntity->position().y);

  int x = mobileEntity->position().x;
  int y = mobileEntity->position().y;

  double time = randomWalk->randomWalk(x, y);

  delete msg;

  selfMovingNotifier = new cMessage;
  selfMovingNotifier->setKind(TMR_WIRELESSMOVE);
  selfMovingNotifier->addPar("x") = x;
  selfMovingNotifier->addPar("y") = y;

  scheduleAt(simTime() + time, selfMovingNotifier);
}

void MobilityRandomWalk::finish()
{}
