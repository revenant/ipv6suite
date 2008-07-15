// -*- C++ -*-
// Copyright (C) 2006 Johnny Lai
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
 * @file MobilityPlayback.h
 * @author Johnny Lai
 * @date 05 Jun 2006
 *
 * @brief Definition of class MobilityPlayback
 *
 * 
 *
 */

#ifndef MOBILITYPLAYBACK_H
#define MOBILITYPLAYBACK_H

#ifndef MOBILITY_HANDLER_H
#include "MobilityHandler.h"
#endif

#include <fstream>
#include <map>
#include <list>

/**
 * @class MobilityPlayback
 *
 * @brief Play back moves recorded in Entity::setPosition
 *
 * Forces node to move according to a file. The file is saved by
 * Entity::setPosition. This is necessary because if we modify the number of
 * network entities the moves are different due to the rng sequences. We can
 * debug simpler scenarios involving the same walk sequence. Also helps to
 * visualise movement of many nodes
 */

class MobilityPlayback: public MobilityHandler
{
 public:

  struct Move
  {
    simtime_t time;
    double x;
    double y;
  };

#ifdef USE_CPPUNIT
  friend class MobilityPlaybackTest;
#endif //USE_CPPUNIT

  Module_Class_Members(MobilityPlayback, MobilityHandler, 0);

  ///@name Overidden cSimpleModule functions
  //@{
  virtual int  numInitStages() const  {return 3;}

  virtual void initialize(int stage);

  virtual void finish();

  virtual void handleMessage(cMessage* msg);
  //@}

 protected:

 private:
  static std::ifstream f;
  static std::map<MobileEntity*, std::list<Move> > moves;
  unsigned int moveIndex;
};


#endif /* MOBILITYPLAYBACK_H */

