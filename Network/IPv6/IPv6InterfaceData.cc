//
// Copyright (C) 2001, 2003, 2004 CTIE, Monash University
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
 *  @file IPv6InterfaceData.cc
 *
 *  Implementation of interface class
 *
 *  @author  Johnny Lai
 *
 *  @date    02/03/2002
 *
 */


#include "sys.h"
#include "debug.h"
#include "config.h"

#include "IPv6InterfaceData.h"
#include "IPv6Headers.h"
#include <string>
#include "opp_akaroa.h"

using namespace IPv6NeighbourDiscovery;

/*
 * Print on the console (first choice) or in the OMNeT window
 * (second choice).
 */
#define PRINTF  printf
//#define PRINTF  ev.printf

const double DEFAULT_MAX_RTR_ADV_INT = 600;

//TODO need to find the real value from RFC ASSIGNED NUMBERS
///Diameter of the Internet at time of implementation
const int DEFAULT_ADVCURHOPLIMIT = 30;

const int DEFAULT_DUPADDRDETECTTRANSMITS = 1; //Send NS once only
const double MAX_RTR_SOLICITATION_DELAY = 1; // seconds
const int RETRANS_TIMER = 1000; //ms
const int REACHABLE_TIME = 30000; // ms

const double MIN_RANDOM_FACTOR = 0.5;
const double MAX_RANDOM_FACTOR = 1.5;

#ifdef USE_MOBILITY
const double MIN_MIPV6_MAX_RTR_ADV_INT = 0.07;
const double MIN_MIPV6_MIN_RTR_ADV_INT = 0.03;
#endif

// --------------------------------------------------
//  IPv6InterfaceData functions
// --------------------------------------------------

IPv6InterfaceData::IPv6InterfaceData() :
   //loopback(false),
   curHopLimit(DEFAULT_ADVCURHOPLIMIT),
   retransTimer(RETRANS_TIMER),
   baseReachableTime(REACHABLE_TIME),
#if FASTRS
   maxRtrSolDelay(MAX_RTR_SOLICITATION_DELAY),
#endif // FASTRS
   dupAddrDetectTrans(DEFAULT_DUPADDRDETECTTRANSMITS),
   _prevBaseReachableTime(0)
{
  // set rechableTime
  reachableTime();

  _interfaceID[0] = 0;
  _interfaceID[1] = 0;
}

std::string IPv6InterfaceData::info() const
{
  std::ostringstream os;
  os << "IPv6:{";
  if (inetAddrs.size() == 0)
  {
    os << "addrs:none";
  }
  else
  {
    os << "addrs:";
    for(unsigned int i=0; i<inetAddrs.size(); i++)
      os << (i?",":"") << inetAddrs[i].address().c_str() << inetAddrs[i].scope_str();
  }
  if (tentativeAddrs.size() == 0)
  {
    os << " tentativeAddrs:none";
  }
  else
  {
    os << " tentativeaddrs:";
    for(unsigned int i=0; i<inetAddrs.size(); i++)
      os << (i?",":"") << inetAddrs[i].address().c_str() << inetAddrs[i].scope_str();
  }
  os << "}";
  return os.str();
}

std::string IPv6InterfaceData::detailedInfo() const
{
  return info(); // XXX this could be improved: multi-line text, etc
}

void IPv6InterfaceData::removeAddrFromArray(IPv6Addresses& addrs, const IPv6Address& addr)
{
  //Weak test to see if addr indeed belongs in this interface.  Should compare
  //the actual pointers
  IPv6Addresses::iterator it = std::find(addrs.begin(),addrs.end(),addr);
  // assert (it!=addrs.end());
  if (it != addrs.end())
  {
    *it = addrs.back();
    addrs.pop_back();
  }
}

bool IPv6InterfaceData::addrAssigned(const ipv6_addr& addr) const
{
  IPv6Addresses::const_iterator it = std::find(inetAddrs.begin(),inetAddrs.end(),addr);
  return it!=inetAddrs.end();
}

bool IPv6InterfaceData::tentativeAddrAssigned(const ipv6_addr& addr) const
{
  IPv6Addresses::const_iterator it = std::find(tentativeAddrs.begin(),tentativeAddrs.end(),addr);
  return it!=tentativeAddrs.end();
}

/* XXX out of order
const char* IPv6InterfaceData::encap(void)
{
  if(iface_name.find("ppp")!=string::npos)
    return "Point-to-Point Protocol";
  else if(iface_name.find("eth")!=string::npos)
    return "Ethernet";
  else if(iface_name.find("wlan")!=string::npos)
    return "Wireless Ethernet";
  else if(iface_name.find("lo")!=string::npos)
    return "Local Loopback";
  else
    return "";
}
*/

double IPv6InterfaceData::reachableTime(void)
{
  if(_prevBaseReachableTime == baseReachableTime)
    return _reachableTime;

  _reachableTime = OPP_UNIFORM(baseReachableTime*MIN_RANDOM_FACTOR,
                           baseReachableTime*MAX_RANDOM_FACTOR);

  _prevBaseReachableTime = baseReachableTime;

  return _reachableTime;
}

/**
   Elapse all valid/preferredLifetimes of assigned addresses. Addrs with
   infinite lifetime are left alone.

   @todo remove IPv6Address::updated and IPv6Address::updatedLifetime
   if rescheduleAddrConfTimer correct
*/
void IPv6InterfaceData::elapseLifetimes(unsigned int seconds)
{
  //start from one as we don't do link local addr timeout
  for (size_t i = 1; i < inetAddrs.size(); i++)
  {
    //Assuming only 1 link local address (not useful to have two except logical
    //subnet on local link )
    if (inetAddrs[i].storedLifetime() == VALID_LIFETIME_INFINITY)
      continue;

    //reset updated flag and ignore these newly added entries
    if (inetAddrs[i].updated())
    {
      inetAddrs[i].updatedLifetime();
      continue;
    }


    //Cater for truncation of seconds during conversion from double to unsigned int and
    //as a consequence rollover for (storedLifetime which is an unsigned int)
    if (inetAddrs[i].storedLifetime() < seconds)
    {
      Dout(dc::warning|flush_cf, FILE_LINE<<" for address "
           <<inetAddrs[i]<<" i="<<i<<" stored="<<inetAddrs[i].storedLifetime()
           <<" but elapsed="<<seconds<<" rescheduling of addr timer wrong?");
      cerr << "Error "<<__FILE__<<":"<<__LINE__<<" for address "
           <<inetAddrs[i]<<" i="<<i<<endl;
      //Perhaps just set to 0 lifetime to invalidate address
      inetAddrs[i].setStoredLifetime(0);
    }
    else
      inetAddrs[i].setStoredLifetime(inetAddrs[i].storedLifetime() - seconds);

    if (inetAddrs[i].preferredLifetime() >= seconds)
      inetAddrs[i].setPreferredLifetime(0);
    else
      inetAddrs[i].setPreferredLifetime(inetAddrs[i].preferredLifetime() - seconds);

  }
}

///Returns VALID_LIFETIME_INFINITY when no addr conf addr are available or when
///those addrs have infinite lifetimes
unsigned int IPv6InterfaceData::minValidLifetime()
{
  unsigned int minLifetime = VALID_LIFETIME_INFINITY, lifetime = 0;

  //1 as link local addr does not expire
  for (size_t i = 1; i < inetAddrs.size(); i++)
  {
    lifetime = inetAddrs[i].storedLifetime();

    assert(lifetime > 0);

    minLifetime = lifetime <= minLifetime?lifetime:minLifetime;
  }
  return minLifetime;
}

/**
 * @param prefix is the actual prefix does not have to be truncated to prefix
 * length
 *
 * @param prefixLength is actually unused in this case as the internally
 * assigned addrs prefix length is the number of bits to match against
 *
 * @param tentative check in tentative addresses list too?
 *
 */

IPv6Address IPv6InterfaceData::matchPrefix(const ipv6_addr& prefix,
                                         unsigned int prefixLength, bool tentative)
{
  for (size_t i = 0; i < inetAddrs.size(); i++)
    if (inetAddrs[i].isNetwork(IPv6Address(prefix, prefixLength)))
      return inetAddrs[i];
  if (tentative)
    for (unsigned int i = 0; i < tentativeAddrs.size(); i++)
    {
      if (tentativeAddrs[i].isNetwork(IPv6Address(prefix,prefixLength)))
        return tentativeAddrs[i];
    }
  return IPv6Address(IPv6_ADDR_UNSPECIFIED);
}

/*XXX to be replaced, implemented etc
void IPv6InterfaceData::print(bool IPForward)
{
  / *
    e.g.
    eth0    Link encap:Ethernet  HWaddr 00:60:97:C3:11:97
    inet6 addr: fe80::260:97ff:fec3:1197/10 Scope:Link
    inet6 addr: 3ffe:8001:12:fc:260:97ff:fec3:1197/64 Scope:Global
    UP BROADCAST RUNNING MULTICAST  MTU:1500
  * /

  // print out general interface info
  PRINTF("\t Link encap: %s HWAddr: %s \n",
         iface_name.c_str(),
         encap(),
         llAddr.c_str());

  // print out each IPv6 address attached to the interace
  for(unsigned int i=0; i<inetAddrs.size(); i++)
  {
    PRINTF("\t inet6 addr: %s Scope:%s \n",
           inetAddrs[i].address().c_str(),
           inetAddrs[i].scope_str());
  }

  // Current information about the interface
  //PRINTF("\t UP %s RUNNING \n", loopback ? " LOOPBACK" : "");

  // Host Configuration Vairables
  PRINTF("==============  Host Configuration Variables  ============================== \n");
  PRINTF("LinkMTU:\t\t %d octets \n", mtu);
  PRINTF("CurHopLimit:\t\t %d hops \n", curHopLimit);
  PRINTF("BaseReachableTime:\t %d millisecond \n", baseReachableTime);
  PRINTF("ReachableTime:\t\t %f millisecond \n", reachableTime());
  PRINTF("RetransTimer:\t\t %d millisecond \n", retransTimer);
  PRINTF("DupAddrDetectTransmits:\t %d transmits \n", dupAddrDetectTrans);
#if FASTRS
  PRINTF("MaxRtrSolDelay:\t %e millisecond \n", maxRtrSolDelay * 1000);
#endif // FASTRS

#ifdef USE_MOBILITY
  PRINTF("==============  MIPv6 Host Variables  ====================================== \n");

  PRINTF("Deprecated: MinRtrSolInterval:\t\t %d seconds\n", mipv6Var.minRtrSolInterval);
  PRINTF("Deprecated: MaxInterval: \t\t %d seconds\n", mipv6Var.maxInterval);
  PRINTF("MaxConsecutiveMissedRtrAdv: \t\t %d misses\n",
         mipv6Var.maxConsecutiveMissedRtrAdv);

#endif //USE_MOBILITY

  // Router Configuration Vairables

  if(IPForward)
  {

    PRINTF("==============  Router Configuration Variables  ============================ \n");
    PRINTF("AdvSendAdvertisements: \t %d \n", rtrVar.advSendAds);
    PRINTF("MaxRtrAdvInterval: \t %f sec\n", rtrVar.maxRtrAdvInt);
    PRINTF("MinRtrAdvInterval: \t %f sec\n", rtrVar.minRtrAdvInt);
    PRINTF("AdvManagedFlag: \t %d \n", rtrVar.advManaged);
    PRINTF("AdvOtherConfigFlag: \t %d \n", rtrVar.advOther);
    PRINTF("AdvLinkMTU: \t \t %d Octets \n", rtrVar.advLinkMTU);
    PRINTF("AdvReachableTime: \t %d ms\n", rtrVar.advReachableTime);
    PRINTF("AdvRetransTimer: \t %d ms\n", rtrVar.advRetransTmr);
    PRINTF("AdvCurHopLimit: \t %d hops\n", rtrVar.advCurHopLimit);
    PRINTF("AdvDefaultLifetime: \t %e sec\n", rtrVar.advDefaultLifetime);
#ifdef USE_MOBILITY
    PRINTF("(MIPv6)AdvHomeAgent: \t %d \n", rtrVar.advHomeAgent);
#endif
    if (!rtrVar.advPrefixList.empty())
    {
      PRINTF("==============  Advertising Prefix List  ============================ \n");
      for(size_t i = 0; i < rtrVar.advPrefixList.size(); i++)
      {
        PRINTF("Prefix[%d]: %s ", i, rtrVar.advPrefixList[i].prefix().address().c_str());
        PRINTF("Scope: %s\n", rtrVar.advPrefixList[i].prefix().scope_str());
        PRINTF("AdvOnLinkFlag: %d\t | ", rtrVar.advPrefixList[i].advOnLink());
        PRINTF("AdvValidLifetime: %d sec\n", rtrVar.advPrefixList[i].advValidLifetime());
        PRINTF("AdvAutonomousFlag: %d\t | ", rtrVar.advPrefixList[i].advAutoFlag());
        PRINTF("AdvPreferredLifetime: %d sec\n", rtrVar.advPrefixList[i].advPrefLifetime());
#ifdef USE_MOBILITY
                PRINTF("(MIPv6)AdvRouterAddressFlag: %d \n", rtrVar.advPrefixList[i].advRtrAddr());
#endif
      }
    }
    PRINTF("============================================================================ \n");

  }
}
*/

