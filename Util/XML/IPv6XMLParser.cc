// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Util/XML/Attic/IPv6XMLParser.cc,v 1.1 2005/02/09 06:15:59 andras Exp $
//
// Copyright (C) 2001 CTIE, Monash University
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
 * @file IPv6XMLParser.cc
 *
 * @brief Read in the interfaces and routing table from a file and parse
 * information to RoutingTable6
 *
 * @author  Eric Wu
 *
 * @date    10/11/2001
 *
 */

#include "sys.h"
#include "debug.h"

#include <xercesc/util/XMLString.hpp>     // general XML string
#include <xercesc/parsers/DOMParser.hpp> // XML parser
#include <xercesc/dom/DOM_DOMException.hpp>

#include "XMLDocHandle.h"
#include "IPv6XMLParser.h"

#include <sstream>
#include <boost/scoped_array.hpp>
#include <boost/cast.hpp>
#include <boost/lexical_cast.hpp>

#include "IPv6Address.h"
#include "RoutingTable6.h"
#include "NDEntry.h"
#include "opp_utils.h"
#include "IPv6Encapsulation.h"
#include "IPv6ForwardCore.h"
#include "IPv6CDS.h"

#include "NeighbourDiscovery.h"

#include "WorldProcessor.h"

#ifdef USE_HMIP
#include "HMIPv6ICMPv6NDMessage.h"
#include "HMIPv6NDStateRouter.h"
#include "HMIPv6Entry.h"
#endif // USE_HMIP

#ifdef USE_MOBILITY
#include "MobileEntity.h"
#include "WirelessEtherModule.h"
#include "MobilityStatic.h"
#include "MobilityRandomWalk.h"
#include <iostream>
#endif //USE_MOBILITY

using namespace OPP_Global;
using IPv6NeighbourDiscovery::PrefixEntry;
using boost::polymorphic_downcast;
using boost::scoped_array;

namespace
{
  typedef scoped_array<char> MyString;
  const string XML_ON = "on";
}

namespace XMLConfiguration
{


/**
 * @class AllElements
 *
 * @brief Must be a Xerces-c specific implmentation detail
 *
 * Types of nodes to parse??
 */

class AllElements : public DOM_NodeFilter
{
public:
  AllElements(){}
  virtual short acceptNode(const DOM_Node& n) const
    {
      switch(n.getNodeType())
      {
          case DOM_Node::ELEMENT_NODE : case DOM_Node::ATTRIBUTE_NODE :
            return FILTER_ACCEPT;
          default :
            return FILTER_SKIP;
      }
    }
};

/**
 * Sure this should have been in private protect it from people instantiating it
 * but then I'd have to declare the whole singleton holder to be a friend and
 * trying to forward declare that is a nightmare ( I tried very briefly) This is
 * the lesser of two evils compared to including singleton in xmlparser
 *
 */

IPv6XMLParser::IPv6XMLParser(void):_version(0)
{}

IPv6XMLParser::~IPv6XMLParser()
{}

IPv6Address* IPv6XMLParser::addresses(RoutingTable6* rt, const string& netNodeName, size_t iface_index,
                                      size_t& numOfAddrs)
{
  DOM_Node netNodeIface;

  if(!findNetNodeIface(netNodeName, netNodeIface, iface_index))
    return 0;

  if (_version >= 2)
  {
    MyString adv(((DOM_Element&)netNodeIface).
                 getAttribute("AdvSendAdvertisements").transcode());
    bool ifadv = adv.get() == XML_ON;
    rt->getInterfaceByIndex(iface_index).rtrVar.advSendAds = ifadv;

    const char* MAXRTRADVINTERVAL;
    const char* MINRTRADVINTERVAL;

#ifdef USE_MOBILITY
    MyString ha(((DOM_Element&)netNodeIface).
                 getAttribute("AdvHomeAgent").transcode());
    bool ifha = ha.get() == XML_ON;
    rt->getInterfaceByIndex(iface_index).rtrVar.advHomeAgent = ifha;
#endif // USE_MOBILITY


#ifdef USE_MOBILITY
    if (rt->mobilitySupport())
    {
      MAXRTRADVINTERVAL = "MIPv6MaxRtrAdvInterval";
      MINRTRADVINTERVAL = "MIPv6MinRtrAdvInterval";
    }
    else
    {
#endif // USE_MOBILITY

      MAXRTRADVINTERVAL = "MaxRtrAdvInterval";
      MINRTRADVINTERVAL = "MinRtrAdvInterval";

#ifdef USE_MOBILITY
    }
#endif // USE_MOBILITY

    MyString str_maxInt(((DOM_Element&)netNodeIface).
                        getAttribute(MAXRTRADVINTERVAL).transcode());
    rt->getInterfaceByIndex(iface_index).rtrVar.maxRtrAdvInt = atof(str_maxInt.get());

    MyString str_minInt(((DOM_Element&)netNodeIface).
                        getAttribute(MINRTRADVINTERVAL).transcode());
    rt->getInterfaceByIndex(iface_index).rtrVar.minRtrAdvInt = atof(str_minInt.get());
    
    if (rt->isRouter())
      Dout(dc::notice, rt->nodeName()<<":"<<iface_index<<" minRtrAdv="
           <<rt->getInterfaceByIndex(iface_index).rtrVar.minRtrAdvInt
           <<" maxRtrAdv="<<rt->getInterfaceByIndex(iface_index).rtrVar.maxRtrAdvInt);
#if FASTRA
    MyString fastRAstr(((DOM_Element&)netNodeIface).
                       getAttribute("MaxFastRAS").transcode());

    unsigned int maxRAS = 0;
    try
    {
      maxRAS = boost::lexical_cast<unsigned int>(fastRAstr.get());
    }
    catch(boost::bad_lexical_cast& e)
    {
      Dout(dc::warning|error_cf, netNodeName<<" Cannot convert into integer maxRas="<<fastRAstr.get());
    }
    bool fastRA = maxRAS != 0;
    rt->getInterfaceByIndex(iface_index).rtrVar.fastRA = fastRA;
    rt->getInterfaceByIndex(iface_index).rtrVar.maxFastRAS = maxRAS;
    if (rt->isRouter())
      Dout(dc::notice, rt->nodeName()<<":"<<iface_index<<" fastRA="<<fastRA
           <<" maxFastRAS="<<maxRAS);
#endif //FASTRA
  }

  DOM_NodeList addrNodes = ((DOM_Element*)(&netNodeIface))->
    getElementsByTagName(DOMString("inet_addr"));

  if(!addrNodes.getLength())
    return 0;

  numOfAddrs = addrNodes.getLength();
  IPv6Address* addrs = new IPv6Address[numOfAddrs];

  Dout(dc::xml_addresses|flush_cf|continued_cf, netNodeName<<":"<<iface_index<<" ");

  MyString addr_str;
  for(size_t i = 0; i < numOfAddrs; i++)
  {
    // IPv6 Address
    addr_str.reset(addrNodes.item(i).getFirstChild().getNodeValue().
      transcode());
    addrs[i].setAddress(addr_str.get());
    Dout(dc::continued, "address "<< i << " is "<<addr_str.get()<<" ");
  }
  Dout( dc::finish, "-|" );
  return addrs;
}

size_t IPv6XMLParser::retrieveVersion()
{
  int major, minor, revision;
  major = minor = revision = 0;
  char c = '.';

  istringstream(
    static_cast<const char*>(MyString(_XMLDoc->doc().getDocumentElement().
                                      getAttribute(DOMString("version")).
                                      transcode()).get()))>>major>>c>>minor>>c>>revision;
  return (major<<16) + (minor<<8) + revision;
}

string IPv6XMLParser::retrieveDebugChannels()
{
  return MyString(_XMLDoc->doc().getDocumentElement().getAttribute(DOMString("debugChannel")).transcode()).get();
}

bool IPv6XMLParser::findNetNode(const string& netNodeName, DOM_Node& netNode)
{
  // look for matching current network node name with the XML file
  // TODO: may need to get rid of this hard coding string, local in
  // the future

  DOM_NodeList networkNodes = _XMLDoc->doc().getElementsByTagName(DOMString("local"));

  if (_version == 0)
  {
    _version = retrieveVersion();
  }


  bool isNodeFound = false;

  for(size_t i = 0; i < networkNodes.getLength(); i ++)
  {
    netNode = networkNodes.item(i);
/*    Dout(dc::xml_addresses, netNode.getAttributes().item(0).
         getFirstChild().getNodeName().transcode()<<"(XML name)="
         << netNode.getAttributes().item(0).getFirstChild().getNodeValue().transcode());
*/
    // TODO: again this "node" may be removed in the future
    if(MyString(static_cast<DOM_Element&>(netNode).getAttribute(
                  DOMString("node")).transcode()).get() ==  netNodeName)
    {
      isNodeFound = true;
      break;
    }
  }

  return isNodeFound;
}

void IPv6XMLParser::parse(RoutingTable6* rt)
{

  typedef std::list<NetParam*>::iterator It;

  string netNodeName = findNetNodeModule(rt)->name();

  for(It it = paramList.begin(); it != paramList.end(); it++)
  {
    NetParam* param = (*it);

    for(size_t i = 0; i < rt->interfaceCount(); i++)
    {
      // Global setting of the parametre, no need to specifiy
      // particular network node name and network interface
      traverse(param[i]);

      // Local setting of the parametre, given with specific network
      // node name and the interface index
      traverse(param[i], netNodeName, i);
    }
    delete[] param;
  }
  //Free memory inside paramList quickly (if we only did this once it would be
  //advantageous) However as we reparse the XML file per node it is best to
  //leave that capacity alone instead of reallocating it until the last
  //iteration.
  //ParamList.swap(paramList.clear()); Item 17
  paramList.clear();
}

#ifdef USE_HMIP
void IPv6XMLParser::parseMAPInfo(RoutingTable6* rt)
{
  NeighbourDiscovery* ndmod = static_cast<NeighbourDiscovery*>(OPP_Global::findModuleByName(rt, "nd"));

  HierarchicalMIPv6::HMIPv6NDStateRouter* hmipRtr = dynamic_cast<HierarchicalMIPv6::HMIPv6NDStateRouter*>(ndmod->nd);

  if (!hmipRtr)
    return;

  DOM_Node dom_node;
  if(!findNetNode(rt->nodeName(), dom_node))
    return;

  if (MyString(static_cast<DOM_Element&>(dom_node).
               getAttribute(DOMString("mapMode")).transcode()).get() == string("Basic"))
    hmipRtr->mode = HierarchicalMIPv6::HMIPv6NDStateRouter::modeBasic;
  else if (MyString(static_cast<DOM_Element&>(dom_node).
                    getAttribute(DOMString("mapMode")).transcode()).get() == string("Extended"))
    hmipRtr->mode = HierarchicalMIPv6::HMIPv6NDStateRouter::modeExtended;

  if (MyString(static_cast<DOM_Element&>(dom_node).
               getAttribute(DOMString("mapMNMAYSetRoCAAsSource")).transcode()).get() == XML_ON)
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

  if (MyString(static_cast<DOM_Element&>(dom_node).
               getAttribute(DOMString("mapMNMUSTSetRoCAAsSource")).transcode()).get() == XML_ON)
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


  if (MyString(static_cast<DOM_Element&>(dom_node).
               getAttribute(DOMString("mapReverseTunnel")).transcode()).get() == XML_ON)
    hmipRtr->reverseTunnel = true;
  else
    hmipRtr->reverseTunnel = false;

  for(size_t i = 0; i < rt->interfaceCount(); i++)
  {
    DOM_Node dom_Iface;

    if(findNetNodeIface(rt->nodeName(), dom_Iface, i))
    {
      DOM_NodeList dom_maps = ((DOM_Element&)(dom_Iface)).
        getElementsByTagName(DOMString("AdvMAPEntry"));

      DOM_Node node_map;
      for (size_t j = 0; j < dom_maps.getLength(); j++)
      {
        node_map = dom_maps.item(j);
        DOM_Element& dom_map = (DOM_Element&)(node_map);

        MyString str_dist(dom_map.getAttribute(DOMString("AdvMAPDist")).transcode());
        int dist = atoi(str_dist.get());

        MyString str_pref(dom_map.getAttribute(DOMString("AdvMAPPref")).
                          transcode());
        int pref = atoi(str_pref.get());

        MyString str_expires(dom_map.getAttribute(DOMString("AdvMAPValidLifetime")).transcode());
        int expires = atoi(str_expires.get());

        MyString str_addr(dom_map.getFirstChild().getNodeValue().transcode());
        ipv6_addr map_addr = IPv6Address(str_addr.get());

        HierarchicalMIPv6::HMIPv6ICMPv6NDOptMAP mapOpt(dist, pref, expires, map_addr);
        mapOpt.iface_idx = i;

        hmipRtr->addMAP(mapOpt);

        Dout(dc::debug, rt->nodeName()<<" XML advMap="<<HierarchicalMIPv6::HMIPv6MAPEntry(mapOpt));
      }
    }
  }
}
#endif // USE_HMIP



#ifdef USE_MOBILITY

void IPv6XMLParser::parseWirelessEtherInfo(WirelessEtherModule* wlanMod)
{
  assert(wlanMod);

  const char* nodeName = OPP_Global::findNetNodeModule(wlanMod)->name();

  DOM_Node dom_node;
  if(!findNetNode(nodeName, dom_node))
    return;

  bool isFound = false;

  // TODO: make this as a function - find net node by name

  DOM_Node dom_iface;

  DOM_NodeList dom_ifaces = ((DOM_Element&)(dom_node)).getElementsByTagName(DOMString("interface"));
  for ( size_t i = 0; i < dom_ifaces.getLength(); i++ )
  {
    MyString str_iface(dom_ifaces.item(i).getAttributes().item(0).
                       getFirstChild().getNodeValue().transcode());

    if ( std::string(str_iface.get()) == wlanMod->getInterfaceName())
    {
      dom_iface = dom_ifaces.item(i);
      isFound = true;
      break;
    }
  }

  if (isFound == false)
    return;

  DOM_Node info = (( DOM_Element&)(dom_iface)).getElementsByTagName(DOMString("WEInfo")).item(0);
  
  DOM_Element& dom_info = (DOM_Element&)info;

  if(dom_info.isNull())
  {
    cerr << "Node: "<< nodeName <<" - "
         << "Iface: " << wlanMod->getInterfaceName()
         << " Make sure you have <WirelessEtherInfo> <WEInfo />"
         << " </WirelessEtherInfo> under <interface>"
         << endl;
    exit(1);
  }

  MyString str_ssid(dom_info.getAttribute(DOMString("WEssid")).transcode());
  wlanMod->ssid = str_ssid.get();
  MyString str_pLExp(dom_info.getAttribute(DOMString("WEPathLossExponent")).transcode());
  const_cast<double&>(wlanMod->pLExp) = atof(str_pLExp.get());
  MyString str_pLStdDev(dom_info.getAttribute(DOMString("WEPathLossStdDev")).transcode());
  const_cast<double&>(wlanMod->pLStdDev) = atof(str_pLStdDev.get());
  MyString str_txpower(dom_info.getAttribute(DOMString("WETxPower")).transcode());
  const_cast<double&>(wlanMod->txpower) = atof(str_txpower.get());
  MyString str_threshpower(dom_info.getAttribute(DOMString("WEThresholdPower")).transcode());
  const_cast<double&>(wlanMod->threshpower) = atof(str_threshpower.get());
  MyString str_hothreshpower(dom_info.getAttribute(DOMString("WEHOThresholdPower")).transcode());
  const_cast<double&>(wlanMod->hothreshpower) = atof(str_hothreshpower.get());
  MyString str_prbenergytout(dom_info.getAttribute(DOMString("WEProbeEnergyTimeout")).transcode());
  const_cast<double&>(wlanMod->probeEnergyTimeout) = atof(str_prbenergytout.get());
  MyString str_prbresponsetout(dom_info.getAttribute(DOMString("WEProbeResponseTimeout")).transcode());
  const_cast<double&>(wlanMod->probeResponseTimeout) = atof(str_prbresponsetout.get());
  MyString str_authtout(dom_info.getAttribute(DOMString("WEAuthenticationTimeout")).transcode());
  const_cast<double&>(wlanMod->authenticationTimeout) = atof(str_authtout.get());
  MyString str_asstout(dom_info.getAttribute(DOMString("WEAssociationTimeout")).transcode());
  const_cast<double&>(wlanMod->associationTimeout) = atof(str_asstout.get());
	MyString str_retry(dom_info.getAttribute(DOMString("WERetry")).transcode());
  const_cast<unsigned int&>(wlanMod->maxRetry) = atoi(str_retry.get());
  MyString str_fastactivescan(dom_info.getAttribute("WEFastActiveScan").transcode());
        if (str_fastactivescan.get() == XML_ON)
           wlanMod->fastActScan = true;
        else
           wlanMod->fastActScan = false;
  MyString str_scanshortcircuit(dom_info.getAttribute("WEScanShortCircuit").transcode());
        if (str_scanshortcircuit.get() == XML_ON)
           wlanMod->scanShortCirc = true;
        else
           wlanMod->scanShortCirc = false;
  MyString str_crosstalk(dom_info.getAttribute("WECrossTalk").transcode());
        if (str_crosstalk.get() == XML_ON)
           wlanMod->crossTalk = true;
        else
           wlanMod->crossTalk = false;
  MyString str_shadowing(dom_info.getAttribute("WEShadowing").transcode());
        if (str_shadowing.get() == XML_ON)
           wlanMod->shadowing = true;
        else
           wlanMod->shadowing = false;
  MyString str_channottoscan(dom_info.getAttribute(DOMString("WEChannelsNotToScan")).transcode());
  wlanMod->chanNotToScan = str_channottoscan.get();
  MyString str_sSMaxSample(dom_info.getAttribute(DOMString("WESignalStrengthMaxSample")).transcode());
  const_cast<unsigned int&>(wlanMod->sSMaxSample) = atoi(str_sSMaxSample.get());

  MyString str_addr(dom_info.getAttribute(DOMString("WEAddress")).transcode());
  std::string addr = str_addr.get();

  if(wlanMod->apMode == false)
  {
    if(addr == "0")
    {
      MAC_address macAddr;
      macAddr.high = OPP_Global::generateInterfaceId() & 0xFFFFFF;
      macAddr.low = OPP_Global::generateInterfaceId() & 0xFFFFFF;
      wlanMod->address.set(macAddr);
    }
    else
      wlanMod->address.set(addr.c_str());
  }

	MyString str_bwreq(dom_info.getAttribute(DOMString("WEBandwidthRequirements")).transcode());
	const_cast<double&>(wlanMod->bWRequirements) = atof(str_bwreq.get());
	MyString str_statsVec(dom_info.getAttribute("WERecordStatisticVector").transcode());
    if (str_statsVec.get() == XML_ON)
      wlanMod->statsVec = true;
    else
      wlanMod->statsVec = false;

    MyString str_activescan(dom_info.getAttribute("WEActiveScan").transcode());
    if (str_activescan.get() == XML_ON)
      wlanMod->activeScan = true;
    else
      wlanMod->activeScan = false;

    MyString str_chanscantime(dom_info.getAttribute(DOMString("WEChannelScanTime")).transcode());
    const_cast<double&>(wlanMod->channelScanTime) = atof(str_chanscantime.get());
        // TODO: parse supported rate
}

void IPv6XMLParser::parseMovementInfo(MobilityStatic* mod)
{
  // superclass for the (L1) mobile module
  MobileEntity* me = mod->mobileEntity;

  // obtain movement info from XML

  DOM_NodeList movingNodes = _XMLDoc->doc().getElementsByTagName(DOMString("MovingNode"));
  DOM_Node movingNode;

  bool isFound = false;

  for(size_t i = 0; i < movingNodes.getLength(); i ++)
  {
    movingNode = movingNodes.item(i);
    // TODO: again this "node" may be removed in the future
    if(!strcmp(MyString(movingNode.getAttributes().item(0).
                        getFirstChild().getNodeValue().transcode()).get(),
               OPP_Global::findNetNodeModule(mod)->name()))
    {
      isFound = true;
      break;
    }
  }

  if ( isFound == false)
    return;

  MyString str_StartTime(movingNode.getAttributes().item(1).
                         getFirstChild().getNodeValue().transcode());

  me->setStartMovingTime(atoi(str_StartTime.get()));

  DOM_NodeList moves = ((DOM_Element*)(&movingNode))->
    getElementsByTagName(DOMString("move"));

  for ( size_t i = 0; i < moves.getLength(); i++)
  {
    MyString str_MoveX(moves.item(i).getAttributes().item(0).
                       getFirstChild().getNodeValue().transcode());
    MyString str_MoveY(moves.item(i).getAttributes().item(1).
                       getFirstChild().getNodeValue().transcode());
    MyString str_Speed(moves.item(i).getAttributes().item(2).
                       getFirstChild().getNodeValue().transcode());

    me->addMove(atoi(str_MoveX.get()), atoi(str_MoveY.get()), atoi(str_Speed.get()));
  }
}

void IPv6XMLParser::parseRandomWalkInfo(MobilityRandomWalk* mod)
{
/*  // superclass for the (L1) mobile module
  MobileEntity* me = mod->mobileEntity;

  // obtain movement info from XML

  DOM_NodeList movingNodes = _XMLDoc->doc().getElementsByTagName(DOMString("MovingNode"));
  DOM_Node movingNode;

  bool isFound = false;

  for(size_t i = 0; i < movingNodes.getLength(); i ++)
  {
    movingNode = movingNodes.item(i);
    // TODO: again this "node" may be removed in the future
    if(!strcmp(MyString(movingNode.getAttributes().item(0).
                        getFirstChild().getNodeValue().transcode()).get(),
               OPP_Global::findNetNodeModule(mod)->name()))
    {
      isFound = true;
      break;
    }
  }

  if ( isFound == false)
    return;

  MyString str_StartTime(movingNode.getAttributes().item(1).
                         getFirstChild().getNodeValue().transcode());

  me->setStartMovingTime(atoi(str_StartTime.get()));

  DOM_NodeList moves = ((DOM_Element*)(&movingNode))->
    getElementsByTagName(DOMString("move"));

  for ( size_t i = 0; i < moves.getLength(); i++)
  {
    MyString str_MoveX(moves.item(i).getAttributes().item(0).
                       getFirstChild().getNodeValue().transcode());
    MyString str_MoveY(moves.item(i).getAttributes().item(1).
                       getFirstChild().getNodeValue().transcode());
    MyString str_Speed(moves.item(i).getAttributes().item(2).
                       getFirstChild().getNodeValue().transcode());

    me->addMove(atoi(str_MoveX.get()), atoi(str_MoveY.get()), atoi(str_Speed.get()));
    }*/
}

#endif //USE_MOBILITY

bool IPv6XMLParser::findNetNodeIface(const string& netNodeName,
                                     DOM_Node& netNodeIface,
                                     int iface_index)
{
  DOM_Node netNode;

  if(!findNetNode(netNodeName, netNode))
    return false;

  netNodeIface = ((DOM_Element*)(&netNode))->
    getElementsByTagName(DOMString("interface")).item(iface_index);

  if(netNodeIface.isNull())
    return false;

  return true;
}

void IPv6XMLParser::staticRoutingTable(RoutingTable6* rt, const string& netNodeName)
{
  DOM_Node netNode;

  if(!findNetNode(netNodeName, netNode))
    return;

  DOM_NodeList routeEntryNodes = ((DOM_Element*)(&netNode))->
    getElementsByTagName(DOMString("routeEntry"));

  size_t numOfEntries = routeEntryNodes.getLength();

  if(!numOfEntries)
    return;

  MyString iface_str, destination_str, nextHop_str;

  for(size_t i = 0; i <  numOfEntries; i++)
  {
    DOM_Node routeNode = routeEntryNodes.item(i);

    DOM_Element* elem = static_cast<DOM_Element*> (&routeNode);

    // route interface ID
    iface_str.reset(elem->getAttribute(DOMString("routeIface")).transcode());

    // destination
    destination_str.reset(elem->
                          getAttribute(DOMString("routeDestination")).transcode());

    // next hop
    nextHop_str.reset(elem->
                      getAttribute(DOMString("routeNextHop")).transcode());

    if (*(iface_str.get()) == '\0')
    {
      cerr << "Node: "<< netNodeName
           <<" Route does not contain an interface identifier attribute" <<endl;
      exit(1);
    }

    if (*(destination_str.get()) == '\0')
    {
      cerr <<  "Node: "<< netNodeName
           <<" -- Need to specify a destination address in static routing entry"
           <<endl;
      exit(1);
    }

    //Distinguish between two types of Routing entries

    //normal next hop route

    //Next Hop is a router ip address on same link as interface to forward
    //things with destination.  Destination can either be a network prefix
    //(prefix length < 128) or a host ipv6 address (prefix length = 128)

        //Default route - special case of next hop route
        //Destination is the unspecified IPv6 address 0::0

    //internal neighbour entries

    //Next Hop attribute is same as destination.  This is to speed up address
    //resolution which would otherwise need to be done on all ifaces in order to
    //find on which iface the destination exists on
      //The isRouter flag is required to disambiguate between neighbouring
      //hosts and routers.  Normally this flag is not required as you usually
      //specify the router as nextHop and either a net prefix for router or a
      //destination/default route for a host so you know nextHop is a router.
      //However a host can now be conveniently specified with just a dest and so
      //can a router so this was added for that purpose too (which means for a
      //host specifying one route with dest addr and isRouter set then that is a
      //default router) .

    int ifaceIdx = rt->interfaceNameToNo(iface_str.get());

    if(ifaceIdx == -1)
    {
      cerr << "Node: "<< netNodeName
           <<" Route has incorrect interface identifier in static routing table... "
           <<"(" << iface_str.get() << ")" << endl;
      exit(1);
    }

    IPv6Address nextHopAddr(nextHop_str.get());
    IPv6Address destAddr(destination_str.get());
    destAddr.truncate();

    bool destIsHost = MyString(elem->getAttribute(DOMString("isRouter")).
                                 transcode()).get() != XML_ON;

    //Route to a neighbouring host
    if ( destIsHost &&
        *(nextHop_str.get()) == '\0' || strcmp(nextHop_str.get(), destination_str.get()) == 0 )
    {
      if (destAddr.prefixLength() > 0 && destAddr.prefixLength() < IPv6_ADDR_LENGTH)
      {
        rt->insertPrefixEntry(PrefixEntry(destAddr, VALID_LIFETIME_INFINITY), ifaceIdx);
        Dout(dc::neighbour_disc, netNodeName<<" Added on-link subnet prefix "
             <<destAddr<<" for "<<iface_str.get());
        continue;
      }

      //Create DE and NE
      rt->cds->insertNeighbourEntry( new NeighbourEntry(destAddr, ifaceIdx));

      Dout(dc::neighbour_disc|dc::routing, netNodeName<<" Neighbouring Host added ifIndex="
           <<ifaceIdx<<" dest="<<destAddr);
      continue;
    }

    if (!destIsHost)
      nextHopAddr = destAddr;

    RouterEntry* re = 0;
    NeighbourEntry* ne = 0;
    //By definition Next Hop has to be router in order to forward packets to
    //dest. Destination can be a network prefix with "0"s for non prefix bits.
    bool createEntry = true;

    //Check if routerEntry has already been created
    if ((ne = (rt->cds->neighbour(nextHopAddr)).lock().get()) != 0)
    {
      if (ne->isRouter())
      {
        createEntry = false;
        re = static_cast<RouterEntry*>(ne);
/*  //NeighbourEntry needs a virtual function in order to test ne identity
#if !defined TESTIPv6
        re = static_cast<RouterEntry*>(ne);
#else
        re = dynamic_cast<RouterEntry*>(ne);
        assert(re != 0);
#endif //TESTIPv6
*/
      }
      else
      {
        Dout(dc::warning, netNodeName<<" specifies a router at "<<nextHopAddr
             <<", conflicts with previous entry for a neighbouring host."
             <<" Neighbouring host entry removed");
        cerr<<netNodeName<<" specifies a router at "<<nextHopAddr
            <<", conflicts with previous entry for a neighbouring host."
            <<" Neighbouring host entry removed"
            <<endl;
        rt->cds->removeNeighbourEntry(ne->addr());
      }
    }

    if (createEntry)
    {
      re = new RouterEntry(nextHopAddr, ifaceIdx, "",
                           VALID_LIFETIME_INFINITY);
      re->setState(NeighbourEntry::INCOMPLETE);
      rt->insertRouterEntry(re);
      Dout(dc::router_disc|dc::routing, netNodeName<<" New router added addr="<<nextHopAddr
           <<" ifIndex="<<ifaceIdx<<" with infinite lifetime");
    }

    if (!(destAddr == IPv6_ADDR_UNSPECIFIED))
    {

      (*rt->cds)[destAddr].neighbour = rt->cds->router(re->addr());
      
      if (destAddr != nextHopAddr && !( destAddr == IPv6_ADDR_UNSPECIFIED))
        Dout(dc::forwarding|dc::routing|dc::xml_addresses, netNodeName<<" Next hop for "<<destAddr <<" is "<<nextHopAddr);
    }
    else
    {
      //Dout(dc::router_disc|dc::routing, netNodeName<<" "<<nextHopAddr<<" is the default route");
      Dout(dc::router_disc|dc::routing, netNodeName<<" "<<re->addr()<<" is the default route "
           <<rt->cds->router(re->addr()).lock()->addr());


      //Still problem of creating many router objects even though they have the
      //same value. Not only that the default route of 0.0.0.0 was added into
      //the DC this should not be required.
      rt->cds->setDefaultRouter(rt->cds->router(re->addr()));
    }

  } //end for each XML route element

  if (_version >= 2)
    tunnelConfiguration(rt, netNode);
}

void IPv6XMLParser::tunnelConfiguration(RoutingTable6* rt, const DOM_Node& netNode)
{
  MyString iface, tunEntry, tunExit, destination;
  ipv6_addr src, dest;
  size_t vifIndex = 0;

  IPv6Encapsulation* tunMod = polymorphic_downcast<IPv6Encapsulation*>
    (findModuleByType(rt, "IPv6Encapsulation"));

  //Even though downcast detects incorrect downcasts it still allows casting 0
  //down to anything
  assert(tunMod != 0);

  const DOM_NodeList& tunnelEntryNodes = static_cast<const DOM_Element&>(netNode).
    getElementsByTagName(DOMString("tunnelEntry"));

  size_t numOfEntries = tunnelEntryNodes.getLength();
  size_t numOfTriggers = 0;

  for(size_t i = 0; i <  numOfEntries; i++, numOfTriggers = 0)
  {
    const DOM_Node& tunnelNode = tunnelEntryNodes.item(i);

    const DOM_Element* elem = static_cast<const DOM_Element*> (&tunnelNode);

    // route interface ID
    iface.reset(elem->getAttribute(DOMString("exitIface")).transcode());
    int ifaceIdx = rt->interfaceNameToNo(iface.get());

    if(ifaceIdx == -1)
    {
      cerr << "Node: "<< rt->nodeName() <<
        " TunnelEntry has incorrect interface identifier in XML configuration file ..."
           <<"(" << iface.get() << ")" << endl;
      assert(ifaceIdx != -1);
      cerr << "Ignoring this tunnel entry "<<endl;
      continue;
    }

    // destination
    tunEntry.reset(elem->getAttribute(DOMString("entryPoint")).transcode());

    tunExit.reset(elem->getAttribute(DOMString("exitPoint")).transcode());

    const DOM_NodeList& triggerNodes = ((const DOM_Element*)(&tunnelNode))->
      getElementsByTagName(DOMString("triggers"));

    numOfTriggers = triggerNodes.getLength();

    src = c_ipv6_addr(tunEntry.get());
    dest = c_ipv6_addr(tunExit.get());

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

    for(size_t j = 0; j <  numOfTriggers; j++)
    {
      const DOM_Node& triggerNode = triggerNodes.item(j);
      const DOM_Element* elem = static_cast<const DOM_Element*>(&triggerNode);

      destination.reset(elem->getAttribute(DOMString("destination")).transcode());
      bool associatedTrigger = false;
      IPv6Address dest(destination.get()); 
      if (dest.prefixLength() == 0 || dest.prefixLength() == IPv6_ADDR_LENGTH)
      {
        ipv6_addr trigdest=c_ipv6_addr(destination.get());
        associatedTrigger = tunMod->tunnelDestination(trigdest, vifIndex);
        Dout(dc::continued, " trigger dest="<<trigdest);
      }
      else
      {
        associatedTrigger = tunMod->tunnelDestination(dest, vifIndex);
        Dout(dc::continued, " trigger dest="<<dest);
      }
      if (!associatedTrigger)
        cerr<<"Unable to use dest="<<destination.get()<<" to trigger tunnel="
            <<vifIndex<<endl;

    } //end for triggers
    Dout(dc::finish, "-|");
  } //end for tunnelEntries

  sourceRoute(rt, netNode);
}

void IPv6XMLParser::sourceRoute(RoutingTable6* rt, const DOM_Node& netNode)
{
  IPv6ForwardCore* forwardMod = polymorphic_downcast<IPv6ForwardCore*>
    (findModuleByTypeDepthFirst(rt, "IPv6ForwardCore"));
  //Even though downcast detects incorrect downcasts it still allows casting 0
  //down to anything
  assert(forwardMod != 0);

  const DOM_NodeList& sourceRouteEN = ((DOM_Element*)(&netNode))->
     getElementsByTagName(DOMString("sourceRouteEntry"));

  MyString dest;

  size_t nextHopCount = 0;

  for (size_t i = 0; i < sourceRouteEN.getLength(); i++)
  {
    const DOM_Node& sourceRouteNode =  sourceRouteEN.item(i);
    const DOM_Element& elem = static_cast<const DOM_Element&> (sourceRouteNode);

    const DOM_NodeList& nextHopNodes = elem.getElementsByTagName(DOMString("nextHop"));
    nextHopCount = nextHopNodes.getLength();

    SrcRoute route(new _SrcRoute(nextHopCount + 1, IPv6_ADDR_UNSPECIFIED));

    dest.reset(elem.getAttribute(DOMString("finalDestination")).transcode());

    (*route)[nextHopCount] = c_ipv6_addr(dest.get());


    for (size_t j = 0; j < nextHopCount; j++)
    {
      const DOM_Node& nextHop = nextHopNodes.item(j);
      dest.reset(((const DOM_Element&)nextHop).
                 getAttribute(DOMString("address")).transcode());
      (*route)[j] = c_ipv6_addr(dest.get());
    }
    forwardMod->addSrcRoute(route);
  }

}

// TODO: RESTRUCTURE THIS CODE... LOOKS TOO UGLY
PrefixEntry* IPv6XMLParser::prefixes(const string& netNodeName, size_t iface_index,
                                     size_t& numOfPrefixes)
{
  DOM_Node netNodeIface;

  if(!findNetNodeIface(netNodeName, netNodeIface, iface_index))
    return 0;

  DOM_NodeList prefixNodes = ((DOM_Element*)(&netNodeIface))->
    getElementsByTagName(DOMString("AdvPrefix"));

  if(!prefixNodes.getLength())
    return 0;

  numOfPrefixes = prefixNodes.getLength();

  // TODO: Be careful about this.. It may cause memory leaks if not
  // deleting properly
  PrefixEntry* prefixes = new PrefixEntry[numOfPrefixes];

  for(size_t i = 0; i <  numOfPrefixes; i++)
  {
    // Prefix Address
    MyString prefix_addr_str(prefixNodes.
      item(i).getFirstChild().getNodeValue().transcode());

    DOM_Node prefixNode = prefixNodes.item(i);
    DOM_Element* elem = static_cast<DOM_Element*>(&prefixNode);

    // Prefix Advertised Valid Life Time
    MyString advValidLifeTime_str(elem->
      getAttribute(DOMString("AdvValidLifetime")).transcode());

    if(advValidLifeTime_str.get())
    {
      //TODO Does this cause problems since we want unsigned int not the signed version?
      saveInfo("AdvValidLifetime", advValidLifeTime_str.get(),
               &(prefixes[i]._advValidLifetime), TYPE_INT);
    }

    // Prefix Advertised On Link Flag
    MyString AdvOnLinkFlag_str(elem->
      getAttribute(DOMString("AdvOnLinkFlag")).transcode());

    if(AdvOnLinkFlag_str.get())
    {
      saveInfo("AdvOnLinkFlag", AdvOnLinkFlag_str.get(),
               &(prefixes[i]._advOnLink), TYPE_BOOL);
    }

    // Prefix Advertised Prefix Life Time
    MyString advPrefLifetime_str(elem->
      getAttribute(DOMString("AdvPreferredLifetime")).transcode());

    if(advPrefLifetime_str.get())
    {
      saveInfo("AdvPreferredLifetime", advPrefLifetime_str.get(),
               &(prefixes[i]._advPrefLifetime), TYPE_INT);
    }

    // Prefix Autonomous Flag
    MyString AdvAutonomousFlag_str(elem->
      getAttribute(DOMString("AdvAutonomousFlag")).transcode());

    if(AdvAutonomousFlag_str.get())
    {
      saveInfo("AdvAutonomousFlag", AdvAutonomousFlag_str.get(),
               &(prefixes[i]._advAutoFlag), TYPE_BOOL);
    }

#ifdef USE_MOBILITY
    // Router Address Flag - for mobility support
    MyString advRtrAddrFlag_str(elem->
      getAttribute(DOMString("AdvRtrAddrFlag")).transcode());

    if(advRtrAddrFlag_str.get())
    {
      saveInfo("AdvRtrAddrFlag", advRtrAddrFlag_str.get(),
               &(prefixes[i]._advRtrAddr), TYPE_BOOL);
    }

    prefixes[i].setPrefix(prefix_addr_str.get(), !prefixes[i]._advRtrAddr);
#else
    prefixes[i].setPrefix(prefix_addr_str.get());
#endif //USE_MOBILITY
  }

  return prefixes;
}

/**
   @warning This code is very much unsafe and leaky.  Check param for details on
   why.  For one the char attr_value[] is never deleted in NetParam and NetParam
   appears to contain a list of pointers to itself. (if they were deleted the
   behaviour is udefined as not all those pointers point to dynamically
   allocated objects.
 */
void IPv6XMLParser::traverse(NetParam& param, const string& netNodeName,
                             int iface_index)
{
  DOM_Element result;
  DOM_Node node;

  // if the net node name is not specified, assume it is a global
  // setting variable
  if(netNodeName == "")
  {
    string tagname = string("g") + param.getTagname();
    node = _XMLDoc->doc().getElementsByTagName(DOMString(tagname.c_str())).item(0);
    result = *(static_cast<DOM_Element*> (&node));
  }
  // if the node name is set, assume it is a local setting variable
  else
  {
    DOM_Node netNode, node;

    if(findNetNode(netNodeName, netNode))
    {
      node = (static_cast<DOM_Element*> (&netNode))->getElementsByTagName(
        DOMString(param.getTagname().c_str())).item(iface_index);
      result = *(static_cast<DOM_Element*> (&node));
    }
  }

  if(!result.isNull())
  {
    // the actaul data in XML document is stored in the child node
    // of the node with tagname
    MyString value_str(result.getFirstChild().getNodeValue().transcode());

    if(param.getElement())
      saveInfo(param.getTagname(), value_str.get(),
               param.getElement(), param.getType());

    // retrieve attributes of the parametre
    if(param.numOfAttrs)
    {
      for(int i = 0; i < param.numOfAttrs; i ++)
      {
        char * attr_value = result.getAttribute(
          DOMString(param.attrs[i].getTagname().c_str())).transcode();

        if(attr_value != 0)
          saveInfo(param.attrs[i].getTagname(),
                   attr_value, param.attrs[i].getElement(), param.attrs[i].getType());
      }
    }
  }
}

}
