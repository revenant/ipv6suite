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
   @file MobileEntity.cc

   @brief Implementation of MobileEntity

   @author Eric Wu
 */


#include "sys.h"
#include "debug.h"

#include <sstream>
#include <boost/cast.hpp>
#include <cmath>
#include "opp_utils.h"  // for int/double <==> string conversions

#include "MobileEntity.h"
#include "WorldProcessor.h"
#include "opp_utils.h"
#include "MobileBaseStation.h"
#include "LinkLayerModule.h"
#include "WirelessEtherModule.h"


using namespace::OPP_Global;
using std::string;
using std::list;

MobileEntity::MobileEntity(cSimpleModule* mod)
  : Entity(mod),
    _startMovingTime(0),
    _currentMoveIdx(0),
    _numOfAllowedBS(1)
{
  _type = MobileMN;

    cModule* m = OPP_Global::findModuleByName(mod, "worldProcessor");
    if(!mod)
    {
      opp_warning("worldProcessor instantiated from WorldProcessor not found . . .");
      return;
    }

    _mwp = static_cast<WorldProcessor*>(m);
}

void MobileEntity::drawWirelessRange()
{
  cModule *nodemod = findNetNodeModule(_mod);
  //Does not work for all modules either as some do not even have layer 3
  //unsigned int numOfPorts = OPP_Global::findModuleByType(nodemod, "RoutingTable6")->par("numOfPorts");

  unsigned int numOfPorts = nodemod->gateSize("wlin");
  //unsigned int numOfPorts = nodemod->gate("wlin")->size();
  if (numOfPorts <= 0)
  {
    cerr << "gateSize of nonexistant gate array " <<numOfPorts<< " in module " << nodemod->fullPath() << endl;
    //For MobileNodes
    numOfPorts = nodemod->gateSize("in");
    //numOfPorts = nodemod->gate("in")->size();
  }

  // label interface name according to the link layer
  for(size_t i = 0; i < numOfPorts; i++)
  {
    cModule* mod = OPP_Global::findModuleByType(_mod, "LinkLayer6");

    LinkLayerModule* llmodule = 0;
    mod = mod->parentModule()->submodule("linkLayers", i);

    if (mod)
    {
      llmodule = (LinkLayerModule*)(mod->submodule("networkInterface"));
      if (llmodule->getInterfaceType() != PR_WETHERNET)
        continue;

      WirelessEtherModule* wem = check_and_cast<WirelessEtherModule*>(llmodule);

      nodemod->displayString().setTagArg("r",0,OPP_Global::dtostr(wem->wirelessRange()).c_str());
      nodemod->displayString().setTagArg("r",2,"blue");
    }
  }
}

bool MobileEntity::moving(void)
{
  if ( moves.size() == 0)
    return false;

  // don't move if  the current position is on the FINAL destionation
  if ( _currentMoveIdx == moves.size()-1 && _pos == moves[moves.size()-1].destPos )
  {
    return false;
  }

  if ( _pos == moves[_currentMoveIdx].destPos)
  {
    _currentMoveIdx++;
  }

  // the mobile entity moves along the x-axis first then y-axis

  if ( moves[_currentMoveIdx].moveXFirst )
  {
    if ( _pos.x != moves[_currentMoveIdx].destPos.x )
    {
      if ( _pos.x < moves[_currentMoveIdx].destPos.x )
        _pos.x++;
      else
        _pos.x--;
    }
    else
    {
      if ( _pos.y < moves[_currentMoveIdx].destPos.y )
        _pos.y++;
      else
        _pos.y--;
    }
  }
  else
  {
    if ( _pos.y != moves[_currentMoveIdx].destPos.y )
    {
      if ( _pos.y < moves[_currentMoveIdx].destPos.y )
        _pos.y++;
      else
        _pos.y--;
    }
    else 
    {
      if ( _pos.x < moves[_currentMoveIdx].destPos.x )
        _pos.x++;
      else
        _pos.x--;
    }
  }

  setDispPosition(_pos.x, _pos.y);

  return true;
}

