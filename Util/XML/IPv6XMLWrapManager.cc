//
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
 * @file   IPv6XMLWrapManager.cc
 * @author Johnny Lai
 * @date   15 Jul 2003
 *
 * @brief  Implementation of IPv6XMLWrapManager
 *
 *
 */

#include "sys.h"
#include "debug.h"
#include "config.h"

#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <xmlwrapp/xmlwrapp.h>

#include "IPv6XMLWrapManager.h"
#include "XMLCommon.h"

#include "RoutingTable6.h"
#include "opp_utils.h"
#include "IPv6Encapsulation.h"
#include "IPv6ForwardCore.h"
#include "IPv6CDS.h"
#include "MACAddress.h"

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

#include <boost/regex.hpp>
namespace
{
  std::string  stripWhitespace(const std::string& content )
  {
    static const std::string blankMatch("\\s+");
    static const std::string blankKiller("");
    static boost::regex match(blankMatch);
    return boost::regex_merge(content, match, blankKiller);
  }
}

namespace XMLConfiguration
{

///For non omnetpp csimplemodule derived classes
IPv6XMLWrapManager::IPv6XMLWrapManager(const char* filename):_version(0)
{
  SingularParser::Instance().parseFile(filename);
}

IPv6XMLWrapManager::~IPv6XMLWrapManager()
{
}

unsigned int IPv6XMLWrapManager::version() const
{
  if (!_version)
  {
    int major, minor, revision;
    major = minor = revision = 0;
    char c = '.';

    std::string v = getNodeProperties(root(), "version");
    //const char* v = (*(root().get_attributes().find("version"))).get_value();
    std::istringstream(v)>>major>>c>>minor>>c>>revision;
    //gnuc compiler bug?
    //std::istringstream((*(root().get_attributes().find("version"))).get_value())>>major>>c>>minor>>c>>revision;
    _version = (major<<16) + (minor<<8) + revision;
    Dout(dc::xml_addresses, "major="<<major<<" minor="<<minor<<" rev="<<revision);
  }
  return _version;
}

struct findElementByName:
  public std::unary_function<xml::node, bool >
{
  findElementByName(const std::string& name):elementName(name)
    {}

  bool operator()(const xml::node& n) const
    {
      if (n.get_type() != xml::node::type_element)
    return false;
      return elementName == n.get_name();
    }
private:
  std::string elementName;
};

struct findElementByValue: public findElementByName
{
  findElementByValue(const char* name, const char* attr,
             const char* value):findElementByName(name),
                           attributeName(attr),
                           value(value)
    {}

   bool operator()(const xml::node& n) const
    {
      if (!findElementByName::operator()(n))
        return false;
      typedef xml::attributes::const_iterator AttIt;
      AttIt ait;
      return (ait = n.get_attributes().find(attributeName)) != n.get_attributes().end() && !strcmp((*ait).get_value(), value);
    }
private:
  const char* attributeName;
  const char* value;
};

/**
   @brief Returns the Element for a network node with name of name
   @param name of network node

   @todo Allow unconfigured nodes at this stage. They only get default values
   defined in ctors not in the schema. Should use prototype object method to
   transfer default values.
*/

IPv6XMLWrapManager::NodeIt IPv6XMLWrapManager::getNetNode(const char* name) const
{
  NodeIt it = find_if(doc().get_root_node().begin(), doc().get_root_node().end(), findElementByValue("local", "node", name));
  if (it == root().end())
  {
    Dout(dc::debug|flush_cf, name<<" No XML config found");
  }
  return it;
}

std::string IPv6XMLWrapManager::getNodeProperties(const xml::node& n, const char* attrName, bool required) const
{
  return SingularParser::Instance().getNodeProperties(n, attrName, required);
}

void IPv6XMLWrapManager::parseNetworkEntity(RoutingTable6* rt)
{
  static bool printXMLFile = false;
  if (!printXMLFile)
  {
    Dout(dc::notice, "XML Configuration file is "<<SingularParser::Instance()._filename);
    printXMLFile = true;
  }

  //const xml::node ne = *getNetNode(rt->nodeName());
  NodeIt it = getNetNode(rt->nodeName());
  if (it == root().end())
  {
    Dout(dc::warning, rt->nodeName()<<" no XML configuration found");
    return;
  }
  else
    Dout(dc::xml_addresses|flush_cf, rt->nodeName()<<" in parseNetworkEntity");

  const xml::node& ne = *it;
  //  parseNodeAttributes(rt);

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
  }
#endif //EDGEHANDOVER

#endif //USE_HMIP

#endif // USE_MOBILITY


  //Parse per interface attribute

  //Global interface defaults in global element does not really work because we
  //still take defaults from non specified ones anyway. To make it work we
  //cannot validate the document so that no default values are filled in. Or we
  //use XSD and the method of prototype objects.
  if (rt->interfaceCount() != count_if(ne.begin(), ne.end(), findElementByName("interface")))
  {
    Dout(dc::notice, rt->nodeName()<<" rt->interfaceCount()="<<rt->interfaceCount()<<" while xml interface count is "<<count_if(ne.begin(), ne.end(), findElementByName("interface")));
  }
  NodeIt startIf = ne.begin();
  for(size_t iface_index = 0; iface_index < rt->interfaceCount(); iface_index++, startIf++)
  {
    Interface6Entry& ie = rt->getInterfaceByIndex(iface_index);
    Interface6Entry::RouterVariables& rtrVar = ie.rtrVar;

    //startIf = find_if(startIf, ne.end(), findElementByName("interface"));
    startIf = ne.find("interface", startIf);
    if (startIf == ne.end())
    {
      Dout(dc::warning|flush_cf, " No more interfaces found in XML for ifIndex>="<<iface_index);
      //Should fill in for rest of unspecified interfaces the defaults in
      //global element when we get that happening.
      break;
    }
    const Node& nif = *startIf;

    rtrVar.advSendAds = getNodeProperties(nif, "AdvSendAdvertisements") == XML_ON;
    rtrVar.maxRtrAdvInt = boost::lexical_cast<double>(getNodeProperties(nif, "MaxRtrAdvInterval"));
    rtrVar.minRtrAdvInt = boost::lexical_cast<double>(getNodeProperties(nif, "MinRtrAdvInterval"));

#ifdef USE_MOBILITY
    rtrVar.advHomeAgent = getNodeProperties(nif, "AdvHomeAgent") == XML_ON;
    if (rt->mipv6Support)
    {
      rtrVar.maxRtrAdvInt = boost::lexical_cast<double>(getNodeProperties(nif, "MIPv6MaxRtrAdvInterval"));
      rtrVar.minRtrAdvInt = boost::lexical_cast<double>(getNodeProperties(nif, "MIPv6MinRtrAdvInterval"));
    if (rt->isRouter())
      Dout(dc::notice, rt->nodeName()<<":"<<iface_index<<" minRtrAdv="
           <<rtrVar.minRtrAdvInt
           <<" maxRtrAdv="<<rtrVar.maxRtrAdvInt);
    }

    Interface6Entry::mipv6Variables& mipVar = ie.mipv6Var;
    mipVar.maxConsecutiveMissedRtrAdv = boost::lexical_cast<unsigned>(getNodeProperties(nif, "MaxConsecMissRtrAdv"));

#endif // USE_MOBILITY
#if FASTRA
    try
    {
      rtrVar.maxFastRAS = boost::lexical_cast<unsigned int>(getNodeProperties(nif, "MaxFastRAS"));
    }
    catch(boost::bad_lexical_cast& e)
    {
      Dout(dc::warning|error_cf,  rt->nodeName()<<" Cannot convert into integer maxRas="<<getNodeProperties(nif, "MaxFastRAS"));
    }
    rtrVar.fastRA = rtrVar.maxFastRAS != 0;
    if (rt->isRouter())
      Dout(dc::notice, rt->nodeName()<<":"<<iface_index<<" fastRA="<<rtrVar.fastRA
           <<" maxFastRAS="<<rtrVar.maxFastRAS);
#endif //FASTRA

    rtrVar.advManaged = getNodeProperties(nif, "AdvManagedFlag") == XML_ON;
    rtrVar.advOther = getNodeProperties(nif, "AdvOtherConfigFlag") == XML_ON;
    rtrVar.advLinkMTU =  boost::lexical_cast<unsigned int>(getNodeProperties(nif, "AdvLinkMTU"));
    rtrVar.advReachableTime =  boost::lexical_cast<unsigned int>(getNodeProperties(nif, "AdvReachableTime"));
    rtrVar.advRetransTmr =  boost::lexical_cast<unsigned int>(getNodeProperties(nif, "AdvRetransTimer"));
    rtrVar.advCurHopLimit =  boost::lexical_cast<unsigned int>(getNodeProperties(nif, "AdvCurHopLimit"));
    //This is a hexadecimal number and as such is not covered by typical
    //stringstream smarts in conversion so lexical_cast won't work. Converted t
    //o decimal in schema now.
    rtrVar.advDefaultLifetime =  boost::lexical_cast<unsigned int>(getNodeProperties(nif, "AdvDefaultLifetime"));
    ie.mtu =  boost::lexical_cast<unsigned int>(getNodeProperties(nif, "HostLinkMTU"));
    ie.curHopLimit = boost::lexical_cast<unsigned int>(getNodeProperties(nif, "HostCurHopLimit"));
    ie.baseReachableTime = boost::lexical_cast<unsigned int>(getNodeProperties(nif, "HostBaseReachableTime"));
    ie.retransTimer = boost::lexical_cast<unsigned int>(getNodeProperties(nif, "HostRetransTimer"));
#if FASTRS
    ie.maxRtrSolDelay = boost::lexical_cast<double>(getNodeProperties(nif, "HostMaxRtrSolDelay"));
#endif // FASTRS
    ie.dupAddrDetectTrans = boost::lexical_cast<unsigned int>(getNodeProperties(nif, "HostDupAddrDetectTransmits"));

    ///Only some older XML files did not name their interfaces guess it is not
    ///really necessary as we do things in order anyway.
    ie.iface_name = getNodeProperties(nif, "name", false);

    it = nif.find("AdvPrefixList");
    if (it != nif.end())
    {
      const Node& napl = *it;

      //Parse prefixes
      size_t numOfPrefixes = count_if(napl.begin(), napl.end(), findElementByName("AdvPrefix"));
      ie.rtrVar.advPrefixList.resize(numOfPrefixes);
      if (numOfPrefixes == 0)
        Dout(dc::xml_addresses|dc::warning, rt->nodeName()
             <<" no prefixes even though advPrefixList exists");
      NodeIt startPr = napl.begin();
      for(size_t j = 0; j < numOfPrefixes; j++, startPr++)
      {
        //startPr = find_if(startPr, napl.end(), findElementByName("AdvPrefix"));
        startPr = napl.find("AdvPrefix", startPr);
        if (startPr == nif.end())
          assert(false);
        const Node& npr = *startPr;
        assert(startPr != nif.end());
        PrefixEntry& pe = ie.rtrVar.advPrefixList[j];
        pe._advValidLifetime =  boost::lexical_cast<unsigned int>(getNodeProperties(npr, "AdvValidLifetime"));
        pe._advOnLink =  getNodeProperties(npr, "AdvOnLinkFlag") == XML_ON;
        pe._advPrefLifetime =  boost::lexical_cast<unsigned int>(getNodeProperties(npr, "AdvPreferredLifetime"));
        pe._advAutoFlag = getNodeProperties(npr, "AdvAutonomousFlag") == XML_ON;
#ifdef USE_MOBILITY
        // Router Address Flag - for mobility support
        pe._advRtrAddr = getNodeProperties(npr, "AdvRtrAddrFlag") == XML_ON;
        pe.setPrefix(stripWhitespace(npr.get_content()).c_str(), !pe._advRtrAddr);
#else
        pe.setPrefix(stripWhitespace(npr.get_content()).c_str());
#endif //USE_MOBILITY
        Dout(dc::xml_addresses, "prefix advertised is "<<pe);
      }

    }
    //parse addresses
    size_t numOfAddrs = count_if(nif.begin(), nif.end(), findElementByName("inet_addr"));
    if (numOfAddrs > 0)
    {
      Dout(dc::xml_addresses|flush_cf|continued_cf, rt->nodeName()<<":"<<iface_index<<" ");
      NodeIt startAd = nif.begin();
      for (size_t k = 0; k < numOfAddrs; k++,startAd++)
      {
        //startAd = find_if(startAd, nif.end(), findElementByName("inet_addr"));
        startAd = nif.find("inet_addr", startAd);
        if (startAd == nif.end())
          assert(false);
        const Node& nad = *startAd;
        ie.tentativeAddrs.add(new IPv6Address(stripWhitespace(nad.get_content()).c_str()));
        Dout(dc::continued, "address "<< k << " is "<<stripWhitespace(nad.get_content())<<" ");
      }
      Dout( dc::finish, "-|" );
    }

  }

  // do an additional parameter check
  checkValidData (rt);
}


#ifdef USE_MOBILITY

/*
void IPv6XMLWrapManager::parseDualInterfaceInfo(DualInterfaceLayer* dualMod)
{
  assert(dualMod);

  // obtain movement info from XML
  const char* nodeName = OPP_Global::findNetNodeModule(dualMod)->name();

  NodeIt it = root().find("misc");
  if (it == root().end())
  {
    Dout(dc::notice, "No misc element hence cannot parse movementInfo");
    return;
  }

  const xml::node& ne = *it;

  it = find_if(ne.begin(), ne.end(), findElementByValue("local", "node", dualMod->getInterfaceName()));
}
*/

void IPv6XMLWrapManager::parseWirelessEtherInfo(WirelessEtherModule* wlanMod)
{
  assert(wlanMod);
  const char* nodeName = OPP_Global::findNetNodeModule(wlanMod)->name();

  NodeIt it = getNetNode(nodeName);
  if (it == root().end())
    return;

  const xml::node& ne = *it;

  it = find_if(ne.begin(), ne.end(), findElementByValue("interface", "name", wlanMod->getInterfaceName()));
  if (it == ne.end())
  {
    Dout(dc::notice, nodeName<<":"<<wlanMod->getInterfaceName()<<"no config found for "
         <<wlanMod->getInterfaceName());
    return;
  }

  NodeIt weIt = (*it).find("WirelessEtherInfo");
  if (weIt == (*it).end())
  {
    cerr<<"Node: "<< nodeName <<" - "
        << "Iface: " << wlanMod->getInterfaceName()
        << " Make sure you have <WirelessEtherInfo> "
        << " </WirelessEtherInfo> under <interface>"
        << endl;
    exit(1);
  }

  NodeIt infoIt = (*weIt).find("WEInfo");
  if (infoIt == (*weIt).end())
  {
       cerr << "Node: "<< nodeName <<" - "
         << "Iface: " << wlanMod->getInterfaceName()
         << " Make sure you have <WirelessEtherInfo> <WEInfo />"
         << " </WirelessEtherInfo> under <interface>"
         << endl;
    exit(1);
  }

  const Node& info = *infoIt;
  wlanMod->ssid = getNodeProperties(info, "WEssid");
  const_cast<double&>(wlanMod->pLExp) = boost::lexical_cast<double>(getNodeProperties(info, "WEPathLossExponent"));

  const_cast<double&>(wlanMod->pLStdDev) = boost::lexical_cast<double>(getNodeProperties(info, "WEPathLossStdDev"));
  const_cast<double&>(wlanMod->txpower) = boost::lexical_cast<double>(getNodeProperties(info, "WETxPower"));
  const_cast<double&>(wlanMod->threshpower) = boost::lexical_cast<double>(getNodeProperties(info, "WEThresholdPower"));
  const_cast<double&>(wlanMod->hothreshpower) = boost::lexical_cast<double>(getNodeProperties(info, "WEHOThresholdPower"));
  const_cast<double&>(wlanMod->probeEnergyTimeout) = boost::lexical_cast<double>(getNodeProperties(info, "WEProbeEnergyTimeout"));
  const_cast<double&>(wlanMod->probeResponseTimeout) = boost::lexical_cast<double>(getNodeProperties(info, "WEProbeResponseTimeout"));
  const_cast<double&>(wlanMod->authenticationTimeout) = boost::lexical_cast<double>(getNodeProperties(info, "WEAuthenticationTimeout"));
  const_cast<double&>(wlanMod->associationTimeout) = boost::lexical_cast<double>(getNodeProperties(info, "WEAssociationTimeout"));
  const_cast<unsigned int&>(wlanMod->maxRetry) = boost::lexical_cast<unsigned int>(getNodeProperties(info, "WERetry"));
  wlanMod->fastActScan = getNodeProperties(info, "WEFastActiveScan") == XML_ON;
  wlanMod->scanShortCirc = getNodeProperties(info, "WEScanShortCircuit") == XML_ON;
  wlanMod->crossTalk = getNodeProperties(info, "WECrossTalk") == XML_ON;
  wlanMod->shadowing = getNodeProperties(info, "WEShadowing") == XML_ON;
  wlanMod->chanNotToScan = getNodeProperties(info, "WEChannelsNotToScan");
  const_cast<unsigned int&>(wlanMod->sSMaxSample) = boost::lexical_cast<unsigned int>(getNodeProperties(info, "WESignalStrengthMaxSample"));

  std::string addr = getNodeProperties(info, "WEAddress");

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
    const_cast<double&>(wlanMod->bWRequirements) = boost::lexical_cast<double>(getNodeProperties(info, "WEBandwidthRequirements"));
    wlanMod->statsVec = getNodeProperties(info, "WERecordStatisticVector") == XML_ON;
    wlanMod->activeScan = getNodeProperties(info, "WEActiveScan") == XML_ON;
    const_cast<double&>(wlanMod->channelScanTime) = boost::lexical_cast<double>(getNodeProperties(info, "WEChannelScanTime"));
 // TODO: parse supported rate
}

void IPv6XMLWrapManager::parseMovementInfo(MobilityStatic* mod)
{
  // superclass for the (L1) mobile module
  MobileEntity* me = mod->mobileEntity;

  // obtain movement info from XML
  const char* nodeName = OPP_Global::findNetNodeModule(mod)->name();
  NodeIt it = root().find("misc");
  if (it == root().end())
  {
    Dout(dc::notice, "No misc element hence cannot parse movementInfo");
    return;
  }

  const Node& nmisc = *it;
  it = nmisc.find("ObjectMovement");
  if (it == nmisc.end())
  {
    DoutFatal(dc::core, "Cannot find ObjectMovement in misc");
  }

  const Node& nobj = *it;

  it = find_if(nobj.begin(), nobj.end(), findElementByValue("MovingNode", "NodeName", nodeName));
  if (it == nobj.end())
  {
    Dout(dc::debug|dc::notice, "No movement info for node "<<nodeName);
    return;
  }

  const Node& nmovenode = *it;
  me->setStartMovingTime(boost::lexical_cast<unsigned int>(getNodeProperties(nmovenode, "startTime")));

  for (it = nmovenode.begin();; it++)
  {
    //it = find_if(it, nmovenode.end(), findElementByName("move"));
    it = nmovenode.find("move", it);
    if (it == nmovenode.end())
      break;
    const Node& nmove = *it;
    me->addMove(boost::lexical_cast<unsigned int>(getNodeProperties(nmove, "moveToX")),
                boost::lexical_cast<unsigned int>(getNodeProperties(nmove, "moveToY")),
                boost::lexical_cast<unsigned int>(getNodeProperties(nmove, "moveSpeed"))
               );
  }
}

void IPv6XMLWrapManager::parseRandomWPInfo(MobilityRandomWP* mod)
{
  // obtain movement info from XML
  const char* nodeName = OPP_Global::findNetNodeModule(mod)->name();
  NodeIt it = root().find("misc");
  if (it == root().end())
  {
    Dout(dc::notice, "No misc element hence cannot parse RandomWPInfo");
    return;
  }

  const Node& nmisc = *it;
  it = nmisc.find("ObjectMovement");
  if (it == nmisc.end())
  {
    DoutFatal(dc::core, "Cannot find ObjectMovement in misc");
  }

  const Node& nobj = *it;

  it = find_if(nobj.begin(), nobj.end(), findElementByValue("RandomMovement", "RWNodeName", nodeName));
  if (it == nobj.end())
  {
    Dout(dc::debug|dc::notice, "No movement info for node "<<nodeName);
    return;
  }

  const Node& nmovenode = *it;
  mod->minX = boost::lexical_cast<unsigned int>(getNodeProperties(nmovenode, "RWMinX"));
  mod->maxX = boost::lexical_cast<unsigned int>(getNodeProperties(nmovenode, "RWMaxX"));
  mod->minY = boost::lexical_cast<unsigned int>(getNodeProperties(nmovenode, "RWMinY"));
  mod->maxY = boost::lexical_cast<unsigned int>(getNodeProperties(nmovenode, "RWMaxY"));
  mod->moveInterval = boost::lexical_cast<double>(getNodeProperties(nmovenode, "RWMoveInterval"));
  mod->minSpeed = boost::lexical_cast<double>(getNodeProperties(nmovenode, "RWMinSpeed"));
  mod->maxSpeed = boost::lexical_cast<double>(getNodeProperties(nmovenode, "RWMaxSpeed"));
  mod->pauseTime = boost::lexical_cast<double>(getNodeProperties(nmovenode, "RWPauseTime"));
  mod->distance = boost::lexical_cast<unsigned int>(getNodeProperties(nmovenode, "RWDistance"));
}

void IPv6XMLWrapManager::parseRandomPatternInfo(MobilityRandomPattern* mod)
{
  // obtain movement info from XML
  const char* nodeName = OPP_Global::findNetNodeModule(mod)->name();
  NodeIt it = root().find("misc");
  if (it == root().end())
  {
    Dout(dc::notice, "No misc element hence cannot parse RandomPatternInfo");
    return;
  }

  const Node& nmisc = *it;
  it = nmisc.find("ObjectMovement");
  if (it == nmisc.end())
  {
    DoutFatal(dc::core, "Cannot find ObjectMovement in misc");
  }

  const Node& nRandompattern = *it;
  it = find_if(nRandompattern.begin(), nRandompattern.end(), findElementByName("RandomPattern"));

  if (it == nRandompattern.end())
    DoutFatal(dc::core, "Cannot find RandomPattern in ObjectMovement");

  if ( !mod->isRandomPatternParsed )
  {
    const Node& nrandompattern = *it;

    mod->xSize = boost::lexical_cast<unsigned int>(getNodeProperties(nrandompattern, "RPXSize"));
    mod->ySize = boost::lexical_cast<unsigned int>(getNodeProperties(nrandompattern, "RPYSize"));
    mod->moveInterval = boost::lexical_cast<unsigned int>(getNodeProperties(nrandompattern, "RPMoveInterval"));
    mod->minSpeed = boost::lexical_cast<double>(getNodeProperties(nrandompattern, "RPMinSpeed"));
    mod->maxSpeed = boost::lexical_cast<double>(getNodeProperties(nrandompattern, "RPMaxSpeed"));
    mod->distance = boost::lexical_cast<unsigned int>(getNodeProperties(nrandompattern, "RPDistance"));
    mod->pauseTime = boost::lexical_cast<double>(getNodeProperties(nrandompattern, "RPPauseTime"));
  }

  const Node& nobj = *it;

  it = find_if(nobj.begin(), nobj.end(), findElementByValue("RPNode", "RPNodeName", nodeName));
  if (it == nobj.end())
  {
    Dout(dc::debug|dc::notice, "No X/Y offsets of movement pattern info for node "<<nodeName);
    return;
  }

  const Node& nmovenode = *it;
  mod->xOffset = boost::lexical_cast<unsigned int>(getNodeProperties(nmovenode, "RPXOffset"));
  mod->yOffset = boost::lexical_cast<unsigned int>(getNodeProperties(nmovenode, "RPYOffset"));
}

#endif //USE_MOBILITY

void IPv6XMLWrapManager::staticRoutingTable(RoutingTable6* rt)
{
  NodeIt it = getNetNode(rt->nodeName());
  if (it == root().end())
  {
    Dout(dc::warning, rt->nodeName()<<" no XML configuration found");
    return;
  }
  else
    Dout(dc::xml_addresses|flush_cf, rt->nodeName()<<" in staticRoutingTable");

  const xml::node& ne = *it;
  it = ne.find("route");
  if (it == ne.end())
  {
    Dout(dc::debug, rt->nodeName()<<" does not have static routes in XML");
    return;
  }
  const Node& nroute = *it;
  std::string iface, dest, nextHop;
  for (it = nroute.begin(); it != nroute.end(); it++)
  {
    it = nroute.find("routeEntry", it);
    if (it == nroute.end())
      break;
    const Node& nre = *it;
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

void IPv6XMLWrapManager::tunnelConfiguration(RoutingTable6* rt)
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

  NodeIt it = getNetNode(rt->nodeName());
  const Node& netNode = *it;
  it = netNode.find("tunnel");
  if (it == netNode.end())
  {
    Dout(dc::debug|dc::encapsulation, rt->nodeName()<<" has no XML tunneling configuration");
    return;
  }
  else
    Dout(dc::xml_addresses|flush_cf, rt->nodeName()<<" in tunnelConfiguration");

  const Node& ntun = *it;
  for (it = ntun.begin();;it++)
  {
    it = ntun.find("tunnelEntry", it);
    if (it == ntun.end())
      break;
    const Node& nte = *it;
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

    for ( NodeIt trigIt = nte.begin();;trigIt++)
    {
      trigIt = nte.find("triggers", trigIt);
      if (trigIt == nte.end())
        break;
      const Node& ntrig = *trigIt;
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

void IPv6XMLWrapManager::sourceRoute(RoutingTable6* rt)
{
  using namespace OPP_Global;
  IPv6ForwardCore* forwardMod = polymorphic_downcast<IPv6ForwardCore*>
    (findModuleByTypeDepthFirst(rt, "IPv6ForwardCore"));
  //Even though downcast detects incorrect downcasts it still allows casting 0
  //down to anything
  assert(forwardMod != 0);

  NodeIt it = getNetNode(rt->nodeName());
  const Node& netNode = *it;

  it = netNode.find("sourceRoute");
  if (it == netNode.end())
  {
    Dout(dc::debug|dc::forwarding, "No source routes for "<<rt->nodeName()
	 <<" in XML config");
    return;
  }
  else
    Dout(dc::xml_addresses|flush_cf, rt->nodeName()<<" in sourceRoute");

  const Node& nsource = *it;

  for (it = nsource.begin();;it++)
  {
    it = nsource.find("sourceRouteEntry", it);
    if (it == nsource.end())
      break;
    const Node& nsre = *it;
    size_t nextHopCount = count_if(nsre.begin(), nsre.end(), findElementByName("nextHop"));
    SrcRoute route(new _SrcRoute(nextHopCount + 1, IPv6_ADDR_UNSPECIFIED));
    (*route)[nextHopCount] = c_ipv6_addr(getNodeProperties(nsre, "finalDestination").c_str());

    NodeIt nextIt = nsre.begin();
    for ( size_t j = 0 ; j < nextHopCount; nextIt++, j++)
    {
      nextIt = nsre.find("nextHop", nextIt);
      assert(nextIt != nsre.end());
      const Node& nnh = *nextIt;
      (*route)[j] = c_ipv6_addr(getNodeProperties(nnh, "address").c_str());
    }
    forwardMod->addSrcRoute(route);
  }

}

#ifdef USE_HMIP
void IPv6XMLWrapManager::parseMAPInfo(RoutingTable6* rt)
{
  NeighbourDiscovery* ndmod = static_cast<NeighbourDiscovery*>(OPP_Global::findModuleByName(rt, "nd"));

  HierarchicalMIPv6::HMIPv6NDStateRouter* hmipRtr = dynamic_cast<HierarchicalMIPv6::HMIPv6NDStateRouter*>(ndmod->nd);

  if (!hmipRtr)
    return;

  NodeIt it = getNetNode(rt->nodeName());
  if (it == root().end())
  {
    Dout(dc::warning, rt->nodeName()<<" no XML configuration found");
  }
  const xml::node& ne = *it;

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

  NodeIt startIf = ne.begin();
  for(size_t i = 0; i < rt->interfaceCount(); i++, ++startIf)
  {

    startIf = ne.find("interface", startIf);
    if (startIf == ne.end())
    {
      Dout(dc::warning|flush_cf, " No more interfaces found in XML for ifIndex>="<<i);
      return;
    }
    const Node& nif = *startIf;
    it = nif.find("AdvMAPList");
    if (it == nif.end())
    {
      Dout(dc::xml_addresses|dc::debug, rt->nodeName()<<":"<<i<<" does not have a map to advertise in XML");
      continue;
    }
    const Node& nmaplist = *it;
    for (it = nmaplist.begin();; it++)
    {
      it = nmaplist.find("AdvMAPEntry", it);
      if (it == nmaplist.end())
        break;
      const Node& nmap = *it;
      unsigned int dist = boost::lexical_cast<unsigned int>(getNodeProperties(nmap, "AdvMAPDist"));
      unsigned int pref = boost::lexical_cast<unsigned int>(getNodeProperties(nmap, "AdvMAPPref"));
      unsigned int expires = boost::lexical_cast<unsigned int>(getNodeProperties(nmap, "AdvMAPValidLifetime"));
      ipv6_addr map_addr = c_ipv6_addr(stripWhitespace(nmap.get_content()).c_str());
      HierarchicalMIPv6::HMIPv6ICMPv6NDOptMAP mapOpt(dist, pref, expires, map_addr);
      mapOpt.iface_idx = i;

      hmipRtr->addMAP(mapOpt);

      Dout(dc::xml_addresses|dc::debug, rt->nodeName()<<":"<<i<<" advMap="<<HierarchicalMIPv6::HMIPv6MAPEntry(mapOpt));
    }
  }
}
#endif // USE_HMIP
/// parse node level attributes
void IPv6XMLWrapManager::parseNodeAttributes(RoutingTable6* rt)
{

}

///Returns a string for further tokenising into logfile & debug channel names
std::string  IPv6XMLWrapManager::retrieveDebugChannels()
{
  return getNodeProperties(root(), "debugChannel", false);
}


}//namespace XMLConfiguration




#if defined USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>


/**
   @class IPv6XMLWrapManagerTest
   @brief Unit test IPv6XMLWrapManager
   @ingroup TestCases
*/

class IPv6XMLWrapManagerTest: public CppUnit::TestFixture
{

  CPPUNIT_TEST_SUITE( IPv6XMLWrapManagerTest );
  CPPUNIT_TEST( testXmlWrap );
  CPPUNIT_TEST_SUITE_END();

 public:

  // Constructor/destructor.
  IPv6XMLWrapManagerTest();
  virtual ~IPv6XMLWrapManagerTest();

  void testXmlWrap();

  void setUp();
  void tearDown();

private:

  // Unused ctor and assignment op.
  IPv6XMLWrapManagerTest(const IPv6XMLWrapManagerTest&);
  IPv6XMLWrapManagerTest& operator=(const IPv6XMLWrapManagerTest&);
  XMLConfiguration::IPv6XMLWrapManager* p;
};

CPPUNIT_TEST_SUITE_REGISTRATION( IPv6XMLWrapManagerTest );

IPv6XMLWrapManagerTest::IPv6XMLWrapManagerTest():p(0)
{
}

IPv6XMLWrapManagerTest::~IPv6XMLWrapManagerTest()
{
}
///@warning act of creating new XMLConfiguration will overwrite global
///object. As XML is used at initialise only this is of no consequence. However
///if we want to save state then turn off this test or modify
void IPv6XMLWrapManagerTest::setUp()
{
}

void IPv6XMLWrapManagerTest::tearDown()
{
  delete p;
}

void IPv6XMLWrapManagerTest::testXmlWrap()
{

  using namespace XMLConfiguration;
  CPPUNIT_ASSERT(1==1);

  if (string("testnet") != simulation.systemModule()->name())
    return;

  p = new XMLConfiguration::IPv6XMLWrapManager("XMLTEST.xml");

  std::string ver = p->getNodeProperties(p->root(), "version");
  cerr<<" ver="<<ver<<" length="<<ver.length()<<endl;

  CPPUNIT_ASSERT(ver == "0.0.3");

  assert(p->getNetNode("router") != p->root().end());
  IPv6XMLWrapManager::NodeIt it = p->getNetNode("router");
  const xml::node& ne = *it;
  CPPUNIT_ASSERT(p->getNodeProperties(ne, "routePackets") == XML_ON);
  CPPUNIT_ASSERT(p->getNodeProperties(ne, "forwardSitePackets") == XML_ON);
  CPPUNIT_ASSERT(p->getNodeProperties(ne, "mobileIPv6Support") == XML_ON);
  CPPUNIT_ASSERT(p->getNodeProperties(ne, "hierarchicalMIPv6Support") != XML_ON);
  CPPUNIT_ASSERT(p->getNodeProperties(ne, "mobileIPv6Role") == "HomeAgent");

  it = ne.find("interface");
  const IPv6XMLWrapManager::Node& iface = *it;
  cerr<<"1st iface "<<stripWhitespace(iface.get_content())<<endl;
  CPPUNIT_ASSERT(stripWhitespace(iface.get_content()) == string("fe80:0:0:0:260:97ff:0:5/64"));
  IPv6XMLWrapManager::NodeIt nextIt = it;
  //cerr<<"next interface"<<stripWhitespace((*ne.find("interface", ++it)).get_content())<<endl;
  CPPUNIT_ASSERT(stripWhitespace((*ne.find("interface", ++nextIt)).get_content()) == string("FEC0:0:0:ABCD:0:0:0:0/64"));
  CPPUNIT_ASSERT(stripWhitespace((*ne.find("interface", ++nextIt)).get_content()) == string("FEC0:0:0:ABCD:0:0:0:0/64"));
  //get next interface
  it = ne.find("interface", ++it);
  CPPUNIT_ASSERT(it != ne.end());
  const IPv6XMLWrapManager::Node& iface2 = *it;
  it = iface2.find("AdvPrefixList");
  CPPUNIT_ASSERT(it != iface2.end());


  const IPv6XMLWrapManager::Node& apl = *it;
  it = apl.find("AdvPrefix");
  CPPUNIT_ASSERT(it != apl.end());
  const IPv6XMLWrapManager::Node& nprefix = *it;

  //The line below is alternative to previous block and will cause segfault
  //because the temporary const iterator from the find() call is same as
  // nprefix's internal pointer? Thus we will always need to copy the
  //iterator to retain a copy of it after the unnamed temporary goes out of scope
  //to retain node. (Valgrind analysis proved this)

  //  const IPv6XMLWrapManager::Node& nprefix = *((*it).find("AdvPrefix")); //*it;
  CPPUNIT_ASSERT(stripWhitespace(nprefix.get_content()) == string("FEC0:0:0:ABCD:0:0:0:0/64"));
  CPPUNIT_ASSERT(888888 == boost::lexical_cast<unsigned int>(p->getNodeProperties(nprefix, "AdvValidLifetime")));
  CPPUNIT_ASSERT(88888 == boost::lexical_cast<unsigned int>(p->getNodeProperties(nprefix, "AdvPreferredLifetime")));
}


#endif //defined USE_CPPUNIT
