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
    @file MobilityRandomPattern.cc
    @brief Header file for MobilityRandomPattern

    Responsibilities:
        - mobility handling

  Initially implemented in PHYWirelessModule (now obsolete and
  evolved into PHYSimple), but taken out since it makes more sense
  as a seperate entity rather than part of the physical layer

  @author Eric Wu
*/

#include <cassert>

#include "MobilityRandomPattern.h"
#include "WorldProcessor.h"
#include "MobileEntity.h"
#include "SingletonRandomPattern.h"

Define_Module_Like( MobilityRandomPattern, MobilityHandler );

void MobilityRandomPattern::initialize(int stage)
{
  MobilityHandler::initialize(stage);

  if ( stage == 0 )
  {
    isRandomPatternParsed = false;
  }
  if ( stage == 1 )
  {
    RandomPattern* rp = RandomPattern::initializePattern();

    // RandomPattern instance has already be created
    if (rp->refCount > 1)
    {
      isRandomPatternParsed = true;

      moveInterval = rp->moveInterval;
      minSpeed = rp->minSpeed;
      maxSpeed = rp->maxSpeed;
      distance = rp->distance;
      xSize = rp->maxX;
      ySize = rp->maxY;
      pauseTime = rp->pauseTime;
    }

    // If the RandomPattern has already been created, we don't need to
    // parse the info to it anymore.
    wproc->parseRandomPatternInfo(this);

    if (rp->refCount == 1)
    {
      rp->moveInterval =  moveInterval;
      rp->minSpeed = minSpeed;
      rp->maxSpeed = maxSpeed;
      rp->distance = distance;
      rp->minX = 0;
      rp->minY = 0;
      rp->maxX = xSize;
      rp->maxY = ySize;
      rp->pauseTime = pauseTime;

      isRandomPatternParsed = true;
    }

//    double time = randomPattern->randomWaypoint(x, y);
    double time = 5; // dodgey! but we stick for now; we do this because we want to make sure that mn estbalishes connection with its ha

    selfMovingNotifier = new cMessage;
    selfMovingNotifier->setKind(TMR_WIRELESSMOVE);
    selfMovingNotifier->addPar("x") = 0;
    selfMovingNotifier->addPar("y") = 0;
    scheduleAt(simTime() + time, selfMovingNotifier);
  }
}

void MobilityRandomPattern::handleMessage(cMessage* msg)
{
  int x = msg->par("x");
  int y = msg->par("y");

  mobileEntity->setPosition(x + xOffset,
                            y + yOffset);

  // update the display string of the net node module
  mobileEntity->setDispPosition(mobileEntity->position().x,
                                  mobileEntity->position().y);

  double time = RandomPattern::instance()->wayPoint(x, y);

  delete msg;

  selfMovingNotifier = new cMessage("move");
  selfMovingNotifier->setKind(TMR_WIRELESSMOVE);
  selfMovingNotifier->addPar("x") = x;
  selfMovingNotifier->addPar("y") = y;

  scheduleAt(simTime() + time, selfMovingNotifier);
}

void MobilityRandomPattern::finish()
{}
