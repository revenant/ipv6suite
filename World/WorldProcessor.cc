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

#if USE_AKAROA
#include "opp_akaroa.h"
static const unsigned int OPP_AK_OBSERVE_PARAMETERS = 3;
#endif //USE_AKAROA

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
  assert(msg);
  if ( !msg->isSelfMessage())
    assert(false);
  else
    static_cast<cTimerMessage*>(msg)->callFunc();
}

void WorldProcessor::initialize(int stage)
{
  if (stage == 0)
  {
    const char* filename = par("IPv6routingFile");

    // Read routing table file once only
    if (filename[0] != '\0')
    {
      parser = new XMLConfiguration::XMLOmnetParser();
      parser->parseFile(filename);
      Debug( libcwdsetup::l_debugSettings(parser->retrieveDebugChannels()) );
    }

#ifdef USE_MOBILITY
    _maxLongitude = par("max_longitude").longValue();
    _maxLatitude = par("max_latitude").longValue();

    // set up terrain size in Tk/Tcl environment
    cDisplayString& disp = parentModule()->displayString();
    disp.setTagArg("b", 0, OPP_Global::ltostr(_maxLongitude).c_str());
    disp.setTagArg("b", 1, OPP_Global::ltostr(_maxLatitude).c_str());

    // XXX A bit of a hack
    //BASE_SPEED is in wirelessEthernet.h/cc unit
    BASE_SPEED = par("wlan_speed").doubleValue() * 1024 * 1024;
    Dout(dc::notice, " 802.11b wlan is at rate of "<<BASE_SPEED<<" bps");

    balanceIndexVec = new cOutVector("balanceIndex");
    // Timer to update statistics
    updateStatsNotifier  = new Loki::cTimerMessageCB<void>(TMR_WPSTATS,
                   this, this, &WorldProcessor::updateStats, "updateStats");

    scheduleAt(simTime()+1, updateStatsNotifier);
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
}

void WorldProcessor::parseNetworkEntity(RoutingTable6* rt)
{
  parser->parseNetworkEntity(rt);
}


#ifdef USE_HMIP
void WorldProcessor::parseMAPInfo(RoutingTable6* rt)
{
  parser->parseMAPInfo(rt);
}
#endif //USE_HMIP

void WorldProcessor::staticRoutingTable(RoutingTable6* rt)
{
  parser->staticRoutingTable(rt);
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

void WorldProcessor::parseWirelessEtherInfo(WirelessEtherModule* mod)
{
  parser->parseWirelessEtherInfo(mod);
}

void WorldProcessor::parseMovementInfo(MobilityStatic* mod)
{
  parser->parseMovementInfo(mod);
}

void WorldProcessor::parseRandomWPInfo(MobilityRandomWP* mod)
{
  parser->parseRandomWPInfo(mod);
}

void WorldProcessor::parseRandomPatternInfo(MobilityRandomPattern* mod)
{
  parser->parseRandomPatternInfo(mod);
}

Entity* WorldProcessor::findEntityByName(string entityName)
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

Entity* WorldProcessor::registerEntity(string name,
                                             MobileEntityType type,
                                             cSimpleModule* mod)
{
  // XXX for loop is obsolete, this gets called only once per mobile host
/*XXX
  for ( size_t i = 0; i < modList.size(); i++)
  {
    if ( modList[i]->entityName == name && modList[i]->entityType() == type)
    {
      return modList[i];
    }
  }
*/
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
      exit(1);
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

void WorldProcessor::updateStats(void)
{
  double balanceIndex =0, loadSum=0, loadSquaredSum=0, n=0, usedBW;

  for (size_t i = 0; i < modList.size(); i++)
  {
    //Get the interface's link layer
    cModule* interface = modList[i]->containerModule()->parentModule()->parentModule();
    cModule* phylayer = interface->gate("wlin")->toGate()->ownerModule();
    cModule* linkLayer =  phylayer->gate("linkOut")->toGate()->ownerModule();

    if(linkLayer != NULL)
    {
      if (std::string(linkLayer->par("NWIName")) == "WirelessAccessPoint")
      {
        WirelessAccessPoint* a = static_cast<WirelessAccessPoint*>(linkLayer->submodule("networkInterface"));
        usedBW = (BASE_SPEED/(1024*1024))-a->getEstAvailBW();
        loadSum += usedBW;
        loadSquaredSum += usedBW*usedBW;
        n++;
      }
    }
  }
  balanceIndex = (n*loadSquaredSum > 0) ? (loadSum*loadSum)/(n*loadSquaredSum):0;
  balanceIndexVec->record(balanceIndex);
  //Allow sim to quit if nothing of interest is happening.
  if (!simulation.msgQueue.empty())
    scheduleAt(simTime()+1, updateStatsNotifier);
}

#endif //USE_MOBILITY
/**
   Private Functions
**/
