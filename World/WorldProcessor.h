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

/**
   @file WorldProcessor.h

   @brief Definition of WorldProcessor

   A module that handles the terrain positions of all the entities

   @author Eric Wu
 */

#ifndef MOBILEWORLDPROCESSOR_H
#define WORLDPROCESSOR_H

#include <omnetpp.h>

#ifdef USE_MOBILITY

#include <string>
#include <vector>
//Can't use forward dec of Entity because we still need definition of
//MobileEntityType
#include "Entity.h"
#include "cTimerMessageCB.h"

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
 * @class WorldProcessor
 * @brief Handles management of mobility in simulation

 * Also its initialize member is used for one off initialisation at start of
 * simulation because this is always the first module to be constructed
 */

class WorldProcessor : public cSimpleModule
{
  static const int RoutingTable6StageCount = 3;

 public:
  // OMNeT++ functions
  Module_Class_Members(WorldProcessor,cSimpleModule, 0);

  ///Place code here that to be done once only before all other initialize
  ///functions
  virtual void initialize(int stage);
  virtual void handleMessage(cMessage* msg);
  virtual int  numInitStages() const  {return  RoutingTable6StageCount;}
  virtual void finish();


  /**
   * Fills in the given RoutingTable6 data structure with the routing table
   * of the host whose hostname is passed in the RoutingTable6 data structure.
   */
  void parseNetworkEntity(RoutingTable6* rt );

  /**
   * Fills in the given RoutingTable6 data structure with the static routing table
   * of the host given by the nodeName parameter.
   */
  void staticRoutingTable(RoutingTable6* rt);

#ifdef USE_HMIP
  /**
   * Fills in the MAP info part of the given RoutingTable6 data structure
   * for the host whose hostname is passed in the RoutingTable6 data structure.
   */
  void parseMAPInfo(RoutingTable6* rt);
#endif // USE_HMIP

  XMLConfiguration::XMLOmnetParser* xmlManager() const { return parser; }
private:
  mutable XMLConfiguration::XMLOmnetParser* parser;

#ifdef USE_MOBILITY
public:

  /**
   * Fills in parameters inside the given WirelessEtherModule.
   */
  void parseWirelessEtherInfo(WirelessEtherModule* mod);

  /**
   * Fills in parameters inside the given MobilityStatic.
   */
  void parseMovementInfo(MobilityStatic* mod);

  /**
   * Fills in parameters inside the given MobilityRandomWP and
   * MobilityRandomWalk.
   */
  void parseRandomWPInfo(MobilityRandomWP* mod);

  /**
   * Fills in parameters inside the given MobilityRandomPattern.
   */
  void parseRandomPatternInfo(MobilityRandomPattern* mod);

  /**
   * Register a wireless entity in the world processor.
   * XXX returns existing entity if already exists
   *
   * FIXME deregistration?
   */
  Entity* registerEntity(std::string entityName, MobileEntityType, cSimpleModule* mod);

  /**
   * Find entity by its name member.
   */
  Entity* findEntityByName(std::string entityName);

  /**
   * Get the list of Wireless Ethernet modules on a given channel.
   * This is used when a wireless entity wants to send a frame to determine
   * who should receive a copy of the frame.
   */
  ModuleList findWirelessEtherModulesByChannel(const int channel);

  ModuleList findWirelessEtherModulesByChannelRange(const int minChan, const int maxChan);

  /**
   * Find entity by its module.
   */
  Entity* findEntityByModule(cModule* module);

 private:
  ModuleList modList;
  int _maxLongitude;
  int _maxAltitude;

  void updateStats(void);
  Loki::cTimerMessageCB<void>* updateStatsNotifier;
  cOutVector* balanceIndexVec;
#endif // USE_MOBILITY
};

#endif // WORLDPROCESSOR_H
