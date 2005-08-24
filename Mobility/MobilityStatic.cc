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
    @file MobilityStatic.cc
    @brief Header file for MobilityStatic

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
#include "MobilityStatic.h"
#include "WorldProcessor.h"
#include "XML/XMLOmnetParser.h"
#include "MobileEntity.h"

Define_Module_Like( MobilityStatic, MobilityHandler );

void MobilityStatic::initialize(int stage)
{
  MobilityHandler::initialize(stage);

  if ( stage == 2 )
  {
    cXMLElement* moveInfo = par("moveXmlConfig");
    if (moveInfo)
    {
       XMLConfiguration::XMLOmnetParser p;
      //default.ini loads empty.xml at element netconf
      if (p.getNodeProperties(moveInfo, "debugChannel", false) == "")
        p.parseMovementInfoDetail(this, moveInfo);
      else
        Dout(dc::xml_addresses, " no global "<<className()<<" move info for node "<<OPP_Global::nodeName(this));
    }

    wproc->xmlConfig()->parseMovementInfo(this);
    if (mobileEntity->speed() && !selfMovingNotifier)
    {
      elapsedTime = (double)1 / mobileEntity->speed();
      selfMovingNotifier = new cMessage("move");
      selfMovingNotifier->setKind(TMR_WIRELESSMOVE);
      scheduleAt(mobileEntity->startMovingTime() + elapsedTime, selfMovingNotifier);
    }
  }
}

void MobilityStatic::handleMessage(cMessage* msg)
{
  MobilityHandler::handleMessage(msg);
}

void MobilityStatic::finish()
{}
