//
// Copyright (C) 2002 CTIE, Monash University
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
   @file WorldProcessor.cc

   @brief A processor that handles the terrain positions of all the entities

   @author Eric Wu
 */

#include "sys.h"
#include "debug.h"

#include <sstream>

#include "Entity.h"
#include "WorldProcessor.h"
#include "RoutingTable6.h"
#include "XML/XMLOmnetParser.h"

#include "libcwdsetup.h"

#include "opp_utils.h"

#ifdef USE_MOBILITY
#include "MobilityStatic.h"
#include "MobilityRandomWP.h"
#include "MobilityRandomPattern.h"
#include "MobileBaseStation.h"
#include "MobileEntity.h"

#include "WirelessEtherModule.h"
#include "WirelessAccessPoint.h"
#include "wirelessethernet.h"
using std::vector;

const int TMR_WPSTATS = 1000;
#endif //USE_MOBILITY


namespace
{
#ifdef USE_CPPUNIT
  bool runUnitTests();
#endif //USE_CPPUNIT
}// anonymous namespace


Define_Module( WorldProcessor );

WorldProcessor::WorldProcessor(const char *name, cModule *parent, unsigned stacksize) :
  cSimpleModule(name, parent, stacksize)
{
  parser = NULL;
}

void WorldProcessor::handleMessage(cMessage* msg)
{
  error("WorldProcessor doesn't process messages");
}

void WorldProcessor::initialize(int stage)
{
  if (stage == 0)
  {
    cXMLElement *config = par("IPv6routingFile");

    parser = new XMLConfiguration::XMLOmnetParser();
    parser->setDoc(config);
#ifdef CWDEBUG
    Debug( libcwdsetup::l_debugSettings(parser->retrieveDebugChannels()) );
#endif

#ifdef USE_MOBILITY
    _maxLongitude = par("max_longitude").longValue();
    _maxLatitude = par("max_latitude").longValue();

    // set up terrain size in Tk/Tcl environment
    cDisplayString& disp = parentModule()->displayString();
    disp.setTagArg("b", 0, OPP_Global::ltostr(_maxLongitude).c_str());
    disp.setTagArg("b", 1, OPP_Global::ltostr(_maxLatitude).c_str());
#endif //USE_MOBILITY
  }
#ifdef USE_CPPUNIT
  else if (stage == RoutingTable6StageCount-1)
  {
    //Has to be after XML loaded as one of the tests is on XML parsing
    //Has to be at least after RoutingTable module is initialised.
    assert(runUnitTests());
  }
#endif //USE_CPPUNIT
}

void WorldProcessor::finish()
{
  delete parser;
  parser = 0;
  recordScalar("liveMessageCount", cMessage::liveMessageCount());
  recordScalar("totalMessageCount", cMessage::totalMessageCount());
  recordScalar("endTime", simTime());
}

#if defined USE_CPPUNIT
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <string>
CPPUNIT_TEST_SUITE_REGISTRATION( cTypedContainerTest );

namespace
{

bool runUnitTests()
{
  CppUnit::TextUi::TestRunner runner;
  CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
  runner.addTest( registry.makeTest() );
  return runner.run();
}

}// anonymous namespace
#endif //defined USE_CPPUNIT

#ifdef USE_MOBILITY
Entity* WorldProcessor::findEntityByName(const std::string& entityName)
{
  for ( size_t i = 0; i < modList.size(); i++)
  {
    if (modList[i]->entityName == entityName)
    {
      return modList[i];
    }
  }
  return 0;
}

Entity* WorldProcessor::findEntityByNodeName(const std::string& nodeName)
{
  for ( size_t i = 0; i < modList.size(); i++)
  {
    cStringTokenizer tokenizer(modList[i]->entityName.c_str(), ".");
    const char *token;
    token = tokenizer.nextToken();
    token = tokenizer.nextToken();
    if (nodeName == token)
      return modList[i];
  }
  return 0;
}

Entity* WorldProcessor::registerEntity(const std::string& name,
                                             MobileEntityType type,
                                             cSimpleModule* mod)
{
  Entity* entity;

  switch(type)
  {
    case MobileBS:
      entity = new BaseStation(mod);
      break;

    case MobileMN:
      entity = new MobileEntity(mod);   // XXX why not let caller do this?
      break;

    default:
      cerr << "Unknown wireless type... " << endl;
      abort_ipv6suite();
      break;
  }
  entity->entityName = name;  //XXX why not let caller do this? --AV
  modList.push_back(entity);

  return entity;
}

//Maybe have another similar function which returns a list of modules which can
//listen on a particular channel, taking into account cross talk.
ModuleList WorldProcessor::
findWirelessEtherModulesByChannel(const int channel)
{
  ModuleList mods;

  // Find all wireless interface on the channel and add it to the list
  for (size_t i = 0; i < modList.size(); i++)
  {
    //Get the interface's link layer
    cModule* interface = modList[i]->containerModule()->parentModule()->parentModule();
    cModule* phylayer = interface->gate("wlin")->toGate()->ownerModule();
    cModule* linkLayer =  phylayer->gate("linkOut")->toGate()->ownerModule();

    if(linkLayer != NULL)
    {
      if (std::string(linkLayer->par("NWIName")) == "WirelessEtherModule" ||
          std::string(linkLayer->par("NWIName")) == "WirelessAccessPoint")
      {
        WirelessEtherModule* a = static_cast<WirelessEtherModule*>(linkLayer->submodule("networkInterface"));
        //check if device is on the same channel
        if ( a->getChannel() == channel )
          mods.push_back(modList[i]);
      }
    }
  }
  return mods;
}

ModuleList WorldProcessor::
findWirelessEtherModulesByChannelRange(const int minChan, const int maxChan)
{
  ModuleList mods;

  // Find all wireless interface on the channel and add it to the list
  for (size_t i = 0; i < modList.size(); i++)
  {
    //Get the entity's interface
    cModule* interface = modList[i]->containerModule()->parentModule()->parentModule();
    unsigned int noOfInterface = interface->gate("wlin")->size();

    //Check all interface in entity
    for(size_t j=0; j< noOfInterface; j++)
    {
      cModule* phylayer = interface->gate("wlin",j)->toGate()->ownerModule();
      cModule* linkLayer =  phylayer->gate("linkOut")->toGate()->ownerModule();

      if(linkLayer != NULL)
      {
        if (std::string(linkLayer->par("NWIName")) == "WirelessEtherModule" ||
            std::string(linkLayer->par("NWIName")) == "WirelessAccessPoint")
        {
          WirelessEtherModule* a = static_cast<WirelessEtherModule*>(linkLayer->submodule("networkInterface"));
          // check if interface is operating in channel range
          if ( (a->getChannel() >= minChan)&&(a->getChannel() <= maxChan) )
          {
            // Only add into the list once
            mods.push_back(modList[i]);
            break;
          }
        }
      }
    }
  }
  return mods;
}

Entity* WorldProcessor::findEntityByModule(cModule* module)
{
  for (size_t i = 0; i < modList.size(); i++)
  {
    if (modList[i]->containerModule() == module)
      return modList[i];
  }

  return 0;
}

#endif //USE_MOBILITY


