// -*- C++ -*-
// Copyright (C) 2006 
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file   MobilityPlayback.cc
 * @author 
 * @date   05 Jun 2006
 *
 * @brief  Implementation of MobilityPlayback
 *
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include "MobilityPlayback.h"
#include "MobileEntity.h"
#include "WorldProcessor.h"
#include "opp_utils.h" // abort_ipv6suite

#include <boost/cast.hpp>
#include <ios> //precision
#include <cfloat> //DBL_DIG (significant digits stored in double == simtime_t)

Define_Module_Like(MobilityPlayback, MobilityHandler);

std::ifstream MobilityPlayback::f;
std::map<MobileEntity*, std::list<MobilityPlayback::Move> > MobilityPlayback::moves;


void MobilityPlayback::initialize(int stage)
{
  MobilityHandler::initialize(stage);
  if (stage != 2)
    return;
  
  if (moves.empty())
  {
    f.open("walk.txt");
    if (!f)
      abort_ipv6suite();

    double x=0,y=0;
    std::string node;
    simtime_t time;
    static std::ofstream of("walkparse-compare.txt");
    of.precision(DBL_DIG);
    assert(sizeof(simtime_t) == sizeof(double));
    std::map<std::string, MobileEntity*> names;
    for (;;)
    {
      //previous f>>node>>space>>time ... does not work as space does not eat
      //spaces but actual character's besides spaces. prob. some ios manip flag
      //to tell space is also a valid char too

      f>>node;
      //need to read past eof before failure is indicated
      if (!f)
	break;
      f.ignore();
      f>>time;
      f.ignore();
      f>>x;
      f.ignore();
      f>>y;
      of<<node<<" "<<time<<" "<<x<<" "<<y<<std::endl;
      if (names.count(node) == 0)
      {
	MobileEntity* me = 
	  boost::polymorphic_downcast<MobileEntity*> (wproc->findEntityByNodeName(node));
	if (!me)
	{
	  std::cerr<<"Skipping walk playback of node "<<node
		   <<" as it does not exist in scenario"<<std::endl;
	  assert(false);
	  continue;
	}
	names[node] = me;
      }
      Move m = {time, x, y};
      moves[names[node] ].push_back(m);     
    }
    f.close();
  }

  if (selfMovingNotifier)
    return;
  selfMovingNotifier = new cMessage("PlaybackMove", TMR_WIRELESSMOVE);
  Move& m = moves[mobileEntity].front();
  scheduleAt(m.time, selfMovingNotifier);

  std::cout<<className()<<" for "<<OPP_Global::nodeName(this)<<" has "<<moves[mobileEntity].size()<<" steps\n";

}

void MobilityPlayback::finish()
{
}

void MobilityPlayback::handleMessage(cMessage* msg)
{
  Move m = moves[mobileEntity].front();
  moves[mobileEntity].pop_front();
  mobileEntity->setPosition(m.x,  m.y);

  // update the display string of the net node module
  mobileEntity->setDispPosition(mobileEntity->position().x,
				mobileEntity->position().y);

  if (moves[mobileEntity].empty())
  {
    delete selfMovingNotifier;
    selfMovingNotifier = 0;
    return;
  }
  m = moves[mobileEntity].front();
  scheduleAt(m.time, selfMovingNotifier);
}
