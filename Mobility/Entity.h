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
   @file Entity.h

   A generic class for both wired and wireless network entites

   Author Eric Wu
 */

#ifndef __ENTITY__
#define __ENTITY__

#include <string>

enum MobileEntityType
{
  MobileNONE, // none mobile element
  MobileBS, // BaseStation
  MobileMN // Mobile Node
};

struct Position
{
  double x; double y;
};

extern bool operator==(Position& lhs, Position& rhs);

class WorldProcessor;
class cSimpleModule;

class Entity
{
 public:
  // OMNeT++ functions
  Entity(cSimpleModule* mod);
  virtual ~Entity();

  // return the position of the entity
  Position position(void) { return _pos; }
  
  void setPosition(double x, double y);

  // return the type of entity
  MobileEntityType entityType(void) { return _type; }

  // return the distance between the entities
  int distance(Entity* entity);

  // return the container module
  cSimpleModule* containerModule(void) { return _mod; }

  std::string entityName;

  ///Draw wireless range of interface
  ///@note assuming receive threshold on all nodes are the same
  virtual void drawWirelessRange();

  // obtain position info from display string of the netnode module
  void getDispPosition(double& x, double& y);

  // update the position info to display string of the netnode module
  void setDispPosition(double x, double y);

 protected:
  cSimpleModule* _mod;
  Position _pos;
  MobileEntityType _type;
  WorldProcessor* _mwp;
  std::string simpleName;
};

#endif // __ENTITY__
