// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/RoutingAlgorithmStatic.cc,v 1.1 2005/02/09 06:15:58 andras Exp $
// Copyright (C) 2003, 2004 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file   RoutingAlgorithmStatic.cc
 * @author Johnny Lai
 * @date   13 Jul 2003
 * 
 * @brief  Implementation of RoutingAlgorithmStatic
 *
 * @todo
 * 
 */

#include "sys.h"
#include "debug.h"

#include "RoutingAlgorithmStatic.h"

#include "WorldProcessor.h"
#include "opp_utils.h"
#include "RoutingTable6.h"
#include "IPv6XMLWrapManager.h"

#include <cassert>
#include <boost/cast.hpp>
#include <xmlwrapp/xmlwrapp.h>

Define_Module_Like(RoutingAlgorithmStatic, RoutingAlgorithm);

int RoutingAlgorithmStatic::numInitStages() const
{
  return 3;
}

void RoutingAlgorithmStatic::initialize(int stageNo)
{
  if (stageNo == 0)
  {
     wp = boost::polymorphic_downcast<WorldProcessor*>
       (OPP_Global::iterateSubMod(simulation.systemModule(), "WorldProcessor"));
     assert(wp != 0);
 
     //parse this network node's parameters and load them into this.
     //parseNetworkEntity(rt->nodeName());
 
  }
  
  //No reason why we do this in stage 3
  //Should be done after all nodes are connected.
  //After link local addr are assigned
  //After static global address are assigned from static xml conf/learned from router
  //Time 30?
  if (stageNo == 2)
  {
    cTopology *topo = new cTopology("topo");

    // this can probably be made more flexible
    //topo->extractByModuleType(par("nodeType"), NULL);
    topo->extractByModuleType("Router6", "UDPNode", NULL);
    ev << "cTopology found " << topo->nodes() << " nodes\n";
    Dout(dc::notice, "cTopology found " << topo->nodes()<<" nodes." );
/*
    scheduleAt(20, new cMessage("routingStater"));


    for (int i=0; i<topo->nodes(); i++)
    {
      //RTEntry key;
      cTopology::Node *destNode = topo->node(i);
      //key.destAddress = destNode->module()->par("address");
      RoutingTable6* drt = check_and_cast<RoutingTable6*>(
        OPP_Global::findModuleByTypeDepthFirst(destNode->module(), "RoutingTable6"));
      assert(drt);
      //retrieve addresses of this node and their ifIndexes and store in a pair
      topo->unweightedSingleShortestPathsTo(destNode);
      //Perhaps should just retieve very last address of first interface and use that as the
      //global address of destination?


      for (int j=0; j<topo->nodes(); j++)
      {
        if (i==j) continue;
        cTopology::Node *atNode = topo->node(j);
        if (atNode->paths()==0) continue; // not connected

        RoutingTable6* art = check_and_cast<RoutingTable6*>(
          OPP_Global::findModuleByTypeDepthFirst(atNode->module(), "RoutingTable6"));
        assert(art);
        //Fill in route and put into this table

        if (art == drt)
          continue;

        //Get nexthop info
        RoutingTable6* nrt = 0;

        if (atNode->path(0)->remoteNode() == destNode)
          nrt = drt;
        else
        {
          Dout(dc::notice, "Different destNode and nextHopNode");
          nrt = check_and_cast<RoutingTable6*>(
            OPP_Global::findModuleByTypeDepthFirst(
              atNode->path(0)->remoteNode()->module(), "RoutingTable6"));
          assert(nrt);
        }

        Dout(dc::notice, "add route for "<<art->nodeName()<<" to "
             <<drt->nodeName()<<" via "<<nrt->nodeName());
        Interface6Entry& nie = nrt->getInterfaceByIndex(atNode->path(0)->remoteGate()->index());
        //Retrieve link local addr for next hop addr


        //key.atAddress = atNode->module()->par("address");
        
        unsigned int ifIndex = atNode->path(0)->localGate()->index();
        //art->
        
        //rtable[key] = gateId;
        //ev << "  from " << key.atAddress << " towards " << key.destAddress << " gateId is " << gateId << endl;
      }
    }
*/
  }
}

void RoutingAlgorithmStatic::handleMessage(cMessage* msg)
{
  assert(false);
}

namespace
{
  const string XML_ON = "on";
}

void RoutingAlgorithmStatic::parseInterface(const xml::node& iface)
{
  const XMLConfiguration::IPv6XMLWrapManager* man = wp->xmlManager();
  cerr << man->getNodeProperties(iface, "AdvManagedFlag");
  
}

void RoutingAlgorithmStatic::parseNetworkNodeProperties(const xml::node& netNode)
{
  const XMLConfiguration::IPv6XMLWrapManager* man = wp->xmlManager();
  const xml::attributes& att = netNode.get_attributes();

  if (wp->xmlManager()->version() < 3 || man->getNodeProperties(netNode, "routePackets") != XML_ON)
    rt->setForwardPackets(false);
  else
    rt->setForwardPackets(true);

  if (wp->xmlManager()->version() < 2 || man->getNodeProperties(netNode, "forwardSitePackets") != XML_ON)
    rt->setForwardPackets(true);
  else
    rt->setForwardPackets(false);

#ifdef USE_MOBILITY

  if (wp->xmlManager()->version() < 3 ||  man->getNodeProperties(netNode, "mobileIPv6Support") != XML_ON)
    rt->setMobilitySupport(false);
  else
    rt->setMobilitySupport(true);

  rt->setRole(RoutingTable6::CORRESPONDENT_NODE);

  if (rt->mobilitySupport() && wp->xmlManager()->version() >= 3)
  {
    string role =  man->getNodeProperties(netNode, "mobileIPv6Role");
    if (role == "HomeAgent")
      rt->setRole(RoutingTable6::HOME_AGENT);
    else if (role == "MobileNode")
      rt->setRole(RoutingTable6::MOBILE_NODE);
  }

#ifdef USE_HMIP
  if (wp->xmlManager()->version() < 5 ||  man->getNodeProperties(netNode, "hierarchicalMIPv6Support") != XML_ON)
    rt->setHmipSupport(false);
  else if (rt->mobilitySupport())
    rt->setHmipSupport(true);
  else
  {
    cerr << "HMIP Support cannot be activated.\n"
         << "Please set mobility support options before trying again\n";
    exit(1);
  }

  rt->setMapSupport(false);

  if (rt->hmipSupport() && wp->xmlManager()->version() >= 3)
    if ((*(att.find("map"))).get_value() == XML_ON)
      if ( rt->getRole() == RoutingTable6::HOME_AGENT)
        rt->setMapSupport(true);
      else
      {
        cerr << "MAP support cannot be activated without HA support.\n"
             << "Please check mobileIPv6Support to make sure it is HomeAgent\n";
        exit(1);
      }

#endif //USE_HMIP

#endif // USE_MOBILITY

}

void RoutingAlgorithmStatic::parseNetworkEntity(const char* nodeName)
{
  const XMLConfiguration::IPv6XMLWrapManager* man = wp->xmlManager();
  //const xml::node& netNode = man->getNetNode(rt->nodeName());
  xml::node netNode;
  parseNetworkNodeProperties(netNode);
  
  typedef xml::node::const_iterator NodeIt;
  for (NodeIt it = netNode.begin(); it != netNode.end(); it++)
  {
    NodeIt ifit = netNode.find("interface", it);
    if (ifit != netNode.end())
      parseInterface(*ifit);
  }
}


#if defined USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>


/**
   @class RoutingAlgorithmStaticTest
   @brief 	
   @ingroup TestCases
*/

class RoutingAlgorithmStaticTest: public CppUnit::TestFixture
{
public:

  CPPUNIT_TEST_SUITE( RoutingAlgorithmStaticTest );
  CPPUNIT_TEST( testExample1 );
  CPPUNIT_TEST( testExample2 );
  CPPUNIT_TEST_SUITE_END();
 public:
  
  // Constructor/destructor.
  RoutingAlgorithmStaticTest();
  virtual ~RoutingAlgorithmStaticTest();

  void testExample1();
  void testExample2();

  void setUp();
  void tearDown();

private:

  // Unused ctor and assignment op.
  RoutingAlgorithmStaticTest(const RoutingAlgorithmStaticTest&);
  RoutingAlgorithmStaticTest& operator=(const RoutingAlgorithmStaticTest&);
};

CPPUNIT_TEST_SUITE_REGISTRATION( RoutingAlgorithmStaticTest );

RoutingAlgorithmStaticTest::RoutingAlgorithmStaticTest()
{
}

RoutingAlgorithmStaticTest::~RoutingAlgorithmStaticTest()
{
}

void RoutingAlgorithmStaticTest::setUp()
{
}

void RoutingAlgorithmStaticTest::tearDown()
{
}

void RoutingAlgorithmStaticTest::testExample1()
{
  CPPUNIT_ASSERT(1==1);
}

void RoutingAlgorithmStaticTest::testExample2()
{
  CPPUNIT_ASSERT((2+3)==5);
}

#endif //defined USE_CPPUNIT

