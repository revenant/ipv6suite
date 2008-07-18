// -*- C++ -*-
//
// Copyright (C) 2001 CTIE, Monash University
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
   @file MobileBaseStation.h

   A class that handles BS operations such as assigning channels for
   mobile entities

   Author Eric Wu
 */

#ifndef __MOBILEBASESTATION__
#define __MOBILEBASESTATION__

#include <list>

#include <omnetpp.h>
#include "Entity.h"

using namespace std;

typedef list<Entity*> MNList;

class BaseStation : public Entity
{
 public:
  BaseStation(cSimpleModule* mod);

  // return the broadcast range
  int bcastRange(void) { return _bcastRange; }

  // return true if the entity is within the BS's broadcast range
  bool isInRange(Entity* entity) { return (_bcastRange > distance(entity)); }

  // return true if the number of links to the mobile entities reach
  // to its maximum number channels
  bool isFull(void) { return (_maxChannels == mnList.size()); }

  // attach MN to BS
  void attachBS(Entity* entity) { mnList.push_back(entity); }

  // dettach MN from BS
  void dettachBS(Entity* entity) { mnList.remove(entity); }

 private:
  int _bcastRange;
  size_t _maxChannels;
  MNList mnList;
};

#endif // __MOBILEBASESTATION__
