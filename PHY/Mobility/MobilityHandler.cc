// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/PHY/Mobility/Attic/MobilityHandler.cc,v 1.2 2005/02/10 01:15:48 andras Exp $
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
	@file MobilityHandler.cc
	@brief Header file for MobilityHandler

	Responsibilities:
        - mobility handling

  Initially implemented in PHYWirelessModule (now obsolete and
  evolved into PHYSimple), but taken out since it makes more sense
  as a seperate entity rather than part of the physical layer

  @author Eric Wu, Steve Woon
*/

#include <cassert>

#include "MobilityHandler.h"
#include "opp_utils.h"
#include "WorldProcessor.h"
#include "MobileEntity.h"

const int TMR_WIRELESSMOVE = 3111;

void MobilityHandler::initialize(int stage)
{
  if ( stage == 0 )
  {
    selfMovingNotifier = 0;
    mobileEntity = 0;
  }
  else if ( stage == 1)
  {
    cModule* mod = OPP_Global::findModuleByName(this, "worldProcessor");
    assert(mod);

    wproc = static_cast<WorldProcessor*>(mod);
    mobileEntity = dynamic_cast<MobileEntity*>(wproc->registerEntity(fullPath(), MobileMN, this));
    assert(mobileEntity);
  }
}

void MobilityHandler::handleMessage(cMessage* msg)
{
  if (msg->isSelfMessage() && msg->kind() == TMR_WIRELESSMOVE)
  {
    if (mobileEntity->moving())
      scheduleAt(simTime() + elapsedTime, static_cast<cMessage*>(msg->dup()));
  }
  else
    assert(false);

  delete msg;
}

void MobilityHandler::finish()
{}

void MobilityHandler::initiateMoveScheduler(void)
{
  if (mobileEntity->speed() && !selfMovingNotifier)
  {
    elapsedTime = (double)1 / mobileEntity->speed();
    selfMovingNotifier = new cMessage("move");
    selfMovingNotifier->setKind(TMR_WIRELESSMOVE);
    scheduleAt(mobileEntity->startMovingTime() + elapsedTime, selfMovingNotifier);
  }
}
