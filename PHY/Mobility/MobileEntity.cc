// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/PHY/Mobility/Attic/MobileEntity.cc,v 1.3 2005/02/14 01:20:38 andras Exp $
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

const int CONNECT = 1111;
const int DISCONNECT = 1112;
const char* CONNECT_MSG = "CONNECT";

const char* ME_OUT = "to";
const char* ME_IN = "from";

bool operator==(MEConnInfo& lhs, MEConnInfo& rhs)
{
  return (lhs.en == rhs.en &&
          lhs.outputGate == rhs.outputGate &&
          lhs.inputGate == rhs.inputGate);
}

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

bool MobileEntity::disconnect(Entity* otherEntity)
{
  bool isGateDisconnected = false;

  MEConnInfo meInfo;
  meInfo.en = otherEntity;

  cModule* selfNode = findNetNodeModule(_mod);
  cModule* otherNode = findNetNodeModule(otherEntity->containerModule());

  cGate* childModGt = 0;

  cModule* currentMod = 0;
  cGate* currentModGt = 0;

  cGate* otherNodeGt = 0;

  for (int i = 0; i < selfNode->gates(); i++)
  {
    cGate* gt = selfNode->gate(i);

    if (gt !=0 && gt->type() == 'O')
    {
      otherNodeGt = gt->toGate();

      if (otherNodeGt != 0 && otherNodeGt->ownerModule() == otherNode)
      {
        // delete self gates from top to down hierarchically

        currentModGt = gt;
        currentMod = currentModGt->ownerModule();
        childModGt = gt->fromGate();

        while (currentMod != _mod)
        {
          currentMod->gatev.remove(currentModGt);
          delete currentModGt;

          currentModGt = childModGt;
          currentMod = currentModGt->ownerModule();
          childModGt = currentModGt->fromGate();
        }

        meInfo.outputGate = currentModGt->id();
        currentMod = currentModGt->ownerModule();
        currentMod->gatev.remove(currentModGt);
        delete childModGt;

        // delete other gates from top to down hierachically

        currentModGt = otherNodeGt;
        childModGt = otherNodeGt->toGate();
        currentMod = currentModGt->ownerModule();

        while (currentMod != otherEntity->containerModule())
        {
          currentMod->gatev.remove(currentModGt);
          delete currentModGt;

          currentModGt = childModGt;
          currentMod = currentModGt->ownerModule();
          childModGt = currentModGt->toGate();
        }

        currentMod = currentModGt->ownerModule();
        currentMod->gatev.remove(currentModGt);
        delete childModGt;

        isGateDisconnected = true;
      }
    }
    else if (gt != 0 && gt->type() == 'I')
    {
      otherNodeGt = gt->fromGate();

      if (otherNodeGt != 0 && otherNodeGt->ownerModule() == otherNode)
      {
        // delete self gates from top to down hierarchically

        currentModGt = gt;
        currentMod = gt->ownerModule();
        childModGt = gt->toGate();

        while (currentMod != _mod)
        {
          currentMod->gatev.remove(currentModGt);
          delete currentModGt;

          currentModGt = childModGt;
          currentMod = currentModGt->ownerModule();
          childModGt = currentModGt->toGate();
        }


        meInfo.inputGate = currentModGt->id();
        currentMod = currentModGt->ownerModule();
        currentMod->gatev.remove(currentModGt);
        delete currentModGt;

        // delete other gates from top to down hierachically

        currentModGt = otherNodeGt;
        currentMod = currentModGt->ownerModule();
        childModGt = otherNodeGt->fromGate();

        while (currentMod != otherEntity->containerModule())
        {
          currentMod->gatev.remove(currentModGt);
          delete currentModGt;

          currentModGt = childModGt;
          currentMod = currentModGt->ownerModule();
          childModGt = currentModGt->fromGate();
        }

        currentMod = currentModGt->ownerModule();
        currentMod->gatev.remove(currentModGt);
        delete childModGt;

        isGateDisconnected = true;
      }
    }
  }

  if (isGateDisconnected)
  {
    if (otherEntity->entityType() == MobileBS)
      ((BaseStation*)(otherEntity))->dettachBS(this);
  }

  return isGateDisconnected;
}

int MobileEntity::connectWith(Entity* otherEntity, bool isOutgoing)
{
  int gateid = -1;

  if (!otherEntity)
    return gateid;

  // network node level modules that contain the simple module of
  // entity
  cModule* selfNode = findNetNodeModule(_mod);
  cModule* otherNode = findNetNodeModule(otherEntity->containerModule());

  char selfDirection; // direction of the gate of this entity
  char otherDirection; // direction of the gate of the connecting entity

  cGate* selfGt = 0; // a pointer to the current gate of this entity
  cGate* otherGt = 0; // a pointer to the current gate of the connecting entity

  // labels that are assigned to all of the gates recursively up to the
  // network node level module
  string selfGtName;
  string otherGtName;

  // initialise all necessary variables

  if (isOutgoing)
  {
    selfDirection = 'O';
    selfGtName = ME_OUT;

    otherDirection = 'I';
    otherGtName = ME_IN;
  }
  else
  {
    selfDirection = 'I';
    selfGtName = ME_IN;

    otherDirection = 'O';
    otherGtName = ME_OUT;
  }

  // assign gate names recursively from simple module to the network
  // node module levels between "this" entity and the connecting
  // entity

  string selfNodeGtName = string(selfNode->name()) +
    selfGtName +
    string(otherNode->name());

  string otherNodeGtName = string(otherNode->name()) +
    otherGtName +
    string(selfNode->name());

  cModule* childMod = _mod;
  cModule* parentMod = childMod->parentModule();

  // connect the self gates recursively to the network node level

  while (childMod != selfNode)
  {
    selfGt = 0;

    if (childMod->findGate(selfNodeGtName.c_str()) == -1)
    {
      selfGt = new cGate(selfNodeGtName.c_str(), selfDirection);
      selfGt->setOwnerModule(childMod, childMod->gates());
      int tempgateid = childMod->gatev.add(selfGt);

      if (gateid == -1)
        gateid = tempgateid;
    }
    else
      selfGt = childMod->gate(selfNodeGtName.c_str());

    cGate* selfParentGt = new cGate(selfNodeGtName.c_str(), selfDirection);
    selfParentGt->setOwnerModule(parentMod, parentMod->gates());
    parentMod->gatev.add(selfParentGt);

    if(isOutgoing)
    {
      selfGt->setTo(selfParentGt);
      selfParentGt->setFrom(selfGt);
    }
    else
    {
      selfGt->setFrom(selfParentGt);
      selfParentGt->setTo(selfGt);
    }

    childMod = parentMod;
    parentMod = parentMod->parentModule();
  }

  // connect the other gates recursively to the network node level

  childMod = otherEntity->containerModule();
  parentMod = childMod->parentModule();

  while(childMod != otherNode)
  {
    otherGt = 0;

    if (childMod->findGate(otherNodeGtName.c_str()) == -1)
    {
      otherGt = new cGate(otherNodeGtName.c_str(), otherDirection);
      otherGt->setOwnerModule(childMod, childMod->gates());
      childMod->gatev.add(otherGt);
    }
    else
      otherGt = childMod->gate(otherNodeGtName.c_str());

    cGate* otherParentGt = new cGate(otherNodeGtName.c_str(), otherDirection);
    otherParentGt->setOwnerModule(parentMod, parentMod->gates());
    parentMod->gatev.add(otherParentGt);

    if(isOutgoing)
    {
      otherGt->setFrom(otherParentGt);
      otherParentGt->setTo(otherGt);
    }
    else
    {
      otherGt->setTo(otherParentGt);
      otherParentGt->setFrom(otherGt);
    }

    childMod = parentMod;
    parentMod = parentMod->parentModule();
  }

  // connect gates between node level modules

  cGate* selfNodeGt = selfNode->gate(selfNodeGtName.c_str());
  cGate * otherNodeGt = otherNode->gate(otherNodeGtName.c_str());

  if(isOutgoing)
  {
    selfNodeGt->setTo(otherNodeGt);
    otherNodeGt->setFrom(selfNodeGt);
  }

  else
  {
    selfNodeGt->setFrom(otherNodeGt);
    otherNodeGt->setTo(selfNodeGt);
  }
  return gateid;
}

void MobileEntity::drawWirelessRange(std::string& dispStr)
{
  if (dispStr.find("r=") == std::string::npos)
  {
    //Does not work for all modules either as some do not even have layer 3
    //unsigned int numOfPorts = OPP_Global::findModuleByType(OPP_Global::findNetNodeModule(_mod), "RoutingTable6")->par("numOfPorts");

    //unsigned int numOfPorts = OPP_Global::findNetNodeModule(_mod)->gateSize("wlin");
    // gateSize() is an omnetpp version 3.0 function
    unsigned int numOfPorts = OPP_Global::findNetNodeModule(_mod)->gate("wlin")->size();
    if (numOfPorts <= 0)
    {
      cerr << "gateSize of nonexistant gate array " <<numOfPorts<<endl;
      //For MobileNodes
      //numOfPorts = OPP_Global::findNetNodeModule(_mod)->gateSize("in");
      numOfPorts = OPP_Global::findNetNodeModule(_mod)->gate("in")->size();
    }

    // label interface name according to the link layer
    for(size_t i = 0; i < numOfPorts; i ++)
    {
      cModule* mod = OPP_Global::findModuleByType(_mod, "LinkLayer6");

      LinkLayerModule* llmodule = 0;
      mod = mod->parentModule()->submodule("linkLayers", i);

      if(mod)
      {
        llmodule = (LinkLayerModule*)(mod->submodule("networkInterface"));
        if (llmodule->getInterfaceType() != PR_WETHERNET)
          continue;

        WirelessEtherModule* wem =
          boost::polymorphic_downcast<WirelessEtherModule*>(llmodule);

        assert(wem);

        dispStr += ";r=";
        dispStr += boost::lexical_cast<std::string>(wem->wirelessRange());
        dispStr += ",,blue";
      }
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

  setDispPosition(_pos.x, _pos.y);

  return true;
}

