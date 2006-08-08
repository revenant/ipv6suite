// -*- C++ -*-
// Copyright (C) 2004, 2006 Johnny Lai
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

#include "InterfaceTable.h"
#include "RoutingTable6.h"
#include "opp_utils.h"
#include "IPv6Encapsulation.h" // XXX WHY????? --AV
#include "IPv6Forward.h" // XXX WHY????? --AV
#include "IPv6CDS.h"
#include "MACAddress6.h"

#ifdef USE_MOBILITY
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
  _version(0), root(0)
{
}

XMLOmnetParser::~XMLOmnetParser()
{
  //Do not need to delete root because if we do double deletions will occur as
  //omnet will clean up too
}

void XMLOmnetParser::setDoc(cXMLElement *config)
{
  root = config;
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

void XMLOmnetParser::staticRoutingTable(InterfaceTable *ift, RoutingTable6 *rt)
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
      abort_ipv6suite();
    }

    InterfaceEntry *ie = ift->interfaceByName(iface.c_str());
    if(!ie)
    {
      cerr << "Node: "<< rt->nodeName()
           <<" Route has incorrect interface identifier in static routing table... "
           <<"(" << iface << ")" << endl;
      abort_ipv6suite();
    }
    int ifaceIdx = ie->outputPort();
    IPv6Address nextHopAddr(nextHop.c_str());
    IPv6Address destAddr(dest.c_str());
    //Ensure dest does not have a suffix?
    destAddr.truncate();

    if (nextHop == "")
      assert(nextHopAddr == IPv6_ADDR_UNSPECIFIED);

    rt->addRoute(ifaceIdx, nextHopAddr, destAddr, destIsHost);

  }//end for loop of routeEntries

  if (_version >= 2)
    tunnelConfiguration(ift, rt);

}


void XMLOmnetParser::tunnelConfiguration(InterfaceTable *ift, RoutingTable6 *rt)
{
  using namespace OPP_Global;
  string iface, tunEntry, tunExit, destination;
  ipv6_addr src, dest;
  size_t vifIndex = 0;

  IPv6Encapsulation* tunMod = check_and_cast<IPv6Encapsulation*>
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

  cXMLElementList tunList = ntun->getChildrenByTagName("tunnelEntry");

  for (NodeIt it = tunList.begin();it != tunList.end();it++)
  {
    cXMLElement* nte = *it;
    iface = getNodeProperties(nte, "exitIface");
    InterfaceEntry *ie = ift->interfaceByName(iface.c_str());
    if (!ie)
    {
      Dout(dc::encapsulation|dc::warning, "Node: "<< rt->nodeName() <<
           " TunnelEntry has incorrect interface identifier in XML configuration file ..."
           <<"(" << iface << ")"<<" IGNORING");
      continue;
    }
    int ifaceIdx = ie->outputPort();

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
      if (dest.prefixLength() == 0 || dest.prefixLength() == IPv6_ADDR_BITLENGTH)
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

  sourceRoute(ift, rt);

}

void XMLOmnetParser::sourceRoute(InterfaceTable *ift, RoutingTable6 *rt)
{
  using namespace OPP_Global;
  IPv6Forward* forwardMod = check_and_cast<IPv6Forward*>
    (findModuleByTypeDepthFirst(rt, "IPv6Forward"));
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

void XMLOmnetParser::parseNetworkEntity(InterfaceTable *ift, RoutingTable6 *rt)
{
  //const xml::node ne = *getNetNode(rt->nodeName());
  cXMLElement* ne = getNetNode(rt->nodeName());
  if (!ne)
  {
    Dout(dc::notice, rt->nodeName()<<" base settings used");
    ne = rt->par("baseSettings");
  }
  else
    Dout(dc::xml_addresses|flush_cf, rt->nodeName()<<" in parseNetworkEntity");

  parseNodeAttributes(rt, ne);

  //Parse per interface attribute
  cXMLElementList el = ne->getChildrenByTagName("interface");
  if (ift->numInterfaceGates() > el.size())
  {
    Dout(dc::notice, rt->nodeName()<<" ift->numInterfaceGates()="<<ift->numInterfaceGates()
         <<" while xml interface count is "<<el.size());
  }

  NodeIt startIf = el.begin();
  for(unsigned int iface_index = 0; iface_index < ift->numInterfaceGates(); iface_index++, startIf++)
  {
    if (startIf == el.end())
    {
      Dout(dc::warning|flush_cf, " No more interfaces found in XML for ifIndex>="<<iface_index);
      //Should fill in for rest of unspecified interfaces the defaults in
      //global element when we get that happening.
      break;
    }
    cXMLElement* nif = *startIf;

    parseInterfaceAttributes(ift, rt, nif, iface_index);
  }

  // do an additional parameter check
  checkValidData(ift, rt);
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

  IPv6Mobility* mob = check_and_cast<IPv6Mobility*>
    (OPP_Global::findModuleByName(rt, "mobility"));

  mob->ewuOutVectorHODelays = getNodeProperties(ne, "ewuOutVectorHODelays") == XML_ON;

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
  
  if (getNodeProperties(ne,"signalingEnhance") == string("Direct"))
    mob->setSignalingEnhance(MobileIPv6::Direct);
  else if (getNodeProperties(ne,"signalingEnhance") == string("CellResidency"))
    mob->setSignalingEnhance(MobileIPv6::CellResidency);
  else
    mob->setSignalingEnhance(MobileIPv6::None);

#ifdef USE_HMIP

 if (version() < 5 || getNodeProperties(ne, "hierarchicalMIPv6Support") != XML_ON)
    rt->hmipv6Support = false;
  else if (rt->mobilitySupport())
    rt->hmipv6Support = true;
  else
  {
    cerr << "HMIP Support cannot be activated without mobility support.\n";
    abort_ipv6suite();
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
        abort_ipv6suite();
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


void XMLOmnetParser::parseInterfaceAttributes(InterfaceTable *ift, RoutingTable6* rt, cXMLElement* nif, unsigned int iface_index)
{
  InterfaceEntry *ie = ift->interfaceByPortNo(iface_index);
  IPv6InterfaceData::RouterVariables& rtrVar = ie->ipv6()->rtrVar;

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

  IPv6InterfaceData::mipv6Variables& mipVar = ie->ipv6()->mipv6Var;
  mipVar.maxConsecutiveMissedRtrAdv = OPP_Global::atoul(getNodeProperties(nif, "MaxConsecMissRtrAdv").c_str());

#endif // USE_MOBILITY
#if FASTRA
  rtrVar.maxFastRAS = OPP_Global::atoul(getNodeProperties(nif, "MaxFastRAS").c_str());
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
  ie->setMtu(OPP_Global::atoul(getNodeProperties(nif, "HostLinkMTU").c_str()));
  ie->ipv6()->curHopLimit = OPP_Global::atoul(getNodeProperties(nif, "HostCurHopLimit").c_str());
  ie->ipv6()->baseReachableTime = OPP_Global::atoul(getNodeProperties(nif, "HostBaseReachableTime").c_str());
  ie->ipv6()->retransTimer = OPP_Global::atoul(getNodeProperties(nif, "HostRetransTimer").c_str());
#if FASTRS
  ie->ipv6()->maxRtrSolDelay = OPP_Global::atod(getNodeProperties(nif, "HostMaxRtrSolDelay").c_str());
#endif // FASTRS
  ie->ipv6()->dupAddrDetectTrans = OPP_Global::atoul(getNodeProperties(nif, "HostDupAddrDetectTransmits").c_str());

/*XXX
  ///Only some older XML files did not name their interfaces guess it is not
  ///really necessary as we do things in order anyway.
  ie->iface_name = getNodeProperties(nif, "name", false);
*/

  cXMLElement* napl =  nif->getElementByPath("./AdvPrefixList");
  if (napl)
  {
    //Parse prefixes
    cXMLElementList apl = napl->getChildrenByTagName("AdvPrefix");
    size_t numOfPrefixes = apl.size();
    ie->ipv6()->rtrVar.advPrefixList.resize(numOfPrefixes);
    if (numOfPrefixes == 0)
      Dout(dc::xml_addresses|dc::warning, rt->nodeName()
           <<" no prefixes even though advPrefixList exists");
    NodeIt startPr = apl.begin();
    for(size_t j = 0; j < numOfPrefixes; j++, startPr++)
    {
      if (startPr == apl.end())
        assert(false);
      cXMLElement* npr = *startPr;
      PrefixEntry& pe = ie->ipv6()->rtrVar.advPrefixList[j];
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
      ie->ipv6()->tentativeAddrs.push_back(IPv6Address(stripWhitespace(nad->getNodeValue())));
      Dout(dc::continued, "address "<< k << " is "<<stripWhitespace(nad->getNodeValue())<<" ");
    }
    Dout( dc::finish, "-|" );
  }
}


#ifdef USE_MOBILITY

void XMLOmnetParser::parseMovementInfo(MobilityStatic* mod)
{
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
    Dout(dc::notice, "Cannot find ObjectMovement in misc better be one in global");
    return;
  }

  cXMLElement* nmovenode = nobj->getFirstChildWithAttribute("MovingNode", "NodeName", nodeName);
  if (!nmovenode)
  {
    Dout(dc::notice, "No movement info for node "<<nodeName);
    return;
  }

  parseMovementInfoDetail(mod, nmovenode);
}

void XMLOmnetParser::parseMovementInfoDetail(MobilityStatic* mod, cXMLElement* nmovenode)
{
  // superclass for the (L1) mobile module
  MobileEntity* me = mod->mobileEntity;

  me->setStartMovingTime(OPP_Global::atoul(getNodeProperties(nmovenode, "startTime").c_str()));

  cXMLElementList moves = nmovenode->getChildrenByTagName("move");
  for (NodeIt it = moves.begin(); it != moves.end(); it++)
  {
    cXMLElement* nmove = *it;

    bool moveXFirst;
    if ( getNodeProperties(nmove, "moveXFirst") == XML_ON )
      moveXFirst = true;
    else
      moveXFirst = false;

    me->addMove(OPP_Global::atoul(getNodeProperties(nmove, "moveToX").c_str()),
                OPP_Global::atoul(getNodeProperties(nmove, "moveToY").c_str()),
                OPP_Global::atod(getNodeProperties(nmove, "moveSpeed").c_str()),
                moveXFirst
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
  parseRandomWPInfoDetail(mod, nmovenode);
}


void XMLOmnetParser::parseRandomWPInfoDetail(MobilityRandomWP* mod, cXMLElement* nmovenode)
{
  mod->minX = OPP_Global::atoul(getNodeProperties(nmovenode, "RWMinX").c_str());
  mod->maxX = OPP_Global::atoul(getNodeProperties(nmovenode, "RWMaxX").c_str());
  mod->minY = OPP_Global::atoul(getNodeProperties(nmovenode, "RWMinY").c_str());
  mod->maxY = OPP_Global::atoul(getNodeProperties(nmovenode, "RWMaxY").c_str());
  mod->moveInterval = OPP_Global::atod(getNodeProperties(nmovenode, "RWMoveInterval").c_str());
  mod->minSpeed = OPP_Global::atod(getNodeProperties(nmovenode, "RWMinSpeed").c_str());
  mod->maxSpeed = OPP_Global::atod(getNodeProperties(nmovenode, "RWMaxSpeed").c_str());
  mod->pauseTime = OPP_Global::atod(getNodeProperties(nmovenode, "RWPauseTime").c_str());
  mod->distance = OPP_Global::atoul(getNodeProperties(nmovenode, "RWDistance").c_str());
  mod->startTime = OPP_Global::atod(getNodeProperties(nmovenode, "RWStartTime").c_str());
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
void XMLOmnetParser::parseMAPInfo(InterfaceTable *ift, RoutingTable6 *rt)
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

  if (getNodeProperties(ne, "mapMNMUSTSetRoCAAsSource") == XML_ON)
    hmipRtr->mnMUSTSetRCoAAsSource = true;
  else
    hmipRtr->mnMUSTSetRCoAAsSource = false;

  cXMLElementList ifaces = ne->getChildrenByTagName("interface");
  NodeIt startIf = ifaces.begin();
  for(size_t i = 0; i < ift->numInterfaceGates() ; i++, ++startIf)
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

