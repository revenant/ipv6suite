//
// Copyright (C) 2001, 2002, 2003, 2004 CTIE, Monash University
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
 *  @file RoutingTable6.cc
 *  @brief Manage requests to the routing table and the interface table
 *
 *  @author  Eric Wu, Johnny Lai
 *
 *  @date    19/9/2001
 *
 */

//These two headers have to come first if libcwd macros are in use
#include "sys.h"
#include "debug.h"
#include "config.h"

#include "RoutingTable6.h"

#include <climits>
#include <functional>
#include <algorithm>
#include <boost/cast.hpp>
#include "opp_utils.h"  // for int/double <==> string conversions
#include <sstream> //stringstream

#include "opp_utils.h"
#include "cTypedMessage.h"
#include "Messages.h"
#include "cTimerMessage.h"
#include "NDTimers.h"
#include "NDEntry.h"
#include "IPv6Datagram.h"
#include "NeighbourDiscovery.h" //Tmr_AddrConf message_id
#include "IPv6Encapsulation.h"
#include "IPv6ForwardCore.h"
#include "IPv6CDS.h"
#include "MLD.h"

#ifdef USE_MOBILITY
#include "MIPv6CDSMobileNode.h"
#include "WirelessEtherModule.h"
#endif //USE_MOBILITY
#include "WorldProcessor.h"

// XXX TBD try to eliminate these dependencies
#include "LinkLayerModule.h"
#include "EtherModule.h"
#include "IPv6PPPAPInterface.h"



#ifdef _MSC_VER
#define strcasecmp  stricmp
#endif


namespace
{
  const size_t MAX_IPv6INTERFACE_NO = 30;
}

/*
 * Print on the console (first choice) or in the OMNeT window
 * (second choice).
 */
#define PRINTF  printf
//#define PRINTF  ev.printf

Define_Module( RoutingTable6 );

void RoutingTable6::initialize(int stage)
{
  if(stage == 0)
  {
    addrExpiryTmr = 0;
    IPForward = false;
    forwardSitePacket = true;
#ifdef USE_MOBILITY
    mipv6Support = false;
#endif
#ifdef USE_HMIP
    hmipv6Support = false;
#endif //USE_HMIP
    odadSupport = false;
    ctrIcmp6OutMsgs = 0;
    //Give it plenty of room so it doesn't have to reallocate too much
    removeHolesArr.reserve(20);

    interfaces.reserve(MAX_IPv6INTERFACE_NO<interfaces.max_size()?
                       MAX_IPv6INTERFACE_NO:
                       interfaces.max_size());

    numOfIfaces = par("numOfPorts").longValue();
    displayIfconfig = par("displayIfconfig").boolValue();

     // add in loopback InterfaceEntry
    Interface6Entry loopback_iface;
    //loopback_iface.loopback = true;
    loopback_iface.iface_name = "lo";

    IPv6Address loopback_adr(LOOPBACK_ADDRESS);

    loopback_iface.inetAddrs.push_back(loopback_adr);
    interfaces.push_back(loopback_iface);

    interfaces.resize(numOfIfaces + 1);

    wp = check_and_cast<WorldProcessor*>
      (OPP_Global::iterateSubMod(simulation.systemModule(), "WorldProcessor"));
    assert(wp != 0);
    // XXX try
    // XXX {
      //parse this network node's parameters and load them into this.
      wp->parseNetworkEntity(this);
    // XXX }
    // XXX catch(boost::bad_lexical_cast& e)
    // XXX {
    // XXX   Dout(dc::warning|error_cf, nodeName()<<" "<<e.what());
    // XXX }

    cModule* ICMP = OPP_Global::findModuleByName(this,"ICMP");
    assert(ICMP);
    mld = static_cast<MLD*>(OPP_Global::findModuleByTypeDepthFirst(ICMP,"MLD"));
    assert(mld);

    //Create here for now
    cds = new IPv6NeighbourDiscovery::IPv6CDS();

    rel = new ExpiryEntryList<RouterEntry*, Loki::cTimerMessageCB<void> >
      (new Loki::cTimerMessageCB<void>
       (IPv6NeighbourDiscovery::Tmr_RouterLifetime, this, this,
        &RoutingTable6::routerTimeout,
        "RouterExpiryTmr"), true);

    pel = new ExpiryEntryList<PrefixEntry*, Loki::cTimerMessageCB<void> >
      (new Loki::cTimerMessageCB<void>
       (IPv6NeighbourDiscovery::Tmr_PrefixLifetime, this, this,
        &RoutingTable6::prefixTimeout,
        "PrefixExpiryTmr"), true);

  }
  else if(stage == 1)
  {
    for(size_t i = 0; i < numOfIfaces; i ++)
    {
      // label interface name according to the link layer
      cModule* mod = OPP_Global::findModuleByType(this, "LinkLayer6");
      LinkLayerModule* llmodule = 0;
      mod = mod->parentModule()->submodule("linkLayers", i);

      if(mod)
      {
        llmodule = (LinkLayerModule*)(mod->submodule("networkInterface"));

        // TODO: PLEASE DO A GRACEFUL SHUTDOWN
        if(!interfaces[i+1].iface_name.empty() &&
           strcasecmp(interfaces[i+1].iface_name.c_str(), llmodule->getInterfaceName()))
        {
          cerr<< endl <<nodeName()<<":"<<i+1
              <<" Check the network interface names in XML file: "
              <<interfaces[i+1].iface_name<<endl
              <<" wi  th corresponding OMNeT++ MAC iface: "
              <<llmodule->getInterfaceName()<<endl;
          exit(1);
        }

        // We already know that the loopback interface name and we just
        // need to find out the network interface name in the link layer
        interfaces[i+1].iface_name = llmodule->getInterfaceName();
        interfaces[i+1].linkMod = llmodule;

        // Assign interfaceID
        switch(llmodule->getInterfaceType())
        {
          case PR_ETHERNET:
            {
              EtherModule* eth_iface = (EtherModule*)llmodule;
              unsigned int iid[2];
              iid[0] = ( eth_iface->macAddress()[0] << 8 ) | 0xFF;
              iid[1] = eth_iface->macAddress()[1] | 0xFE000000;
              interfaces[i+1].setInterfaceID(iid);
              //       std::stringstream ss;
              //ss << std::hex << iid[0] << iid[1];
              interfaces[i+1].setLLAddr(eth_iface->macAddressString());
            }
            break;

#ifdef USE_MOBILITY
          case PR_WETHERNET:
            {
              WirelessEtherModule* eth_iface = (WirelessEtherModule*)llmodule;
              unsigned int iid[2];
              iid[0] = ( eth_iface->macAddress()[0] << 8 ) | 0xFF;
              iid[1] = eth_iface->macAddress()[1] | 0xFE000000;
              interfaces[i+1].setInterfaceID(iid);
              //       std::stringstream ss;
              //ss << std::hex << iid[0] << iid[1];
              interfaces[i+1].setLLAddr(eth_iface->macAddressString());
            }
            break;
#endif // USE_MOBILITY

            case PR_PPP:
            {
              IPv6PPPInterface* ppp_iface = (IPv6PPPInterface*)llmodule;
              unsigned int iid[2];
              iid[0] = ppp_iface->highInterfaceId();
              iid[1] = ppp_iface->lowInterfaceId();
              interfaces[i+1].setInterfaceID(iid);
              std::stringstream ss1, ss2;
              ss1 << std::hex << iid[0];
              ss2 << std::hex << iid[1];
              string lladdr = ss1.str() + string(":") + ss2.str();
              interfaces[i+1].setLLAddr(lladdr);
            }
            break;
        }
      }
    }

    //Added static routes, source routing and tunneling, This can only be done
    //after interface names have been assigned i.e. couldn't get
    //parseNetworkEntity completed here at same time.
    wp->staticRoutingTable(this);
    // test function
    if (displayIfconfig)
      print();
  } //end stage 1
  else if( stage == 2 )
  {
#ifdef USE_HMIP
    if (hmipSupport())
      wp->parseMAPInfo(this);
#endif //USE_HMIP
  }
}

/// handleMessage just throws the message away
void RoutingTable6::handleMessage(cMessage* msg)
{
  if (msg->isSelfMessage())
  {
    check_and_cast<cTimerMessage *> (msg)->callFunc();
  }
  else
    assert(false);

  //if addr
}

void RoutingTable6::finish()
{
  //opp drop needed but inaccessible but opp will delete for us anyway
  //delete addrExpiryTmr;
  addrExpiryTmr = 0;

  delete cds;
  delete pel;
  delete rel;
  cds = 0;
}

void RoutingTable6::addRoute(unsigned int ifIndex, IPv6Address& nextHop,
                             const IPv6Address& dest, bool destIsHost)
{
  //Distinguish between two types of Routing entries

  //normal next hop route

  //Next Hop is a router ip address on same link as interface to forward
  //things with destination.  Destination can either be a network prefix
  //(prefix length < 128) or a host ipv6 address (prefix length = 128)

  //Default route - special case of next hop route
  //Destination is the unspecified IPv6 address 0::0

  //neighbouring node route
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


  if (destIsHost && nextHop == IPv6_ADDR_UNSPECIFIED || nextHop == dest)
  {
    if (dest.prefixLength() > 0 && dest.prefixLength() < IPv6_ADDR_LENGTH)
    {
      insertPrefixEntry(PrefixEntry(dest, VALID_LIFETIME_INFINITY), ifIndex);
      Dout(dc::neighbour_disc, nodeName()<<" Added on-link subnet prefix "
           <<dest<<" for "<<ifIndex); //was ifaceName
      return;
    }
    //Create DE and NE
    cds->insertNeighbourEntry( new NeighbourEntry(dest, ifIndex));

    Dout(dc::neighbour_disc|dc::routing, nodeName()<<" Neighbouring Host added ifIndex="
         <<ifIndex<<" dest="<<dest);
    return;
  }

  if (!destIsHost)
    nextHop = dest;


  RouterEntry* re = 0;
  NeighbourEntry* ne = 0;
  //By definition Next Hop has to be router in order to forward packets to
  //dest. Destination can be a network prefix with "0"s for non prefix bits.
  bool createEntry = true;

  //Check if routerEntry has already been created
  if ((ne = (cds->neighbour(nextHop)).lock().get()) != 0)
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
      Dout(dc::warning, nodeName()<<" specifies a router at "<<nextHop
           <<", conflicts with previous entry for a neighbouring host."
           <<" Neighbouring host entry removed");
      cerr<<nodeName()<<" specifies a router at "<<nextHop
          <<", conflicts with previous entry for a neighbouring host."
          <<" Neighbouring host entry removed"
          <<endl;
      cds->removeNeighbourEntry(ne->addr());
    }
  }

  if (createEntry)
  {
    re = new RouterEntry(nextHop, ifIndex, "",
                         VALID_LIFETIME_INFINITY);
    re->setState(NeighbourEntry::INCOMPLETE);
    insertRouterEntry(re);
    Dout(dc::router_disc|dc::routing, nodeName()<<" New router added addr="<<nextHop
         <<" ifIndex="<<ifIndex<<" with infinite lifetime");
  }

  if (!(dest == IPv6_ADDR_UNSPECIFIED))
  {

    (*cds)[dest].neighbour = cds->router(re->addr());

    if (dest != nextHop && !( dest == IPv6_ADDR_UNSPECIFIED))
      Dout(dc::forwarding|dc::routing|dc::xml_addresses, nodeName()
           <<" Next hop for "<<dest <<" is "<<nextHop);
  }
  else
  {
    //Dout(dc::router_disc|dc::routing, nodeName()<<" "<<nextHop<<" is the default route");
    Dout(dc::router_disc|dc::routing, nodeName()<<" "<<re->addr()
         <<" is the default route "
         <<cds->router(re->addr()).lock()->addr());


    //Still problem of creating many router objects even though they have the
    //same value. Not only that the default route of 0.0.0.0 was added into
    //the DC this should not be required.
    cds->setDefaultRouter(cds->router(re->addr()));
  }
}

void RoutingTable6::prefixTimeout()
{
  PrefixEntry* ppe = pel->removeExpiredEntry(Loki::Int2Type<Loki::TypeTraits<PEL::ElementType>::isPointer>());
  assert(ppe);
  Dout(dc::prefix_timer|flush_cf, nodeName()<<" "<<*ppe<<" removed at "<<simTime());
  cds->removePrefixEntry(*ppe);
}

///Associate the prefix with interface link on which it belongs
void RoutingTable6::insertPrefixEntry(const PrefixEntry& pe, size_t ifIndex)
{
  OPP_Global::ContextSwitcher switcher(this);
  PrefixEntry* ppe = 0;
  ppe = cds->insertPrefixEntry(pe, ifIndex);
  assert(ppe != 0);
//   ppe->setTimer(
//     new IPv6NeighbourDiscovery::PrefixExpiryTmr(this, ppe,
//                                                 simTime() + pe.advValidLifetime()));
//   ppe->timer()->setOwner(this);
  if (pe.advValidLifetime() != VALID_LIFETIME_INFINITY)
  {
    Dout(dc::neighbour_disc|flush_cf, nodeName()<<" "<<simTime()
         <<" PE "<<pe<<" will be inserted into list->"<<*pel);
    pel->addEntry(ppe);
  }
}

void RoutingTable6::removePrefixEntry(PrefixEntry* ppe)
{
  OPP_Global::ContextSwitcher switcher(this);
  assert(ppe != 0);
  Dout(dc::neighbour_disc|flush_cf, nodeName()<<" "<<simTime()
       <<" PE "<<*ppe<<" may be removed from list->"<<*pel);
  if (ppe->advValidLifetime() != VALID_LIFETIME_INFINITY)
    pel->removeEntry(ppe);
  cds->removePrefixEntry(*ppe);
}

void RoutingTable6::routerTimeout()
{
  //RouterEntry* re = rel->removeExpiredEntry(Loki::Int2Type<true>());
  RouterEntry* re = rel->removeExpiredEntry(Loki::Int2Type<Loki::TypeTraits<REL::ElementType>::isPointer>());
  assert(re);
  Dout(dc::router_timer|flush_cf, nodeName()<<" "<<*re<<" removed at "<<simTime());
  cds->removeRouterEntry(re->addr());
}

/// insert router entry into the list
void RoutingTable6::insertRouterEntry(RouterEntry* re, bool setDefault)
{
  OPP_Global::ContextSwitcher switcher(this);
  assert(re);
  cds->insertRouterEntry(re, setDefault);
/*
  re->setTimer(
     new IPv6NeighbourDiscovery::RouterExpiryTmr(this, re,
                                                 simTime() + re->invalidTime()));
  re->timer()->setOwner(this);
*/
  if (re->invalidTime() != VALID_LIFETIME_INFINITY)
  {
    Dout(dc::router_disc|flush_cf, nodeName()<<" "<<simTime()
         <<" RE="<<*re<<" will be inserted into list->"<<*rel);
    rel->addEntry(re);
  }
}

void RoutingTable6::removeRouterEntry(RouterEntry* re)
{
  OPP_Global::ContextSwitcher switcher(this);
  assert(re);
  Dout(dc::router_disc|flush_cf, nodeName()<<" "<<simTime()
        <<" RE="<<*re<<" may be removed from list->"<<*rel);
  if (re->invalidTime() != VALID_LIFETIME_INFINITY)
    rel->removeEntry(re);
  cds->removeRouterEntry(re->addr());
}

#ifdef USE_MOBILITY
bool RoutingTable6::awayFromHome() const
{
  assert(mobilitySupport());
  MobileIPv6::MIPv6CDSMobileNode* mipv6cdsMN =
    boost::polymorphic_downcast<MobileIPv6::MIPv6CDSMobileNode*>(mipv6cds);
  return mipv6cdsMN->awayFromHome();
}

void RoutingTable6::setMobilitySupport(bool mipv6)
{
  mipv6Support = mipv6;
}

#endif //USE_MOBILITY

// --------------
//  Print tables
// --------------
void RoutingTable6::print()
{
  const char* node_name = OPP_Global::findNetNodeModule(this)->name();

  PRINTF("\n\n============================================================================ \n");
  PRINTF("IPv6 ifconfig for NODE %s id:%d\n", node_name, nodeId());
  PRINTF("============================================================================ \n");

  for (size_t i =0; i < interfaces.size(); i++)
  {
    if(interfaces[i].iface_name != "lo")
    {
      interfaces[i].print(IPForward);
      Dout(dc::xml_addresses, "HWAddr "<<nodeName()<<":"<<i-1<<" "
           <<interfaces[i].LLAddr());
    }
  }
  PRINTF("==============  Node Level Configuration Variables  ======================== \n");

  PRINTF("IPForward: \t\t %d \n", IPForward);
  PRINTF("ForwardSitePackets: \t %d \n", forwardSitePacket);

#ifdef USE_MOBILITY
  PRINTF("MIPv6 Support: \t\t %d \n", mipv6Support);

  if (mipv6Support)
  {
    switch (role)
    {
        case HOME_AGENT:
          PRINTF("MIPv6 Role: \t Home Agent \n");
          break;
        case MOBILE_NODE:
          PRINTF("MIPv6 Role: \t\t Mobile Node \n");
          break;
        case CORRESPONDENT_NODE:
          PRINTF("MIPv6 Role: \t\t Correspondent Node \n");
          break;
        default:
          PRINTF("MIPv6 Role: \t\t UNKNOWN! \n");
          break;
    }
  }
#endif // USE_MOBILITY

  PRINTF("============================================================================ \n");

  IPv6Encapsulation* tunMod = check_and_cast<IPv6Encapsulation*>
    (OPP_Global::findModuleByName(this, "tunneling"));
  assert(tunMod != 0);

  cout<<(*tunMod);

  IPv6ForwardCore* forwardMod = check_and_cast<IPv6ForwardCore*>
    (OPP_Global::findModuleByTypeDepthFirst(this, "IPv6ForwardCore"));
  cout<<(*forwardMod);
}

// -----------------------------------
//  Access of interface/routing table
// -----------------------------------

/*
 * Look if the address is a local one, ie one of the host
 */

bool RoutingTable6::localDeliver(const ipv6_addr& dest)
{
  //look through all interfaces to check for matching dest

  //TODO Use DC to add all local addr and have them pointing to
  //loopback interface so when interface is loopback send to LocalDeliver.
  //Same can be done for Prefixes too.

  if (!dest.isMulticast())
  {
    for(size_t i=0; i<interfaces.size(); i++)
    {
      if (interfaces[i].addrAssigned(dest))
        return true;

      if (odad()
#if defined USE_MOBILITY
          && mobilitySupport() && isMobileNode()// && awayFromHome())
#endif //defined USE_MOBILITY
         )
      {
        //Tentative addresses considered assigned while DAD is carried out
        if (interfaces[i].tentativeAddrAssigned(dest))
          return true;
      }
    }
// possible implementation of efficient local addr lookup once a subclass of
// NeighbourEntry contains an IPv6Address pointer
//     boost::weak_ptr<NeighbourEntry> ne = cds->neighbour(dest);
//     if (ne.get() != 0 && ne->addr() == LOOPBACK_ADDRESS)
//       return true;
  }
  else
  {
    for (size_t i = 0; i < multicastGroup.size(); i++)
    {
      if (multicastGroup[i] == dest)
      {
        return true;
      }
    }
  }

  return false;
}

/*
 * Return the InterfaceEntry specified by its index.
 */

Interface6Entry& RoutingTable6::getInterfaceByIndex(unsigned int index)
{
  if (index + 1 >= interfaces.size())
    DoutFatal(dc::core, nodeName()<<" index is "<<index<<" while interfaces.size"
              <<interfaces.size());
  //Want to return only real interfaces not the loop back
  return interfaces[index + 1];
}

/*
 * Return the InterfaceEntry specified by its index. Const version
 */
const Interface6Entry& RoutingTable6::getInterfaceByIndex(unsigned int index) const
{
  if (index + 1 >= interfaces.size())
    DoutFatal(dc::core, nodeName()<<" index is "<<index<<" while interfaces.size"
              <<interfaces.size());
  //Want to return only real interfaces not the loop back
  return interfaces[index + 1];
}

/*
 * Search the index of an interface given by its name.
 * Return -1 on error.
 */
int RoutingTable6::interfaceNameToNo(const char *iface_name) const
{
  if (iface_name == NULL) return -1;

  for (size_t i = 0; i < interfaces.size(); i++)
  {
    if (interfaces[i].iface_name == iface_name)
      // the interface index corresponds to the port index
          return i-1;
  }

  return -1;
}

/*
 * Search the index of an interface given by its address.
 * Return -1 if not found.
 */
int RoutingTable6::interfaceAddressToNo(const IPv6Address& addr) const
{
  for (size_t i = 0; i < interfaces.size(); i++)
  {
    for (size_t j = 0; j < interfaces[i].inetAddrs.size(); j++)
    {
      if (addr==interfaces[i].inetAddrs[j])
        return i;
    }
  }

  return -1;
}

int RoutingTable6::nodeId() const
{
  int selfNodeId = -1;
  int index = 0;

  cModule* network = simulation.systemModule();
  for (cSubModIterator submod(*network); !submod.end(); submod++, index++)
  {
    selfNodeId = index;
    if (OPP_Global::findNetNodeModule(this) == submod())
      break;
  }

  return selfNodeId;
}

/// Makes it easier to see which node is what during debug
const char* RoutingTable6::nodeName() const
{
  return OPP_Global::nodeName(this);
}



/**
   @brief assign a possibly tentative addr
*/
bool RoutingTable6::assignAddress(const IPv6Address& addr, unsigned int if_idx)
{
  Dout(dc::debug|flush_cf, nodeName()<<" assigning address "<<addr);
  assert(if_idx < interfaces.size());
  if (if_idx >= interfaces.size())
    return false;

  Interface6Entry& ie = getInterfaceByIndex(if_idx);
  assert(!ie.addrAssigned(addr));
  if (ie.tentativeAddrAssigned(addr))
    ie.removeTentativeAddress(addr);
  ie.inetAddrs.push_back(addr);


  //Don't know why it asserts when default ctor of IPv6Address puts lifetime of
  //inifinity. Investigate later. Meanwhile will have to leave out
  //rescheduleAddrConfTimer call because of min lifetime of 0
  //assert(addr->storedLifetime() != 0);
  //if (addr->storedLifetime() > 0 && addr->storedLifetime() < VALID_LIFETIME_INFINITY)
  //  rescheduleAddrConfTimer(addr->storedLifetime());
  return true;
}

/**
   @brief Removes an assigned address
   @param addr object has be one that is bound at ifIndex
   @param ifIndex index of the interface

*/
void RoutingTable6::removeAddress(const IPv6Address& addr, unsigned int ifIndex)
{
  Dout(dc::debug|flush_cf, nodeName()<<":"<<ifIndex<<" "<<addr<<" removed");
  Interface6Entry& ie = getInterfaceByIndex(ifIndex);
  ie.removeAddress(addr);
}

/**
   @brief Remove assigned address
   @param addr address to remove
   @param ifIndex interface to remove from

   Uses the other version of removeAddress to do its dirty work
 */
void RoutingTable6::removeAddress(const ipv6_addr& addr, unsigned int ifIndex)
{
  removeAddress(IPv6Address(addr), ifIndex);
}

/**
   @brief Convenience function for checking if addr assigned
   @param addr check if address is assigned on ifIndex
   @param ifIndex index of interface to check if address is assigned

   @return true if addr is assigned at ifIndex false otherwise

 */
bool RoutingTable6::addrAssigned(const ipv6_addr& addr, unsigned int ifIndex) const
{
  const Interface6Entry& ie = getInterfaceByIndex(ifIndex);
  return ie.addrAssigned(addr);
}

///Elapse all valid/preferredLifetimes of assigned addresses on all
///ifaces.
void RoutingTable6::elapseLifetimes(unsigned int seconds)
{
  for (size_t i = 0; i < interfaceCount(); i++)
  {
    getInterfaceByIndex(i).elapseLifetimes(seconds);
  }
  //InterfaceEntries6::iterator start = interfaces.begin();
  //start++;
  //for_each(start, interfaces.end(),bind2nd(mem_fun_ref(&Interface6Entry::elapseLifetimes), seconds));
}

///Remove addresss that have stored lifetime of 0
void RoutingTable6::invalidateAddresses()
{
  bool addrRemovedFromIface = false;
  for (size_t ifIndex = 0; ifIndex < interfaceCount(); ifIndex++)
  {
    Interface6Entry& ie = getInterfaceByIndex(ifIndex);
    for (size_t addrIndex = 1; addrIndex < ie.inetAddrs.size(); addrIndex++)
    {
      if (ie.inetAddrs[addrIndex].storedLifetime() == 0)
      {
        Dout(dc::address_timer|flush_cf|dc::ipv6, nodeName()<<":"<<ifIndex<<" "
             <<ie.inetAddrs[addrIndex]<<" removed");

        removeAddress(ie.inetAddrs[addrIndex], ifIndex);
        addrRemovedFromIface = true;
      }
    }
    if (addrRemovedFromIface)
    {
      addrRemovedFromIface = false;
    }
  }
}

unsigned int RoutingTable6::minValidLifetime()
{
  unsigned int minLifetime = VALID_LIFETIME_INFINITY, lifetime = 0;

  for (size_t i = 0; i < interfaceCount(); i++)
  {
    lifetime = getInterfaceByIndex(i).minValidLifetime();

    assert(lifetime != 0);

    minLifetime = lifetime < minLifetime?lifetime:minLifetime;
  }
  assert(minLifetime != 0);

  return minLifetime;
}

/**
   Lifetime expiry callback
   Forgot about efficiency for now.  Just iterate through all addresses and
   remove ones that are close to expiry time
*/
void RoutingTable6::lifetimeExpired()
{
  assert(!addrExpiryTmr->isScheduled());

  Dout(dc::address_timer|flush_cf, nodeName()
       <<" An address is about to expire at "<<simTime());

  //Beware of truncation errors that will cause an address not to be expired
  elapseLifetimes(static_cast<unsigned int> (addrExpiryTmr->elapsedTime()));

  invalidateAddresses();

  unsigned int minStoredLifetime = minValidLifetime();
  assert(minStoredLifetime > 0);
  addrExpiryTmr->reschedule(simTime() + minStoredLifetime);
}


using IPv6NeighbourDiscovery::AddrExpiryTmr;

void RoutingTable6::rescheduleAddrConfTimer(unsigned int minUpdatedLifetime)
{
  OPP_Global::ContextSwitcher switcher(this);

  if (addrExpiryTmr == 0)
  {
    minUpdatedLifetime = minValidLifetime();
    if (minUpdatedLifetime == VALID_LIFETIME_INFINITY)
      return;

    addrExpiryTmr = new AddrExpiryTmr(this);

    Dout(dc::address_timer, nodeName()<<" Initial Shortest lifetime is "
         <<minUpdatedLifetime);
    scheduleAt(simTime() + minUpdatedLifetime, addrExpiryTmr);

    return;
  }

  //Elapse lifetimes of current entries leaving new entries alone (occurs in InterfaceEntry::elapseLifetimes)
  if (minUpdatedLifetime < addrExpiryTmr->remainingTime())
  {
    Dout(dc::address_timer|flush_cf, nodeName()<<" elapsedTime="<<addrExpiryTmr->elapsedTime()

         <<" rescheduling as minUpdatedLifetime="<<minUpdatedLifetime<<" < "<<"remaningTime="
         <<addrExpiryTmr->remainingTime());

    elapseLifetimes(static_cast<unsigned int>(addrExpiryTmr->elapsedTime()));
    assert(addrExpiryTmr->isScheduled());
    addrExpiryTmr->cancel();
    addrExpiryTmr->reschedule(simTime() + minUpdatedLifetime);
    return;
  }

  //This case happens when routers continually update address prefixes so that
  //the timer must obviously be reset.
  unsigned int minValid = minValidLifetime();
  if (minValid > addrExpiryTmr->remainingTime())
  {
    Dout(dc::address_timer|flush_cf, nodeName()<<" minValidLifetime="<<minValid<<"> remainingTime="
         << addrExpiryTmr->remainingTime()<<" Restarting timer");

    assert(addrExpiryTmr->isScheduled());
    addrExpiryTmr->cancel();
    addrExpiryTmr->reschedule(simTime() + minValid);

  }

}


//addrConfTimer(size_t ifIndex, IPv6Address* addr);




//Refer to MLD::MLDvw2ChangeReport
void RoutingTable6::joinMulticastGroup(const ipv6_addr& addr)
{
  //Reimplment using a set to make sure the address isn't inserted twice?
  //If it will allow storage of Omnet objects of course
  //Add value to destCache
  if (!addr.isMulticast())
  {
    cerr<<" Why is multicast test failing in opt build "<<addr<<endl;
  }
  assert(addr.isMulticast());
  multicastGroup.push_back(addr);

}

void RoutingTable6::leaveMulticastGroup(const ipv6_addr& addr)
{
  if (!addr.isMulticast())
  {
    cerr<<" Why is multicast test failing in opt build "<<addr<<endl;
  }
  assert(addr.isMulticast());
  for (MulticastAddresses::iterator it = multicastGroup.begin();
       it != multicastGroup.end(); it++)
    if (addr == *it)
    {
      multicastGroup.erase(it);
      break;
    }

  if ((addr & c_ipv6_addr(SOLICITED_NODE_PREFIX)) !=
      c_ipv6_addr(SOLICITED_NODE_PREFIX) &&
      c_ipv6_addr(ALL_NODES_NODE_ADDRESS) != addr &&
      c_ipv6_addr(ALL_NODES_LINK_ADDRESS) != addr &&
      c_ipv6_addr(ALL_NODES_SITE_ADDRESS) != addr)
  {

    if (!isRouter() ||
        (isRouter() && c_ipv6_addr(ALL_ROUTERS_NODE_ADDRESS) != addr &&
         c_ipv6_addr(ALL_ROUTERS_LINK_ADDRESS) != addr &&
         c_ipv6_addr(ALL_ROUTERS_SITE_ADDRESS) != addr))
    {
      //cout <<nodeName()<< " leave MUlticastgroup"<<endl;
      //assert(mld);
      //IPv6Address addrObj(addr);
      //mld->removeRtEntry(addr);
      //mld->sendDone(addr);
    }
  }
}

void  RoutingTable6::setForwardPackets(bool forward)
{
  IPForward = true;
}
// ------------- RoutingTable6 Tests -------------- //

#ifdef USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <iostream>

/**
   @class RoutingTableTest
   @brief Unit test for RoutingTable6
   @ingroup TestCases

   This differs from many of the other unit tests because it can only run when a
   certain test node is in existence.  Specifically the test node is only
   available in the Examples/TestNetwork configuration.
*/

class RoutingTableTest: public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( RoutingTableTest );
  CPPUNIT_TEST( testLookupAddress );
  CPPUNIT_TEST_SUITE_END();
public:
  RoutingTableTest(){};

  void setUp();
  void tearDown();
  void testLookupAddress();

private:

  RoutingTable6* rt;

  // Unused ctor and assignment op.
  RoutingTableTest(const RoutingTableTest&);
  RoutingTableTest& operator=(const RoutingTableTest&);
};

CPPUNIT_TEST_SUITE_REGISTRATION( RoutingTableTest );

void RoutingTableTest::setUp()
{
  rt = check_and_cast<RoutingTable6*>(simulation.moduleByPath("routingTableTest.networkLayer.proc.ipv6.routingTable6"));

  if (!rt)
    return;

  Interface6Entry ie1;
  ie1.iface_name = "eth800";
  rt->interfaces.push_back(ie1);

  Interface6Entry ie2;
  ie2.iface_name = "ppp200";
  rt->interfaces.push_back(ie2);

  PrefixEntry p1;
  p1.setPrefix("3271:2222:3312:4444:6433:0:0:0/40");
  rt->cds->insertPrefixEntry(p1, rt->interfaceNameToNo(ie1.iface_name.c_str()));

  PrefixEntry p2;
  p2.setPrefix("3271:2222:3312:4444:6433:0:0:0/50");
  rt->cds->insertPrefixEntry(p2, rt->interfaceNameToNo(ie2.iface_name.c_str()));

  Interface6Entry ie3;
  ie3.iface_name = "ppp201";
  rt->interfaces.push_back(ie3);

  PrefixEntry p3;
  p3.setPrefix("3271:2222:3312:4444:6433:3343:0:0/96");
  rt->cds->insertPrefixEntry(p3, rt->interfaceNameToNo(ie3.iface_name.c_str()));

  Interface6Entry ie4;
  ie4.iface_name = "eth101";
  rt->interfaces.push_back(ie4);

  PrefixEntry p4;
  p4.setPrefix("3271:1111:3312:4444:6433:3343:0:0/96");
  rt->cds->insertPrefixEntry(p4, rt->interfaceNameToNo(ie4.iface_name.c_str()));

  rt->numOfIfaces = rt->numOfIfaces + 4;

}

void RoutingTableTest::tearDown()
{
  rt = 0;
}

/**
 *
 * perform the longest prefix matching and retrieve the corresponding interface
 *
 */
void RoutingTableTest::testLookupAddress()
{
  if (!rt)
  {
    return;
  }

  size_t ifIndex = 0;

  CPPUNIT_ASSERT(rt->cds->lookupAddress(c_ipv6_addr("3271:2222:3312:4444:6433:3343:1234:0"), ifIndex));
  Interface6Entry ie = rt->getInterfaceByIndex(ifIndex);

  // result should be "ppp201" since ppp201 interface has longer prefix
  // length specified than ppp200 interface
  CPPUNIT_ASSERT(string("ppp201") == ie.iface_name);
}
#endif //USE_CPPUNIT
