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
#include "sys.h"
#include "debug.h"
#include "opp_utils.h"
#include "MobilityRandomWP.h"
#include "WorldProcessor.h"
#include "XML/XMLOmnetParser.h"
#include "MobileEntity.h"
#include "randomWP.h"

Define_Module_Like( MobilityRandomWP, MobilityHandler );

// const int TMR_WIRELESSMOVE = 3111;

void MobilityRandomWP::initialize(int stage)
{
  MobilityHandler::initialize(stage);

  if ( stage == 1 )
  {
    cXMLElement* moveInfo = par("moveXmlConfig");
    if (moveInfo)
    {
       XMLConfiguration::XMLOmnetParser p;
      //default.ini loads empty.xml at element netconf
      if (p.getNodeProperties(moveInfo, "debugChannel", false) == "")
        p.parseRandomWPInfoDetail(this, moveInfo);
      else
        Dout(dc::xml_addresses, " no global "<<className()<<" move info for node "<<OPP_Global::nodeName(this));
    }

    wproc->xmlConfig()->parseRandomWPInfo(this);

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
    randomWP->generateMovements();

    double x = mobileEntity->position().x;
    double y = mobileEntity->position().y;

//    double time = randomWP->randomWaypoint(x, y);

    selfMovingNotifier = new cMessage("move");
    selfMovingNotifier->setKind(TMR_WIRELESSMOVE);
    selfMovingNotifier->addPar("x") = x;
    selfMovingNotifier->addPar("y") = y;
    scheduleAt(simTime() + startTime, selfMovingNotifier);
  }
}

void MobilityRandomWP::handleMessage(cMessage* msg)
{
  mobileEntity->setPosition(msg->par("x"),  msg->par("y"));

  // update the display string of the net node module
  mobileEntity->setDispPosition(mobileEntity->position().x,
                                  mobileEntity->position().y);

  double x = mobileEntity->position().x;
  double y = mobileEntity->position().y;

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
