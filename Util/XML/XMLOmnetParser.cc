// -*- C++ -*-
// Copyright (C) 2004 Johnny Lai
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
 * @file   XMLOmnetParser.cc
 * @author Johnny Lai
 * @date   09 Sep 2004
 *
 * @brief  Implementation of XMLOmnetParser
 *
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include <iostream>
#include <sstream>
#include <string>
#include "opp_utils.h"  // for int/double <==> string conversions
#include <omnetpp.h>

#include "XMLOmnetParser.h"
#include "XMLCommon.h"

#include "RoutingTable6.h"
#include "opp_utils.h"
#include "IPv6Encapsulation.h"
#include "IPv6ForwardCore.h"
#include "IPv6CDS.h"
#include "MACAddress6.h"

#ifdef USE_MOBILITY
#include "WirelessEtherModule.h"
#include "IPv6Mobility.h"
#include "MobilityStatic.h"
#include "MobilityRandomWP.h"
#include "MobilityRandomPattern.h"
#include "MobileEntity.h"
#endif

#ifdef USE_HMIP
#include "NeighbourDiscovery.h"
#include "HMIPv6ICMPv6NDMessage.h"
#include "HMIPv6NDStateRouter.h"
#include "HMIPv6Entry.h"
#endif // USE_HMIP

using std::cerr;
using std::endl;

typedef cXMLElementList::iterator NodeIt;

namespace
{
  const char* stripWhitespace(const char* r)
  {
    return r;
  }
}

namespace XMLConfiguration
{

XMLOmnetParser::XMLOmnetParser():
  _filename(""), _version(0), root(0)
{
}

XMLOmnetParser::~XMLOmnetParser()
{
  //Do not need to delete root because if we do double deletions will occur as
  //omnet will clean up too
}

void XMLOmnetParser::parseFile(const char* filename)
{
  if(!filename || '\0' == filename[0])
  {
    cerr << "Empty name for XML input file"<<endl;
    exit(1);
  }
  _filename = filename;
  root = ev.getXMLDocument(filename);
}

std::string XMLOmnetParser::getNodeProperties(const cXMLElement* netNode, const char* attrName, bool required) const
{
  assert(netNode);
  const char* s = netNode->getAttribute(attrName);
  if (s == 0)
  {
    if (required)
      Dout(dc::xml_addresses|flush_cf , "xml element "<<netNode->getTagName()<<" does not have attribute "
           <<attrName);
    return string();
  }
  else
    Dout(dc::xml_addresses|flush_cf, "read value = "<< s <<" "<<attrName);

  return string(s);
}

bool XMLOmnetParser::getNodePropBool(const cXMLElement* netNode, const char* attrName)
{
  return getNodeProperties(netNode, attrName, false) == XML_ON;
}

cXMLElement* XMLOmnetParser::getNetNode(const char* name) const
{
  cXMLElement* n = root->getFirstChildWithAttribute("local", "node", name);
  if (!n)
    Dout(dc::debug|flush_cf, name<<" No XML config found!");
  return n;
}

std::string XMLOmnetParser::retrieveDebugChannels()
{
  return getNodeProperties(doc(), "debugChannel", false);
}


unsigned int  XMLOmnetParser::version() const
{
  if (!_version)
  {
    int major, minor, revision;
    major = minor = revision = 0;
    char c = '.';

    std::string v = getNodeProperties(doc(), "version");
    //const char* v = (*(root().get_attributes().find("version"))).get_value();
    std::istringstream(v)>>major>>c>>minor>>c>>revision;
    //gnuc compiler bug?
    //std::istringstream((*(root().get_attributes().find("version"))).get_value())>>major>>c>>minor>>c>>revision;
    _version = (major<<16) + (minor<<8) + revision;
    Dout(dc::xml_addresses, "major="<<major<<" minor="<<minor<<" rev="<<revision);
  }
  return _version;
}

void XMLOmnetParser::staticRoutingTable(RoutingTable6* rt)
{
  cXMLElement* ne = getNetNode(rt->nodeName());
  if (!ne)
  {
    Dout(dc::warning, rt->nodeName()<<" no XML configuration found");
    return;
  }
  else
    Dout(dc::xml_addresses|flush_cf, rt->nodeName()<<" in staticRoutingTable");

  const cXMLElement* nroute = ne->getElementByPath("./route");
  if (!nroute)
  {
    Dout(dc::debug, rt->nodeName()<<" does not have static routes in XML");
    return;
  }

  std::string iface, dest, nextHop;
  cXMLElementList el = nroute->getChildrenByTagName("routeEntry");
  for (NodeIt it = el.begin(); it != el.end(); it++)
  //for (cXMLElement* nre = nroute->getFirstChild(); nre != el.end(); )
  {
    const cXMLElement* nre = *it;
    assert(!strcmp(nre->getTagName(), "routeEntry"));
        iface = getNodeProperties(nre, "routeIface");
    dest = getNodeProperties(nre, "routeDestination");
    nextHop = getNodeProperties(nre, "routeNextHop", false);
    bool destIsHost = getNodeProperties(nre, "isRouter") != XML_ON;

    Dout(dc::debug, rt->nodeName()<<"iface="<<iface<<"dest="<<dest<<"nextHop="<<nextHop
         <<"destIsHost is "<<destIsHost);
    if (dest == "")
    {
      cerr <<  "Node: "<< rt->nodeName()
           <<" -- Need to specify a destination address in static routing entry"
           <<endl;
      exit(1);
    }

    int ifaceIdx = rt->interfaceNameToNo(iface.c_str());
    if(ifaceIdx == -1)
    {
      cerr << "Node: "<< rt->nodeName()
           <<" Route has incorrect interface identifier in static routing table... "
           <<"(" << iface << ")" << endl;
      exit(1);
    }
    IPv6Address nextHopAddr(nextHop.c_str());
    IPv6Address destAddr(dest.c_str());
    //Ensure dest does not have a suffix?
    destAddr.truncate();

    if (nextHop == "")
      assert(nextHopAddr == IPv6_ADDR_UNSPECIFIED);

    rt->addRoute(ifaceIdx, nextHopAddr, destAddr, destIsHost);

  }//end for loop of routeEntries

  if (_version >= 2)
    tunnelConfiguration(rt);

}


void XMLOmnetParser::tunnelConfiguration(RoutingTable6* rt)
{
  using namespace OPP_Global;
  string iface, tunEntry, tunExit, destination;
  ipv6_addr src, dest;
  size_t vifIndex = 0;

  IPv6Encapsulation* tunMod = polymorphic_downcast<IPv6Encapsulation*>
    (findModuleByType(rt, "IPv6Encapsulation"));

  //Even though downcast detects incorrect downcasts it still allows casting 0
  //down to anything
  assert(tunMod != 0);

  cXMLElement* netNode = getNetNode(rt->nodeName());

  cXMLElement* ntun = netNode->getElementByPath("./tunnel");
  if (!ntun)
  {
    Dout(dc::debug|dc::encapsulation, rt->nodeName()<<" has no XML tunneling configuration");
    return;
  }
  else
    Dout(dc::xml_addresses|flush_cf, rt->nodeName()<<" in tunnelConfiguration");

  cXMLElementList tunList = netNode->getChildrenByTagName("tunnelEntry");

  for (NodeIt it = tunList.begin();it != tunList.end();it++)
  {
    cXMLElement* nte = *it;
    iface = getNodeProperties(nte, "exitIface");
    int ifaceIdx = rt->interfaceNameToNo(iface.c_str());

    if(ifaceIdx == -1)
    {
      Dout(dc::encapsulation|dc::warning, "Node: "<< rt->nodeName() <<
           " TunnelEntry has incorrect interface identifier in XML configuration file ..."
           <<"(" << iface << ")"<<" IGNORING");
      assert(ifaceIdx != -1);
      continue;
    }

    tunEntry = getNodeProperties(nte, "entryPoint");
    tunExit = getNodeProperties(nte, "exitPoint");

    src = c_ipv6_addr(tunEntry.c_str());
    dest = c_ipv6_addr(tunExit.c_str());
    vifIndex = tunMod->createTunnel(src, dest, ifaceIdx);
    if (vifIndex == 0)
    {
      cerr<<rt->nodeName()<<" tunnel not created from XML src="<<src<<endl
          <<" dest="<<dest<<" exitIface="<<dec<<ifaceIdx<<endl;
      continue;
    }

    Dout(dc::encapsulation|continued_cf, rt->nodeName()
         <<" read tunnel config: src="<<src<<" dest="<<dest<<" exitIface="<<dec
         <<ifaceIdx);
    cXMLElementList trigs = nte->getChildrenByTagName("triggers");
    for ( NodeIt trigIt = trigs.begin();trigIt != trigs.end();trigIt++)
    {
      cXMLElement* ntrig = *trigIt;
      bool associatedTrigger = false;
      destination = getNodeProperties(ntrig, "destination");
      IPv6Address dest(destination.c_str());
      if (dest.prefixLength() == 0 || dest.prefixLength() == IPv6_ADDR_LENGTH)
      {
        ipv6_addr trigdest=c_ipv6_addr(destination.c_str());
        associatedTrigger = tunMod->tunnelDestination(trigdest, vifIndex);
        Dout(dc::continued, " trigger dest="<<trigdest);
      }
      else
      {
        associatedTrigger = tunMod->tunnelDestination(dest, vifIndex);
        Dout(dc::continued, " trigger dest="<<dest);
      }
      if (!associatedTrigger)
      {
        Dout(dc::warning, "Unable to use dest="<<destination<<" to trigger tunnel="
             <<vifIndex);
        cerr<<"Unable to use dest="<<destination<<" to trigger tunnel="
            <<vifIndex<<endl;
      }
    } //end for triggers
    Dout(dc::finish, "-|");

  } //end for tunnelEntries

  sourceRoute(rt);

}

void XMLOmnetParser::sourceRoute(RoutingTable6* rt)
{
  using namespace OPP_Global;
  IPv6ForwardCore* forwardMod = polymorphic_downcast<IPv6ForwardCore*>
    (findModuleByTypeDepthFirst(rt, "IPv6ForwardCore"));
  //Even though downcast detects incorrect downcasts it still allows casting 0
  //down to anything
  assert(forwardMod != 0);

  cXMLElement* netNode = getNetNode(rt->nodeName());

  cXMLElement* nsource = netNode->getElementByPath("./sourceRoute");
  if (!nsource)
  {
    Dout(dc::debug|dc::forwarding, "No source routes for "<<rt->nodeName()
         <<" in XML config");
    return;
  }
  else
    Dout(dc::xml_addresses|flush_cf, rt->nodeName()<<" in sourceRoute");

  cXMLElementList sres = nsource->getChildrenByTagName("sourceRouteEntry");
  for (NodeIt it = sres.begin();it != sres.end();it++)
  {
    cXMLElement* nsre = *it;
    cXMLElementList nextHops = nsre->getChildrenByTagName("nextHop");
    size_t nextHopCount = nextHops.size();
    SrcRoute route(new _SrcRoute(nextHopCount + 1, IPv6_ADDR_UNSPECIFIED));
    (*route)[nextHopCount] = c_ipv6_addr(getNodeProperties(nsre, "finalDestination").c_str());

    NodeIt nextIt = nextHops.begin();
    for ( size_t j = 0 ; j < nextHopCount; nextIt++, j++)
    {
      cXMLElement* nnh = *nextIt;
      (*route)[j] = c_ipv6_addr(getNodeProperties(nnh, "address").c_str());
    }
    forwardMod->addSrcRoute(route);
  }

}

void XMLOmnetParser::parseNetworkEntity(RoutingTable6* rt)
{
  static bool printXMLFile = false;
  if (!printXMLFile)
  {
    Dout(dc::notice, "XML Configuration file is "<<_filename);
    printXMLFile = true;
  }

  //const xml::node ne = *getNetNode(rt->nodeName());
  cXMLElement* ne = getNetNode(rt->nodeName());
  if (!ne)
  {
    Dout(dc::warning, rt->nodeName()<<" no XML configuration found");
    return;
  }
  else
    Dout(dc::xml_addresses|flush_cf, rt->nodeName()<<" in parseNetworkEntity");

  parseNodeAttributes(rt, ne);

  //Parse per interface attribute
  cXMLElementList el = ne->getChildrenByTagName("interface");
  if (rt->interfaceCount() > el.size())
  {
    Dout(dc::notice, rt->nodeName()<<" rt->interfaceCount()="<<rt->interfaceCount()
         <<" while xml interface count is "<<el.size());
  }

  NodeIt startIf = el.begin();
  for(size_t iface_index = 0; iface_index < rt->interfaceCount(); iface_index++, startIf++)
  {
    if (startIf == el.end())
    {
      Dout(dc::warning|flush_cf, " No more interfaces found in XML for ifIndex>="<<iface_index);
      //Should fill in for rest of unspecified interfaces the defaults in
      //global element when we get that happening.
      break;
    }
    cXMLElement* nif = *startIf;

    parseInterfaceAttributes(rt, nif, iface_index);
  }

  // do an additional parameter check
  checkValidData (rt);
}

/// parse node level attributes
void XMLOmnetParser::parseNodeAttributes(RoutingTable6* rt, cXMLElement* ne)
{
  rt->IPForward = getNodeProperties(ne, "routePackets") == XML_ON;
  rt->forwardSitePacket = getNodeProperties(ne, "forwardSitePackets") == XML_ON;
  rt->setODAD(getNodeProperties(ne, "optimisticDAD") == XML_ON);
  if (rt->odad())
    Dout(dc::notice|flush_cf, rt->nodeName()<<" ODAD is on");

#ifdef USE_MOBILITY
  rt->mipv6Support = getNodeProperties(ne, "mobileIPv6Support") == XML_ON;
  if (rt->mobilitySupport() && version() >= 3)
  {
    if (getNodeProperties(ne,"mobileIPv6Role") == string("HomeAgent"))
      rt->role = RoutingTable6::HOME_AGENT;
    else if (getNodeProperties(ne,"mobileIPv6Role") == string("MobileNode"))
      rt->role = RoutingTable6::MOBILE_NODE;
    else
      rt->role = RoutingTable6::CORRESPONDENT_NODE;
  }
  else
    rt->role = RoutingTable6::CORRESPONDENT_NODE;

  IPv6Mobility* mob = boost::polymorphic_downcast<IPv6Mobility*>
    (OPP_Global::findModuleByName(rt, "mobility"));

  if (getNodeProperties(ne, "routeOptimisation") == XML_ON)
  {
    assert(mob);
    Dout(dc::notice, rt->nodeName()<<" Route Optimisation is true");
    mob->setRouteOptimise(true);
  }
  else
    mob->setRouteOptimise(false);

  if (getNodeProperties(ne, "returnRoutability") == XML_ON)
  {
    Dout(dc::notice, rt->nodeName()<<" Return Routability Procedure is true");
    mob->setReturnRoutability(true);
  }
  else
    mob->setReturnRoutability(false);

  if (getNodeProperties(ne, "earlyBU") == XML_ON)
  {
    Dout(dc::notice, rt->nodeName()<<" Early BU is true");
    mob->setEarlyBindingUpdate(true);
  }
  else
    mob->setEarlyBindingUpdate(false);

  if (getNodeProperties(ne, "directSignaling") == XML_ON)
  {
    Dout(dc::notice, rt->nodeName()<<" Direct Signaling is true");
    mob->setDirectSignaling(true);
  }
  else
    mob->setDirectSignaling(false);

#ifdef USE_HMIP

 if (version() < 5 || getNodeProperties(ne, "hierarchicalMIPv6Support") != XML_ON)
    rt->hmipv6Support = false;
  else if (rt->mobilitySupport())
    rt->hmipv6Support = true;
  else
  {
    cerr << "HMIP Support cannot be activated without mobility support.\n";
    exit(1);
  }

  if (rt->mobilitySupport() && version() >= 3)
  {
    if (getNodeProperties(ne, "map") == XML_ON)
    {
      if ( rt->role == RoutingTable6::HOME_AGENT)
      {
        rt->mapSupport = true;
      }
      else
      {
        cerr << "MAP support cannot be activated without HomeAgent support.\n";
        exit(1);
      }
    }
    else
      rt->mapSupport = false;
  }
  else
    rt->role = RoutingTable6::CORRESPONDENT_NODE;

#if EDGEHANDOVER
  string ehType;
  if (rt->hmipSupport() && rt->isMobileNode())
    ehType = getNodeProperties(ne, "edgeHandoverType", false);
  mob->setEdgeHandoverType(ehType);
  if (ehType != "")
  {
    Dout(dc::xml_addresses|dc::debug, rt->nodeName()<<" EH type "<<ehType);
    assert(mob->edgeHandover());
  }
#endif //EDGEHANDOVER

#endif //USE_HMIP

#endif // USE_MOBILITY

}


void XMLOmnetParser::parseInterfaceAttributes(RoutingTable6* rt, cXMLElement* nif, unsigned int iface_index)
{
  Interface6Entry& ie = rt->getInterfaceByIndex(iface_index);
  Interface6Entry::RouterVariables& rtrVar = ie.rtrVar;

  rtrVar.advSendAds = getNodeProperties(nif, "AdvSendAdvertisements") == XML_ON;
  rtrVar.maxRtrAdvInt = OPP_Global::atod(getNodeProperties(nif, "MaxRtrAdvInterval").c_str());
  rtrVar.minRtrAdvInt = OPP_Global::atod(getNodeProperties(nif, "MinRtrAdvInterval").c_str());

#ifdef USE_MOBILITY
  rtrVar.advHomeAgent = getNodeProperties(nif, "AdvHomeAgent") == XML_ON;
  if (rt->mipv6Support)
  {
    rtrVar.maxRtrAdvInt = OPP_Global::atod(getNodeProperties(nif, "MIPv6MaxRtrAdvInterval").c_str());
    rtrVar.minRtrAdvInt = OPP_Global::atod(getNodeProperties(nif, "MIPv6MinRtrAdvInterval").c_str());
    if (rt->isRouter())
      Dout(dc::notice, rt->nodeName()<<":"<<iface_index<<" minRtrAdv="
           <<rtrVar.minRtrAdvInt
           <<" maxRtrAdv="<<rtrVar.maxRtrAdvInt);
  }

  Interface6Entry::mipv6Variables& mipVar = ie.mipv6Var;
  mipVar.maxConsecutiveMissedRtrAdv = OPP_Global::atoul(getNodeProperties(nif, "MaxConsecMissRtrAdv").c_str());

#endif // USE_MOBILITY
#if FASTRA
/* XXX let exception pass through as with other similar lines --AV
  try
  {
*/
    rtrVar.maxFastRAS = OPP_Global::atoul(getNodeProperties(nif, "MaxFastRAS").c_str());
/* XXX let exception pass through as with other entries --AV
  }
  catch(boost::bad_lexical_cast& e)
  {
    Dout(dc::warning|error_cf,  rt->nodeName()<<" Cannot convert into integer maxRas="<<getNodeProperties(nif, "MaxFastRAS"));
  }
*/
  rtrVar.fastRA = rtrVar.maxFastRAS != 0;
  if (rt->isRouter())
    Dout(dc::notice, rt->nodeName()<<":"<<iface_index<<" fastRA="<<rtrVar.fastRA
         <<" maxFastRAS="<<rtrVar.maxFastRAS);
#endif //FASTRA

  rtrVar.advManaged = getNodeProperties(nif, "AdvManagedFlag") == XML_ON;
  rtrVar.advOther = getNodeProperties(nif, "AdvOtherConfigFlag") == XML_ON;
  rtrVar.advLinkMTU =  OPP_Global::atoul(getNodeProperties(nif, "AdvLinkMTU").c_str());
  rtrVar.advReachableTime =  OPP_Global::atoul(getNodeProperties(nif, "AdvReachableTime").c_str());
  rtrVar.advRetransTmr =  OPP_Global::atoul(getNodeProperties(nif, "AdvRetransTimer").c_str());
  rtrVar.advCurHopLimit =  OPP_Global::atoul(getNodeProperties(nif, "AdvCurHopLimit").c_str());
  //This is a hexadecimal number and as such is not covered by typical
  //stringstream smarts in conversion so lexical_cast won't work. Converted t
  //o decimal in schema now.
  rtrVar.advDefaultLifetime =  OPP_Global::atoul(getNodeProperties(nif, "AdvDefaultLifetime").c_str());
  ie.mtu =  OPP_Global::atoul(getNodeProperties(nif, "HostLinkMTU").c_str());
  ie.curHopLimit = OPP_Global::atoul(getNodeProperties(nif, "HostCurHopLimit").c_str());
  ie.baseReachableTime = OPP_Global::atoul(getNodeProperties(nif, "HostBaseReachableTime").c_str());
  ie.retransTimer = OPP_Global::atoul(getNodeProperties(nif, "HostRetransTimer").c_str());
#if FASTRS
  ie.maxRtrSolDelay = OPP_Global::atod(getNodeProperties(nif, "HostMaxRtrSolDelay").c_str());
#endif // FASTRS
  ie.dupAddrDetectTrans = OPP_Global::atoul(getNodeProperties(nif, "HostDupAddrDetectTransmits").c_str());

  ///Only some older XML files did not name their interfaces guess it is not
  ///really necessary as we do things in order anyway.
  ie.iface_name = getNodeProperties(nif, "name", false);

  cXMLElement* napl =  nif->getElementByPath("./AdvPrefixList");
  if (napl)
  {
    //Parse prefixes
    cXMLElementList apl = napl->getChildrenByTagName("AdvPrefix");
    size_t numOfPrefixes = apl.size();
    ie.rtrVar.advPrefixList.resize(numOfPrefixes);
    if (numOfPrefixes == 0)
      Dout(dc::xml_addresses|dc::warning, rt->nodeName()
           <<" no prefixes even though advPrefixList exists");
    NodeIt startPr = apl.begin();
    for(size_t j = 0; j < numOfPrefixes; j++, startPr++)
    {
      if (startPr == apl.end())
        assert(false);
      cXMLElement* npr = *startPr;
      PrefixEntry& pe = ie.rtrVar.advPrefixList[j];
      pe._advValidLifetime =  OPP_Global::atoul(getNodeProperties(npr, "AdvValidLifetime").c_str());
      pe._advOnLink =  getNodeProperties(npr, "AdvOnLinkFlag") == XML_ON;
      pe._advPrefLifetime =  OPP_Global::atoul(getNodeProperties(npr, "AdvPreferredLifetime").c_str());
      pe._advAutoFlag = getNodeProperties(npr, "AdvAutonomousFlag") == XML_ON;
#ifdef USE_MOBILITY
      // Router Address Flag - for mobility support
      pe._advRtrAddr = getNodeProperties(npr, "AdvRtrAddrFlag") == XML_ON;
      pe.setPrefix(stripWhitespace(npr->getNodeValue()), !pe._advRtrAddr);
#else
      pe.setPrefix(stripWhitespace(npr->getNodeValue()));
#endif //USE_MOBILITY
      Dout(dc::xml_addresses, "prefix advertised is "<<pe);
    }

  }
  //parse addresses
  cXMLElementList addrList = nif->getChildrenByTagName("inetAddr");
  size_t numOfAddrs = addrList.size();
  if (numOfAddrs > 0)
  {
    Dout(dc::xml_addresses|flush_cf|continued_cf, rt->nodeName()<<":"<<iface_index<<" ");
    NodeIt startAd = addrList.begin();
    for (size_t k = 0; k < numOfAddrs; k++,startAd++)
    {
      //startAd = find_if(startAd, nif.end(), findElementByName("inetAddr"));
      //startAd = nif.find("inetAddr", startAd);
      if (startAd == addrList.end())
        assert(false);
      cXMLElement* nad = *startAd;
      ie.tentativeAddrs.push_back(IPv6Address(stripWhitespace(nad->getNodeValue())));
      Dout(dc::continued, "address "<< k << " is "<<stripWhitespace(nad->getNodeValue())<<" ");
    }
    Dout( dc::finish, "-|" );
  }
}


#ifdef USE_MOBILITY

void XMLOmnetParser::parseWirelessEtherInfo(WirelessEtherModule* wlanMod)
{
  assert(wlanMod);
  const char* nodeName = OPP_Global::findNetNodeModule(wlanMod)->name();

  cXMLElement* ne = getNetNode(nodeName);
  if (!ne)
    return;

  cXMLElement* nif = ne->getFirstChildWithAttribute("interface", "name", wlanMod->getInterfaceName());
  if (!nif)
  {
    Dout(dc::notice, nodeName<<":"<<wlanMod->getInterfaceName()<<"no config found for interface"
         <<wlanMod->getInterfaceName());
    return;
  }
  cXMLElement* nwei = nif->getElementByPath("./WirelessEtherInfo");
  if (!nwei)
  {
    Dout(dc::xml_addresses, "Make sure global WirelessEtherInfo option exists for "
         <<nodeName<<":"<< wlanMod->getInterfaceName()
         <<" as no interface specific WirelessEtherInfo element in XML file");
    return;
  }

  cXMLElement* info = nwei->getElementByPath("./WEInfo");
  if (!info)
  {
    Dout(dc::xml_addresses, "Make sure global WEInfo option is assigned for "
         <<nodeName<<":"<< wlanMod->getInterfaceName()
         <<" as no interface specific WEInfo element in XML file");
    return;
  }

  parseWEInfo(wlanMod, info);
}

void XMLOmnetParser::parseWEInfo(WirelessEtherModule* wlanMod, cXMLElement* info)
{
  wlanMod->ssid = getNodeProperties(info, "WEssid");
  const_cast<double&>(wlanMod->pLExp) = OPP_Global::atod(getNodeProperties(info, "WEPathLossExponent").c_str());

  const_cast<double&>(wlanMod->pLStdDev) = OPP_Global::atod(getNodeProperties(info, "WEPathLossStdDev").c_str());
  const_cast<double&>(wlanMod->txpower) = OPP_Global::atod(getNodeProperties(info, "WETxPower").c_str());
  const_cast<double&>(wlanMod->threshpower) = OPP_Global::atod(getNodeProperties(info, "WEThresholdPower").c_str());
  const_cast<double&>(wlanMod->hothreshpower) = OPP_Global::atod(getNodeProperties(info, "WEHOThresholdPower").c_str());
  const_cast<double&>(wlanMod->probeEnergyTimeout) = OPP_Global::atod(getNodeProperties(info, "WEProbeEnergyTimeout").c_str());
  const_cast<double&>(wlanMod->probeResponseTimeout) = OPP_Global::atod(getNodeProperties(info, "WEProbeResponseTimeout").c_str());
  const_cast<double&>(wlanMod->authenticationTimeout) = OPP_Global::atod(getNodeProperties(info, "WEAuthenticationTimeout").c_str());
  const_cast<double&>(wlanMod->associationTimeout) = OPP_Global::atod(getNodeProperties(info, "WEAssociationTimeout").c_str());
  const_cast<unsigned int&>(wlanMod->maxRetry) = OPP_Global::atoul(getNodeProperties(info, "WERetry").c_str());
  wlanMod->fastActScan = getNodeProperties(info, "WEFastActiveScan") == XML_ON;
  wlanMod->scanShortCirc = getNodeProperties(info, "WEScanShortCircuit") == XML_ON;
  wlanMod->crossTalk = getNodeProperties(info, "WECrossTalk") == XML_ON;
  wlanMod->shadowing = getNodeProperties(info, "WEShadowing") == XML_ON;
  wlanMod->chanNotToScan = getNodeProperties(info, "WEChannelsNotToScan");
  const_cast<unsigned int&>(wlanMod->sSMaxSample) = OPP_Global::atoul(getNodeProperties(info, "WESignalStrengthMaxSample").c_str());

  std::string addr = getNodeProperties(info, "WEAddress");

  if(!wlanMod->apMode)
  {
    if(addr != "0")
      wlanMod->address.set(addr.c_str());
    //Only want to initialise the MN mac address once otherwise routing tables
    //are wrong if we generate another random number from the stream
    else if (wlanMod->address == MACAddress6())
    {
        MAC_address macAddr;
        macAddr.high = OPP_Global::generateInterfaceId() & 0xFFFFFF;
        macAddr.low = OPP_Global::generateInterfaceId() & 0xFFFFFF;
        wlanMod->address.set(macAddr);
    }
  }
    const_cast<double&>(wlanMod->bWRequirements) = OPP_Global::atod(getNodeProperties(info, "WEBandwidthRequirements").c_str());
    wlanMod->statsVec = getNodeProperties(info, "WERecordStatisticVector") == XML_ON;
    wlanMod->activeScan = getNodeProperties(info, "WEActiveScan") == XML_ON;
    const_cast<double&>(wlanMod->channelScanTime) = OPP_Global::atod(getNodeProperties(info, "WEChannelScanTime").c_str());
 // TODO: parse supported rate

}

void XMLOmnetParser::parseMovementInfo(MobilityStatic* mod)
{
  // superclass for the (L1) mobile module
  MobileEntity* me = mod->mobileEntity;

  // obtain movement info from XML
  const char* nodeName = OPP_Global::findNetNodeModule(mod)->name();
  cXMLElement* nmisc = doc()->getElementByPath("./misc");
  if (!nmisc)
  {
    Dout(dc::notice, "No misc element hence cannot parse movementInfo");
    return;
  }

  cXMLElement* nobj = nmisc->getElementByPath("./ObjectMovement");
  if (!nobj)
  {
    DoutFatal(dc::core, "Cannot find ObjectMovement in misc");
  }

  cXMLElement* nmovenode = nobj->getFirstChildWithAttribute("MovingNode", "NodeName", nodeName);
  if (!nmovenode)
  {
    Dout(dc::debug|dc::notice, "No movement info for node "<<nodeName);
    return;
  }

  me->setStartMovingTime(OPP_Global::atoul(getNodeProperties(nmovenode, "startTime").c_str()));

  cXMLElementList moves = nmovenode->getChildrenByTagName("move");
  for (NodeIt it = moves.begin(); it != moves.end(); it++)
  {
    cXMLElement* nmove = *it;
    me->addMove(OPP_Global::atoul(getNodeProperties(nmove, "moveToX").c_str()),
                OPP_Global::atoul(getNodeProperties(nmove, "moveToY").c_str()),
                OPP_Global::atod(getNodeProperties(nmove, "moveSpeed").c_str())
                );
  }
}

void XMLOmnetParser::parseRandomWPInfo(MobilityRandomWP* mod)
{
  // obtain movement info from XML
  const char* nodeName = OPP_Global::findNetNodeModule(mod)->name();
  std::string path = string("./misc/ObjectMovement/RandomMovement[@RWNodeName=\"") + nodeName + "\"]";
  cXMLElement* nmovenode = doc()->getElementByPath(path.c_str());

  if (!nmovenode)
  {
    Dout(dc::debug|dc::notice, "No movement info for node "<<nodeName);
    return;
  }

  mod->minX = OPP_Global::atoul(getNodeProperties(nmovenode, "RWMinX").c_str());
  mod->maxX = OPP_Global::atoul(getNodeProperties(nmovenode, "RWMaxX").c_str());
  mod->minY = OPP_Global::atoul(getNodeProperties(nmovenode, "RWMinY").c_str());
  mod->maxY = OPP_Global::atoul(getNodeProperties(nmovenode, "RWMaxY").c_str());
  mod->moveInterval = OPP_Global::atod(getNodeProperties(nmovenode, "RWMoveInterval").c_str());
  mod->minSpeed = OPP_Global::atod(getNodeProperties(nmovenode, "RWMinSpeed").c_str());
  mod->maxSpeed = OPP_Global::atod(getNodeProperties(nmovenode, "RWMaxSpeed").c_str());
  mod->pauseTime = OPP_Global::atod(getNodeProperties(nmovenode, "RWPauseTime").c_str());
  mod->distance = OPP_Global::atoul(getNodeProperties(nmovenode, "RWDistance").c_str());
}

void XMLOmnetParser::parseRandomPatternInfo(MobilityRandomPattern* mod)
{
  // obtain movement info from XML
  const char* nodeName = OPP_Global::findNetNodeModule(mod)->name();
  std::string path = "./misc/ObjectMovement/RandomPattern";
  cXMLElement* nrandompattern =  doc()->getElementByPath(path.c_str());

  if (!nrandompattern)
    DoutFatal(dc::core, "Cannot find RandomPattern in ObjectMovement");

  if ( !mod->isRandomPatternParsed )
  {
    mod->xSize = OPP_Global::atoul(getNodeProperties(nrandompattern, "RPXSize").c_str());
    mod->ySize = OPP_Global::atoul(getNodeProperties(nrandompattern, "RPYSize").c_str());
    mod->moveInterval = OPP_Global::atoul(getNodeProperties(nrandompattern, "RPMoveInterval").c_str());
    mod->minSpeed = OPP_Global::atod(getNodeProperties(nrandompattern, "RPMinSpeed").c_str());
    mod->maxSpeed = OPP_Global::atod(getNodeProperties(nrandompattern, "RPMaxSpeed").c_str());
    mod->distance = OPP_Global::atoul(getNodeProperties(nrandompattern, "RPDistance").c_str());
    mod->pauseTime = OPP_Global::atod(getNodeProperties(nrandompattern, "RPPauseTime").c_str());
  }

  cXMLElement* nmovenode = nrandompattern->getFirstChildWithAttribute("RPNode", "RPNodeName", nodeName);

  if (!nmovenode)
  {
    Dout(dc::debug|dc::notice, "No X/Y offsets of movement pattern info for node "<<nodeName);
    return;
  }

  mod->xOffset = OPP_Global::atoul(getNodeProperties(nmovenode, "RPXOffset").c_str());
  mod->yOffset = OPP_Global::atoul(getNodeProperties(nmovenode, "RPYOffset").c_str());
}

#ifdef USE_HMIP
void XMLOmnetParser::parseMAPInfo(RoutingTable6* rt)
{
  NeighbourDiscovery* ndmod = static_cast<NeighbourDiscovery*>(OPP_Global::findModuleByName(rt, "nd"));

  HierarchicalMIPv6::HMIPv6NDStateRouter* hmipRtr = dynamic_cast<HierarchicalMIPv6::HMIPv6NDStateRouter*>(ndmod->nd);

  if (!hmipRtr)
    return;

  cXMLElement* ne = getNetNode(rt->nodeName());
  if (!ne)
  {
    Dout(dc::warning, rt->nodeName()<<" no XML configuration found");
  }

  if (getNodeProperties(ne, "mapMode") == "Basic")
    hmipRtr->mode = HierarchicalMIPv6::HMIPv6NDStateRouter::modeBasic;
  else
    hmipRtr->mode = HierarchicalMIPv6::HMIPv6NDStateRouter::modeExtended;

  if (getNodeProperties(ne, "mapMNMAYSetRoCAAsSource") == XML_ON)
  {
    if (!hmipRtr->mode == HierarchicalMIPv6::HMIPv6NDStateRouter::modeBasic)
    {
      cerr << " Flag I MUST NOT be set if the R flag is not set -- section 4, hmipv6" << endl;
      exit(1);
    }
    hmipRtr->mnMAYSetRoCAAsSource = true;
  }
  else
    hmipRtr->mnMAYSetRoCAAsSource = false;

  if (getNodeProperties(ne, "mapMNMUSTSetRoCAAsSource") == XML_ON)
  {
    if (hmipRtr->mode == HierarchicalMIPv6::HMIPv6NDStateRouter::modeExtended)
    {
      cerr << " Flag P MUST NOT be set if the M flag is set -- section 4, hmipv6" << endl;
      exit(1);
    }
    hmipRtr->mnMUSTSetRoCAAsSource = true;
  }
  else
    hmipRtr->mnMUSTSetRoCAAsSource = false;

  if (getNodeProperties(ne, "mapReverseTunnel") == XML_ON)
    hmipRtr->reverseTunnel = true;
  else
    hmipRtr->reverseTunnel = false;

  cXMLElementList ifaces = ne->getChildrenByTagName("interface");
  NodeIt startIf = ifaces.begin();
  for(size_t i = 0; i < rt->interfaceCount() ; i++, ++startIf)
  {
    if (startIf == ifaces.end())
    {
      Dout(dc::warning|flush_cf, " No more interfaces found in XML for ifIndex>="<<i);
      return;
    }
    cXMLElement* nif = *startIf;
    cXMLElement* nmaplist = nif->getElementByPath("./AdvMAPList");
    if (!nmaplist)
    {
      Dout(dc::xml_addresses|dc::debug, rt->nodeName()<<":"<<i<<" does not have a map to advertise in XML");
      continue;
    }
    cXMLElementList nmaps = nmaplist->getElementsByTagName("AdvMAPEntry");
    for (NodeIt it = nmaps.begin(); it != nmaps.end(); it++)
    {
      cXMLElement* nmap = *it;
      unsigned int dist = OPP_Global::atoul(getNodeProperties(nmap, "AdvMAPDist").c_str());
      unsigned int pref = OPP_Global::atoul(getNodeProperties(nmap, "AdvMAPPref").c_str());
      unsigned int expires = OPP_Global::atoul(getNodeProperties(nmap, "AdvMAPValidLifetime").c_str());
      ipv6_addr map_addr = c_ipv6_addr(stripWhitespace(nmap->getNodeValue()));
      HierarchicalMIPv6::HMIPv6ICMPv6NDOptMAP mapOpt(dist, pref, expires, map_addr);
      mapOpt.iface_idx = i;

      hmipRtr->addMAP(mapOpt);

      Dout(dc::xml_addresses|dc::debug, rt->nodeName()<<":"<<i<<" advMap="<<HierarchicalMIPv6::HMIPv6MAPEntry(mapOpt));
    }
  }
}
#endif // USE_HMIP

#endif //USE_MOBILITY



}; //namespace XMLConfiguration





#if defined USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>


/**
   @class XMLOmnetParserTest
   @brief Unit test for        XMLOmnetParser
   @ingroup TestCases
*/

class XMLOmnetParserTest: public CppUnit::TestFixture
{

  CPPUNIT_TEST_SUITE( XMLOmnetParserTest );
  CPPUNIT_TEST( testOmnetParser );
  CPPUNIT_TEST_SUITE_END();

 public:

  // Constructor/destructor.
  XMLOmnetParserTest();
  virtual ~XMLOmnetParserTest();

  void testOmnetParser();

  void setUp();
  void tearDown();

private:

  // Unused ctor and assignment op.
  XMLOmnetParserTest(const XMLOmnetParserTest&);
  XMLOmnetParserTest& operator=(const XMLOmnetParserTest&);

  XMLConfiguration::XMLOmnetParser* p;
};

CPPUNIT_TEST_SUITE_REGISTRATION( XMLOmnetParserTest );

XMLOmnetParserTest::XMLOmnetParserTest()
{
}

XMLOmnetParserTest::~XMLOmnetParserTest()
{
}

void XMLOmnetParserTest::setUp()
{
}

void XMLOmnetParserTest::tearDown()
{
}

void XMLOmnetParserTest::testOmnetParser()
{

  using namespace XMLConfiguration;
  CPPUNIT_ASSERT(1==1);

  if (string("testnet") != simulation.systemModule()->name())
    return;

  p = new XMLConfiguration::XMLOmnetParser();
  p->parseFile("XMLTEST.xml");

  std::string ver = p->getNodeProperties(p->doc(), "version");
  cerr<<" ver="<<ver<<" length="<<ver.length()<<endl;

  CPPUNIT_ASSERT(ver == "0.0.5");

  assert(p->getNetNode("router") != 0);
  cXMLElement* ne = p->getNetNode("router");
  CPPUNIT_ASSERT(p->getNodeProperties(ne, "routePackets") == XML_ON);
  CPPUNIT_ASSERT(p->getNodeProperties(ne, "forwardSitePackets") == XML_ON);
  CPPUNIT_ASSERT(p->getNodeProperties(ne, "mobileIPv6Support") == XML_ON);
  CPPUNIT_ASSERT(p->getNodeProperties(ne, "hierarchicalMIPv6Support") != XML_ON);
  CPPUNIT_ASSERT(p->getNodeProperties(ne, "mobileIPv6Role") == "HomeAgent");

  cXMLElement* iface = ne->getElementByPath("interface[0]");

  cerr<<"1st iface "<<stripWhitespace(iface->getFirstChild()->getNodeValue())<<endl;
  CPPUNIT_ASSERT(stripWhitespace(iface->getFirstChild()->getNodeValue()) == string("fe80:0:0:0:260:97ff:0:5/64"));
  //cerr<<"next interface"<<stripWhitespace((*ne.find("interface", ++it))->getNodeValue())<<endl;
  CPPUNIT_ASSERT(stripWhitespace((iface->getNextSibling()->getElementByPath("AdvPrefixList/AdvPrefix"))->getNodeValue()) == string("FEC0:0:0:ABCD:0:0:0:0/64"));
  CPPUNIT_ASSERT(stripWhitespace((iface->getNextSibling()->getElementByPath("AdvPrefixList/AdvPrefix"))->getNodeValue()) == string("FEC0:0:0:ABCD:0:0:0:0/64"));
  //get next interface
  cXMLElement* iface2 = iface->getNextSibling();
  CPPUNIT_ASSERT(iface2);

  cXMLElement* apl = iface2->getElementByPath("./AdvPrefixList");
  CPPUNIT_ASSERT(apl);



  cXMLElement* nprefix = apl->getElementByPath("AdvPrefix");
  CPPUNIT_ASSERT(nprefix);

  //The line below is alternative to previous block and will cause segfault
  //because the temporary const iterator from the find() call is same as
  // nprefix's internal pointer? Thus we will always need to copy the
  //iterator to retain a copy of it after the unnamed temporary goes out of scope
  //to retain node. (Valgrind analysis proved this)

  //  cXMLElement* nprefix = *((*it).find("AdvPrefix")); //*it;
  CPPUNIT_ASSERT(stripWhitespace(nprefix->getNodeValue()) == string("FEC0:0:0:ABCD:0:0:0:0/64"));
  CPPUNIT_ASSERT(888888 == OPP_Global::atoul(p->getNodeProperties(nprefix, "AdvValidLifetime")).c_str());
  CPPUNIT_ASSERT(88888 == OPP_Global::atoul(p->getNodeProperties(nprefix, "AdvPreferredLifetime")).c_str());
}

#endif //defined USE_CPPUNIT

