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

// XXX FIXME wrong: should be changed to displayString().setTagArg()!!!!!!!!!!!!!!!!!!!!!!!!!!! --AV
void Entity::drawWirelessRange(std::string& dispStr)
{
  if (dispStr.find("r=") == std::string::npos)
  {
      cModule* mod = OPP_Global::findModuleByName(_mod, "wirelessAccessPoint");
      if(mod)
      {
        LinkLayerModule* llmodule = (LinkLayerModule*)(mod->submodule("networkInterface"));
        assert(llmodule->getInterfaceType() == PR_WETHERNET);

        WirelessEtherModule* wem =
          boost::polymorphic_downcast<WirelessEtherModule*>(llmodule);
        assert(wem);

        dispStr += ";r=";
//XXX see comment above!   dispStr += boost::lexical_cast<std::string>(wem->wirelessRange());
        dispStr += ",,red";
      }
  }
}

void Entity::getDispPosition(int& x, int& y)
{
  std::string dispStr = static_cast<const char*> (findNetNodeModule(_mod)->displayString());
  drawWirelessRange(dispStr);
  findNetNodeModule(_mod)->setDisplayString(dispStr.c_str());

  cDisplayStringParser disParser(dispStr.c_str());

  // Tcl/Tk environment, simply use Tcl/Tk object co-ordinates for
  // entity terrain position
  if(disParser.existsTag("p"))
  {
    const char* xStr = disParser.getTagArg("p", 0);
    const char* yStr = disParser.getTagArg("p", 1);

    x = atoi(xStr);
    y = atoi(yStr);
  }
  // command line environment, TODO: may use psuedo random number
  // generator to generate terrain position
  else
  {
  }
}

void Entity::setDispPosition(int x, int y)
{
  cDisplayStringParser disParser(findNetNodeModule(_mod)->displayString());

  // update Tk/Tcl environment
  if(disParser.existsTag("p"))
  {
    std::stringstream ss_x;
    std::stringstream ss_y;

    ss_x << x;
    ss_y << y;

    // TODO: some problem with the DisplayStringParser.. so mannually
    // remove the unwanted characters just for now

    std::string tempDispStr(findNetNodeModule(_mod)->displayString());
    std::string oldX(disParser.getTagArg("p", 0));
    std::string oldY(disParser.getTagArg("p", 1));

    int oldXStrPos = tempDispStr.find(oldX);

    if (oldX != ss_x.str())
    {
      tempDispStr.erase(oldXStrPos, oldX.length());
      tempDispStr.insert(oldXStrPos, ss_x.str());
    }
    if ( oldY != ss_y.str())
    {
      int oldYStrPos = tempDispStr.find(oldY, oldXStrPos + oldX.length());
      int newYStrPos = oldYStrPos + (ss_x.str().length() - oldX.length());

      tempDispStr.erase(newYStrPos, oldY.length());
      tempDispStr.insert(newYStrPos, ss_y.str());
    }

    drawWirelessRange(tempDispStr);

    findNetNodeModule(_mod)->setDisplayString(tempDispStr.c_str());
  }
}
