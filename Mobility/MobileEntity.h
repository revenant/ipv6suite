// -*- C++ -*-
//
// Copyright (C) 2001, 2002 CTIE, Monash University
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
   @file MobileEntity.h

   @brief Definition of MobileEntity

   Handles attachment and dettachment of links with neighbouring network
   elements. NOTE: When access points are nearby, the mobile entity will
   automatically maintain link with those particular BSs.

   @author Eric Wu
 */

#ifndef __MOBILEENTITY__
#define __MOBILEENTITY__

#include <string>
#include <omnetpp.h>
#include <list>
#include <vector>


#include "Entity.h"

using namespace std;

/* XXX apparently not used
extern const char* ME_OUT;
extern const char* ME_IN;

extern const int CONNECT;
extern const int DISCONNECT;
extern const char* CONNECT_MSG;

// Information about connection with other entity
struct MEConnInfo
{
  Entity* en;
  int outputGate; // outgoing gate to other entity
  int inputGate; // incoming gate from other entity
};
*/

// record of destionation and the speed per move
struct MoveInfo
{
  Position destPos;
  float speed;
};

/* XXX
bool operator==(MEConnInfo& lhs, MEConnInfo& rhs);
*/

class MobileEntity : public Entity
{
 public:
  typedef vector<MoveInfo> Moves;

  // OMNeT++ functions
  MobileEntity(cSimpleModule* mod);

/* XXX apparently obsolete -- removed  --AV
  // return index of the gate that is belonged to the
  // Entity::containerModule() if gate connection is successful
  // otherwise return -1
  int connectWith(Entity* otherEntity, bool isOutgoing);

  // delete all gates that are associated with gates at the other
  // module, return true if gate disconnection is successful
  bool disconnect(Entity* otherEntity);
*/

  // return true if the mobile entity moves
  bool moving(void);

  ///Draw wireless range of interface
  ///@note assuming receive threshold on all nodes are the same
  virtual void drawWirelessRange();

  // return the moving speed of the mobile entity
  float speed(void)
    {
      if ( moves.size() == 0)
        return 0;

      return moves[_currentMoveIdx].speed;
    }

  // return the start of the time of moving
  int startMovingTime(void) { return _startMovingTime; }

  // set the number of base station that the mobile entity can connect
  // to
  void setNumOfAllowedBS(size_t numOfBS) { _numOfAllowedBS = numOfBS; }

  void setStartMovingTime(int t)
    {
      _startMovingTime = t;
    }

  void addMove(int x, int y, float speed)
    {
      MoveInfo info = { { x,y }, speed };
      moves.push_back(info);
    }

 private:

  int _startMovingTime;
  size_t _currentMoveIdx;

    // number of base stations that the mobile entity can connect to;
  // initial value of this parameter is set to 1
  size_t _numOfAllowedBS;

  Moves moves;
};

#endif // __MOBILEENTITY__
