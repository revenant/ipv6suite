// -*- C++ -*-
//
// Copyright (C) 2002, 2003 CTIE, Monash University
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


#ifndef WORLDPROCESSOR_H
#define WORLDPROCESSOR_H

#include <omnetpp.h>

#ifdef USE_MOBILITY

#include <string>
#include <vector>
//Can't use forward dec of Entity because we still need definition of
//MobileEntityType
#include "Entity.h"

class BaseStation;

#endif //USE_MOBILITY


class RoutingTable6;

namespace XMLConfiguration
{
  class IPv6XMLManager;
  class XMLOmnetParser;
  class IPv6XMLWrapManager;
}

#ifdef USE_MOBILITY
class WirelessEtherModule;
class MobilityStatic;
class MobilityRandomWP;
class MobilityRandomPattern;

typedef std::vector<Entity*> ModuleList;
typedef std::vector<Entity*>::iterator MLIT;

extern const int TMR_WPSTATS;
#endif // USE_MOBILITY


/**
 * Handles management of mobility in simulation.
 *
 * Also its initialize member is used for one off initialisation at start of
 * simulation because this is always the first module to be constructed
 */
class WorldProcessor : public cSimpleModule
{
  static const int RoutingTable6StageCount = 3;

 public:
  // OMNeT++ functions
  WorldProcessor(const char *name=NULL, cModule *parent=NULL, unsigned stacksize=0);

  ///Place code here that to be done once only before all other initialize
  ///functions
  virtual void initialize(int stage);
  virtual void handleMessage(cMessage* msg);
  virtual int  numInitStages() const  {return  RoutingTable6StageCount;}
  virtual void finish();

  XMLConfiguration::XMLOmnetParser* xmlConfig() const { return parser; }

#ifdef USE_MOBILITY
public:

  /**
   * Register a wireless entity in the world processor.
   * XXX returns existing entity if already exists
   *
   * FIXME deregistration?
   */
  Entity* registerEntity(const std::string& entityName, MobileEntityType, cSimpleModule* mod);

  /**
   * Find entity by its name member.
   */
  Entity* findEntityByName(const std::string& entityName);

  /**
   * Find entity by its network node name 
   */
  Entity* findEntityByNodeName(const std::string& nodeName);

  /**
   * Get the list of Wireless Ethernet modules on a given channel.
   * This is used when a wireless entity wants to send a frame to determine
   * who should receive a copy of the frame.
   */
  ModuleList findWirelessEtherModulesByChannel(const int channel);

  ModuleList findWirelessEtherModulesByChannelRange(const int minChan, const int maxChan);

  /**
   * Returns the list of all registered entities
   */
  const ModuleList& entities()  {return modList;}

  /**
   * Find entity by its module.
   */
  Entity* findEntityByModule(cModule* module);
#endif // USE_MOBILITY

private:
  mutable XMLConfiguration::XMLOmnetParser* parser;

#ifdef USE_MOBILITY
  ModuleList modList;
  int _maxLongitude;
  int _maxLatitude;
#endif // USE_MOBILITY

  //XXX void updateStats() stuff factored out to separate WirelessStats module --AV
};

#endif // WORLDPROCESSOR_H
