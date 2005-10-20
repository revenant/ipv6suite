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
   @file Entity.cc

   A generic class for both wired and wireless network entites

   Author Eric Wu
 */

#include "sys.h"
#include "debug.h"

#include <cmath>
#include <sstream>
#include <string>
#include "opp_utils.h"  // for int/double <==> string conversions
#include <boost/cast.hpp>

#include "Entity.h"
#include "WorldProcessor.h"
#include "opp_utils.h"
#include "LinkLayerModule.h"
#include "WirelessEtherModule.h"   //XXX FIXME remove dependency!!!


using namespace::OPP_Global;

bool operator==(Position& lhs, Position& rhs)
{
  return (lhs.x == rhs.x && lhs.y == rhs.y);
}

Entity::Entity(cSimpleModule* mod)
  : _mod(mod)
{
  _type = MobileNONE;

  getDispPosition(_pos.x, _pos.y);
}

Entity::~Entity()
{}

int Entity::distance(Entity* entity)
{
  double hypotenuse =
  // pythargoras rule
    sqrt( pow( (double)(_pos.x - entity->_pos.x ), (double)2 ) +
          pow( (double)(_pos.y - entity->_pos.y ), (double)2 ) );

  if (_pos.x ==  entity->_pos.x)
    hypotenuse = fabs( (double)(_pos.y - entity->_pos.y) );

  if (_pos.y == entity->_pos.y)
    hypotenuse = fabs( (double)(_pos.x - entity->_pos.x) );

  return (int)(hypotenuse);
}

void Entity::drawWirelessRange()
{
  cModule *nodemod = findNetNodeModule(_mod);
  cModule* mod = OPP_Global::findModuleByName(_mod, "wirelessAccessPoint");
  if(mod)
  {
    LinkLayerModule* llmodule = (LinkLayerModule*)(mod->submodule("networkInterface"));
    assert(llmodule->getInterfaceType() == PR_WETHERNET);

    WirelessEtherModule* wem = check_and_cast<WirelessEtherModule*>(llmodule);

    nodemod->displayString().setTagArg("r",0,OPP_Global::dtostr(wem->wirelessRange()).c_str());
    nodemod->displayString().setTagArg("r",2,"red");
  }
}

void Entity::getDispPosition(double& x, double& y)
{
  cModule *nodemod = findNetNodeModule(_mod);
  x = atod(nodemod->displayString().getTagArg("p",0));
  y = atod(nodemod->displayString().getTagArg("p",1));
}

void Entity::setDispPosition(double x, double y)
{
  cModule *nodemod = findNetNodeModule(_mod);
  nodemod->displayString().setTagArg("p", 0, OPP_Global::ltostr((int)x).c_str());
  nodemod->displayString().setTagArg("p", 1, OPP_Global::ltostr((int)y).c_str());

  drawWirelessRange(); // XXX FIXME move it out of here
}
