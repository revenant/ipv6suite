//
// Copyright (C) 2002, 2004 CTIE, Monash University
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
 * @file   IPv6XMLManager.cc
 * @author Johnny Lai
 * @date   08 Apr 2002
 *
 * @brief Implementation of IPv6XMLManager class Takes over the XML related
 * functions in RoutingTable6 and introduce some efficiency and logic into the
 * code.

 * @todo Remaining bugs to do with incorrect interface initialisation (Twiki bug report)
 *
 */

/**
 * @def XML_STATICROUTINGTABLE
 *
 * @brief Display static routes (direct neighbours, next hop router and default
 * route) as parsed from XML
 */

/**
 * @def XML_ADDRESSES
 *
 * @brief Display addresses parsed from XML config file
 *
 */

/**
 * @def ROUTING
 *
 * @brief Display most of Routing decisions taken by every node
 * @sa VERBOSE ROUTINGTABLELOOKUP
 */

/**
 * @def ROUTINGTABLELOOKUP
 *
 * @brief Display the verbose walkthrough of the lookup i.e. longest prefix match.
 */

/**
 * @def VERBOSE
 *
 * @brief Display nearly just about every debug message (probably not useful)
 */

/**
 * @def BUILD_UML_INTERFACE
 *
 * @brief BUILD User Mode Linux interface only valid on linux systems
 */

/**
 * @def NOIPMASQ
 *
 * @brief prevent basic masquerade from occuring i.e. will not tunnel private
 * addresses
 */

#include "sys.h"
#include "debug.h"

#include "IPv6XMLManager.h"

#include <cassert>
#include <iterator> //ostream_iterator

#include <omnetpp.h>

#include "XMLCommon.h"
#include "opp_utils.h"
#include "RoutingTable6.h"

#ifdef USE_MOBILITY
#include "WirelessEtherModule.h"
#include "IPv6Mobility.h"
#endif

#include <xercesc/util/XMLString.hpp>     // general XML string
#include <xercesc/parsers/DOMParser.hpp> // XML parser
#include <xercesc/dom/DOM_DOMException.hpp>
#include "XMLDocHandle.h"
#include <boost/scoped_array.hpp>



namespace
{
  typedef boost::scoped_array<char> MyString;
}

namespace XMLConfiguration
{

  IPv6XMLManager::IPv6XMLManager(const char* filename)
  {
    std::cout<<"XML configuration file is "<<filename<<std::endl;
    readRoutingTableFromFile(filename);
  }

  IPv6XMLManager::~IPv6XMLManager()
  {}

    /// top level parsing function
  void IPv6XMLManager::readRoutingTableFromFile(const char* filename)
  {
    SingularParser::Instance().setFile(filename);
    SingularParser::Instance().initialize();
  }

  void IPv6XMLManager::parseNetworkEntity(RoutingTable6* rt)
  {
    static bool printXMLFile = false;
    if (!printXMLFile)
    {
      Dout(dc::notice, "XML Configuration file is "<<SingularParser::Instance().filename());
      printXMLFile = true;
    }
    parseNodeAttributes(rt);

    // construct XML mapping
    constructXMLMapping(rt);

    // parse the XML file and delete the instances that were created in
    // constructXMLmapping()
    SingularParser::Instance().parse(rt);

    // TODO: Additional parsing.. may need to restructure this section
    // in the future
    const char* node_name = OPP_Global::findNetNodeModule(rt)->name();

    for(size_t ifIndex = 0; ifIndex < rt->interfaceCount(); ifIndex++)
    {
      size_t numOfPrefixes = 0;
      PrefixEntry* entries = SingularParser::Instance().prefixes(node_name, ifIndex, numOfPrefixes);

      Interface6Entry& ie = rt->getInterfaceByIndex(ifIndex);
      ie.rtrVar.advPrefixList.resize(numOfPrefixes);

      if(entries)
      {
        for (size_t i = 0; i < numOfPrefixes; i++)
          ie.rtrVar.advPrefixList[i] = entries[i];

        delete[] entries;
      }

      size_t numOfAddrs = 0;
      IPv6Address* addrs = SingularParser::Instance().addresses(rt, node_name, ifIndex, numOfAddrs);

      if(addrs)
      {
        for(size_t j = 0; j < numOfAddrs; j ++)
        {
          // All addrs have to go through dup addr detection Sec 5.4 RFC 2462
          ie.tentativeAddrs.add(new IPv6Address(addrs[j]));
        }

        delete[] addrs;
      }
    }

    // do an additional parametre check
    checkValidData (rt);
  }

  /**
     construct mapping between the parametres and the XML
     tagname. When there is a new parametre, this is the first place
     to set up the mapping to be parsed from the XML file

     @warning The line in func below: "p_Iface[i].attrs[0].setParam(
     &(rt->getInterfaceByIndex(i).iface_name), TYPE_STRING, "name");" is
     dangerous because 1 we are taking the address of string.  We should always
     take a copy of it.  Otherwise we could just use char*.  2.  because if we
     try to 'fix' NetParam by deleting element we will be deleting something
     which was not allocated dynamically in some cases (perhaps in all case?)
     3.  This is inconsistent with stuff elsewhere.  It is just lucky that
     iface_name is valid during these operations and NetParm::element is never
     deleted. In other cases element should be deleted.  Well it works now
     because we allow memory leaks.
  */
  void IPv6XMLManager::constructXMLMapping(RoutingTable6* rt)
  {
    size_t numOfIfaces = rt->interfaceCount();

    // Router configuration parametres mapping
    //NetParam* p_advSendAds = new NetParam[numOfIfaces];
//    NetParam* p_maxRtrAdvInt = new NetParam[numOfIfaces];
//    NetParam* p_minRtrAdvInt = new NetParam[numOfIfaces];
    NetParam* p_advManaged = new NetParam[numOfIfaces];
    NetParam* p_advOther = new NetParam[numOfIfaces];
    NetParam* p_advLinkMTU = new NetParam[numOfIfaces];
    NetParam* p_advReachableTime = new NetParam[numOfIfaces];
    NetParam* p_advRetransTmr = new NetParam[numOfIfaces];
    NetParam* p_advCurHopLimit = new NetParam[numOfIfaces];
    NetParam* p_advDefaultLifetime = new NetParam[numOfIfaces];

    // Host configuration parametres mapping
    NetParam* p_HostLinkMTU = new NetParam[numOfIfaces];
    NetParam* p_HopLimit = new NetParam[numOfIfaces];
    NetParam* p_HostBaseReachableTime = new NetParam[numOfIfaces];
    NetParam* p_HostRetransTimer = new NetParam[numOfIfaces];
    NetParam* p_HostDupAddrDetectTransmits = new NetParam[numOfIfaces];

    // Additional interface parametres mapping
    NetParam* p_Iface = new NetParam[numOfIfaces];

    // excluding loopback interface which is interfaces[0]
    for(size_t i=0; i<numOfIfaces; i++)
    {
//       p_advSendAds[i].setParam(
//         &(rt->getInterfaceByIndex(i).rtrVar.advSendAds), TYPE_BOOL,
//         "AdvSendAdvertisements");

//      p_maxRtrAdvInt[i].setParam(
//        &(rt->getInterfaceByIndex(i).rtrVar.maxRtrAdvInt), TYPE_DOUBLE,
//        "MaxRtrAdvInterval");

//      p_minRtrAdvInt[i].setParam(
//        &(rt->getInterfaceByIndex(i).rtrVar.minRtrAdvInt), TYPE_DOUBLE,
//        "MinRtrAdvInterval");

      p_advManaged[i].setParam(
        &(rt->getInterfaceByIndex(i).rtrVar.advManaged), TYPE_BOOL,
        "AdvManagedFlag");

      p_advOther[i].setParam(
        &(rt->getInterfaceByIndex(i).rtrVar.advOther), TYPE_BOOL,
        "AdvOtherConfigFlag");

      p_advLinkMTU[i].setParam(
        &(rt->getInterfaceByIndex(i).rtrVar.advLinkMTU), TYPE_INT,
        "AdvLinkMTU");

      p_advReachableTime[i].setParam(
        &(rt->getInterfaceByIndex(i).rtrVar.advReachableTime), TYPE_INT,
        "AdvReachableTime");

      p_advRetransTmr[i].setParam(
        &(rt->getInterfaceByIndex(i).rtrVar.advRetransTmr), TYPE_INT,
        "AdvRetransTimer");

      p_advCurHopLimit[i].setParam(
        &(rt->getInterfaceByIndex(i).rtrVar.advCurHopLimit), TYPE_INT,
        "AdvCurHopLimit");

      p_advDefaultLifetime[i].setParam(
        &(rt->getInterfaceByIndex(i).rtrVar.advDefaultLifetime), TYPE_INT,
        "AdvDefaultLifetime");

      p_HostLinkMTU[i].setParam(
        &(rt->getInterfaceByIndex(i).mtu), TYPE_INT,
        "HostLinkMTU");

      p_HopLimit[i].setParam(
        &(rt->getInterfaceByIndex(i).curHopLimit), TYPE_INT,
        "HostCurHopLimit");

      p_HostBaseReachableTime[i].setParam(
        &(rt->getInterfaceByIndex(i).baseReachableTime), TYPE_INT,
        "HostBaseReachableTime");

      p_HostRetransTimer[i].setParam(
        &(rt->getInterfaceByIndex(i).retransTimer), TYPE_INT,
        "HostRetransTimer");

      p_HostDupAddrDetectTransmits[i].setParam(
        &(rt->getInterfaceByIndex(i).dupAddrDetectTrans), TYPE_INT,
        "HostDupAddrDetectTransmits");

      p_Iface[i].setParam(0, TYPE_NONE, "interface");

      // attribute of the parametre, Iface
      p_Iface[i].numOfAttrs = 1;
      p_Iface[i].attrs = new NetParam[1];
      p_Iface[i].attrs[0].setParam(
        &(rt->getInterfaceByIndex(i).iface_name), TYPE_STRING, "name");
    }

    //SingularParser::Instance().addEntry(p_advSendAds);
//    SingularParser::Instance().addEntry(p_maxRtrAdvInt);
//    SingularParser::Instance().addEntry(p_minRtrAdvInt);
    SingularParser::Instance().addEntry(p_advManaged);
    SingularParser::Instance().addEntry(p_advOther);
    SingularParser::Instance().addEntry(p_advLinkMTU);
    SingularParser::Instance().addEntry(p_advReachableTime);
    SingularParser::Instance().addEntry(p_advRetransTmr);
    SingularParser::Instance().addEntry(p_advCurHopLimit);
    SingularParser::Instance().addEntry(p_advDefaultLifetime);

    SingularParser::Instance().addEntry(p_HostLinkMTU);
    SingularParser::Instance().addEntry(p_HopLimit);
    SingularParser::Instance().addEntry(p_HostBaseReachableTime);
    SingularParser::Instance().addEntry(p_HostRetransTimer);
    SingularParser::Instance().addEntry(p_HostDupAddrDetectTransmits);

    SingularParser::Instance().addEntry(p_Iface);
  }


/**
 * Parses node level attributes
 *
 */

void IPv6XMLManager::parseNodeAttributes(RoutingTable6* rt)
{
  DOM_Node netNode;

  if(!SingularParser::Instance().findNetNode(rt->nodeName(), netNode))
    return;

  if (SingularParser::Instance()._version < 3 || MyString(static_cast<DOM_Element&>(netNode).
               getAttribute(DOMString("routePackets")).transcode()).get()
      != XML_ON)
    rt->IPForward = false;
  else
    rt->IPForward = true;

  if (SingularParser::Instance()._version < 2 || MyString(static_cast<DOM_Element&>(netNode).
               getAttribute(DOMString("forwardSitePackets")).transcode()).get()
      == XML_ON)
    rt->forwardSitePacket = true;
  else
    rt->forwardSitePacket = false;

  rt->setODAD(MyString(static_cast<DOM_Element&>(netNode).
                       getAttribute(DOMString("optimisticDAD")).transcode()).get()
              == XML_ON);

  if (rt->odad())
    Dout(dc::notice|flush_cf, rt->nodeName()<<" ODAD is on");

#ifdef USE_MOBILITY

  if (SingularParser::Instance()._version < 3 || MyString(static_cast<DOM_Element&>(netNode).
               getAttribute(DOMString("mobileIPv6Support")).transcode()).get()
      != XML_ON)
    rt->mipv6Support = false;
  else
    rt->mipv6Support = true;

  if (rt->mobilitySupport() && SingularParser::Instance()._version >= 3)
  {
    if (MyString(static_cast<DOM_Element&>(netNode).
                 getAttribute(DOMString("mobileIPv6Role")).transcode()).get() == string("HomeAgent"))
      rt->role = RoutingTable6::HOME_AGENT;
    else if (MyString(static_cast<DOM_Element&>(netNode).
                      getAttribute(DOMString("mobileIPv6Role")).transcode()).get() == string("MobileNode"))
      rt->role = RoutingTable6::MOBILE_NODE;
    else
      rt->role = RoutingTable6::CORRESPONDENT_NODE;
    
  }
  else
    rt->role = RoutingTable6::CORRESPONDENT_NODE;

  IPv6Mobility* mob = boost::polymorphic_downcast<IPv6Mobility*>
    (OPP_Global::findModuleByName(rt, "mobility"));

  if (MyString(static_cast<DOM_Element&>(netNode).
                 getAttribute(DOMString("routeOptimisation")).transcode()).get() == XML_ON)
  {
    assert(mob);
    Dout(dc::notice, rt->nodeName()<<" Route Optimisation is true");
    mob->setRouteOptimise(true);
  }
  else
    mob->setRouteOptimise(false);

  // return routability procedure option

  if (MyString(static_cast<DOM_Element&>(netNode).
                 getAttribute(DOMString("returnRoutability")).transcode()).get() == XML_ON)
  {
    assert(mob);
    Dout(dc::notice, rt->nodeName()<<" Return Routability Procedure is true");
    mob->setReturnRoutability(true);
  }
  else
    mob->setReturnRoutability(false);

  // care-of Test only option in return routability procedure

  if (MyString(static_cast<DOM_Element&>(netNode).
                 getAttribute(DOMString("careofTestOnly")).transcode()).get() == XML_ON)
  {
    assert(mob);
    Dout(dc::notice, rt->nodeName()<<" Care-of Test Only is true");
    mob->setCareofTestOnly(true);
  }
  else
    mob->setCareofTestOnly(false);

  // direct signaling option in return routability procedure

  if (MyString(static_cast<DOM_Element&>(netNode).
                 getAttribute(DOMString("directSignaling")).transcode()).get() == XML_ON)
  {
    assert(mob);
    Dout(dc::notice, rt->nodeName()<<" Direct Signaling is true");
    mob->setDirectSignaling(true);
  }
  else
    mob->setDirectSignaling(false);
  
#ifdef USE_HMIP

 if (SingularParser::Instance()._version < 5 || MyString(static_cast<DOM_Element&>(netNode).
               getAttribute(DOMString("hierarchicalMIPv6Support")).transcode()).get()
      != XML_ON)
    rt->hmipv6Support = false;
  else if (rt->mobilitySupport())
    rt->hmipv6Support = true;
  else
  {
    cerr << "HMIP Support cannot be activated.\n"
         << "Please set mobility support options before trying again\n";
    exit(1);
  }

  if (rt->mobilitySupport() && SingularParser::Instance()._version >= 3)
  {
    if (MyString(static_cast<DOM_Element&>(netNode).
                 getAttribute(DOMString("map")).transcode()).get() == XML_ON)
    {
      if ( rt->role == RoutingTable6::HOME_AGENT)
      {
        rt->mapSupport = true;
      }
      else
      {
        cerr << "MAP support cannot be activated without HA support.\n"
             << "Please check mobileIPv6Support to make sure it is HomeAgent\n";
        exit(1);
      }
    }
    else
      rt->mapSupport = false;
  }
  else
    rt->role = RoutingTable6::CORRESPONDENT_NODE;


#endif //USE_HMIP

#endif // USE_MOBILITY
}

} //namespace XMLConfiguration

#if defined USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>



/**
   @class IPv6XMLParserTest
   @brief Unit Test for IPv6XMLParser
   @ingroup TestCases
*/

class XMLConfiguration::IPv6XMLParserTest: public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( IPv6XMLParserTest );
  CPPUNIT_TEST( testXML );
  CPPUNIT_TEST_SUITE_END();
public:

  // Constructor/destructor.
  IPv6XMLParserTest();
  void setUp();
  void tearDown();
  void testXML();
private:

  // Unused ctor and assignment op.
  IPv6XMLParserTest(const IPv6XMLParserTest&);
  IPv6XMLParserTest& operator=(const IPv6XMLParserTest&);
};

CPPUNIT_TEST_SUITE_REGISTRATION( XMLConfiguration::IPv6XMLParserTest );

#include <boost/scoped_array.hpp>

namespace XMLConfiguration
{

IPv6XMLParserTest::IPv6XMLParserTest()
{}

void IPv6XMLParserTest::setUp()
{
}

void IPv6XMLParserTest::tearDown()
{
}

void IPv6XMLParserTest::testXML()
{
  typedef boost::scoped_array<char> MyString;
  const string XML_ON = "on";

  //DOMString is "" when the attribute does not exist.  However for retrieving
  //attributes of nonexistant nodes I don't know whether it also returns "".  It
  //does not throw an exception as before versioning was introduced the
  //tunnelConf and source routes did not "appear" to create visible problems
  DOMString ver = IPv6XMLManager::SingularParser::Instance()._XMLDoc->doc().getDocumentElement().getAttribute(DOMString("version"));
  ver.print();

  MyString test(ver.transcode());

  cerr <<"My String "<<test.get() << " strlen="<< strlen(test.get())<<endl;
  cerr <<"proper len "<<ver.length()<<" ";
  copy(&test[0], &test[0] + ver.length(), ostream_iterator<char>(cerr));
  cerr <<endl;
  string ver_str(test.get());
  CPPUNIT_ASSERT(ver_str.length() == ver.length());


  //Testcase of XML parsing
  if (IPv6XMLManager::SingularParser::Instance()._filename == std::string("XMLTEST.xml"))
  {
    CPPUNIT_ASSERT(test.get() == std::string("0.0.3"));

    DOM_Node netNode;

///Check router attributes
    CPPUNIT_ASSERT(IPv6XMLManager::SingularParser::Instance().findNetNode("router", netNode));

    CPPUNIT_ASSERT(MyString(static_cast<DOM_Element&>(netNode).
                    getAttribute(DOMString("routePackets")).transcode()).get()
           == XML_ON);


    CPPUNIT_ASSERT(MyString(static_cast<DOM_Element&>(netNode).
                    getAttribute(DOMString("forwardSitePackets")).transcode()).get()
           == XML_ON);

    CPPUNIT_ASSERT(MyString(static_cast<DOM_Element&>(netNode).
               getAttribute(DOMString("mobileIPv6Support")).transcode()).get()
           == XML_ON);

    CPPUNIT_ASSERT(MyString(static_cast<DOM_Element&>(netNode).
                    getAttribute(DOMString("mobileIPv6Role")).transcode()).get()
           == std::string("HomeAgent"));

  }

}
}
#endif //USE_CPPUNIT

