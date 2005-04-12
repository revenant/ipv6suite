//
// Copyright (C) 2001, 2003, 2004, 2005 CTIE, Monash University
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
/**
    @file NDEntry.cc
    @brief Neighbour Discovery Entries - Prefix, Neighbour and Router
    @see RFC2461 Sec 5
    @author Johnny Lai
    @date 19.9.01

*/

//These two headers have to come first if libcwd macros are in use
#include "sys.h"
#include "debug.h"

#include <iostream>
#include <boost/cast.hpp>

#include "NDEntry.h"
#include "ICMPv6NDMessage.h"
#include "NDTimers.h"
#include "IPv6CDS.h"


const unsigned int VALID_LIFETIME_INFINITY = 0xffffffff;
const unsigned int VALID_LIFETIME_DEFAULT = 2592000;  //30 days
const unsigned int VALID_PREFLIFETIME_DEFAULT = 605800; //7 days

namespace IPv6NeighbourDiscovery
{

bool operator<(const PrefixEntry& lhs, const PrefixEntry& rhs)
{
  int lhs_pref_len = lhs._prefix.prefixLength();
  int rhs_pref_len = rhs._prefix.prefixLength();

  if(lhs_pref_len != rhs_pref_len)
    return (lhs_pref_len < rhs_pref_len)? false : true;

  return ((ipv6_addr)lhs._prefix < (ipv6_addr)rhs._prefix);
}

std::ostream& operator<<(std::ostream& os, const PrefixEntry& dat)
{
  os << "Prefix: "<<dat.prefix()<<" onLink="
     <<(dat.advOnLink()?"true":"false")<< " valid="<<dec<<dat.advValidLifetime()
     <<" preferred="<<dec<<dat.advPrefLifetime()<< " auto="
     <<(dat.advAutoFlag()?"true":"false");
  return os;
}


PrefixEntry::PrefixEntry(const IPv6Address& addr, unsigned int lifetime)
  :_prefix(addr),
   _advValidLifetime(lifetime),
   _realtime(false),
   _advOnLink(true),
   _advPrefLifetime(VALID_PREFLIFETIME_DEFAULT),
   _advAutoFlag(true)
#ifdef USE_MOBILITY
   ,_advRtrAddr(false)
#endif
//   tmr(0)
{}

PrefixEntry::PrefixEntry(const ipv6_addr& pref, unsigned int prefixLength,
                         unsigned int lifetime)
  :_prefix(pref),
   _advValidLifetime(lifetime),
   _realtime(false),
   _advOnLink(true),
   _advPrefLifetime(VALID_PREFLIFETIME_DEFAULT),
   _advAutoFlag(true)
#ifdef USE_MOBILITY
  ,_advRtrAddr(false)
#endif
//   tmr(0)
{
  _prefix.setPrefixLength(prefixLength);
}

PrefixEntry::PrefixEntry(const IPv6NeighbourDiscovery::ICMPv6NDOptPrefix& prefOpt)
  :_prefix(prefOpt.prefix, prefOpt.prefixLen),
   _advValidLifetime(prefOpt.validLifetime),
   _realtime(false),
   _advOnLink(prefOpt.onLink),
   _advPrefLifetime(prefOpt.preferredLifetime),
   _advAutoFlag(prefOpt.autoConf)
#ifdef USE_MOBILITY
  ,_advRtrAddr(prefOpt.rtrAddr)
#endif
//   tmr(0)
{}

/**
 * @warning possible duplicate PrefixEntry and thus multiple deletions of timer
 * cause segfault at simulation exit.

 *
 */

PrefixEntry::~PrefixEntry()
{
/*
  if (tmr && tmr->isScheduled())
    tmr->cancel();
  delete tmr;
*/
}

/*
///Expiry func called to destroy the prefix entry when prefix valid lifetime
///expires
void PrefixEntry::prefixTimeout(PrefixExpiryTmr* otmr)
{

  assert(otmr == tmr);

#if defined TESTIPv6 && defined PREFIXTIMER
  cout<<tmr->msgOwner()->nodeName()<<" "<<*this<<" removed at "
      <<tmr->msgOwner()->simTime()<<endl;
#endif

  //this deletes everything i.e. prefixEntry->prefixExpiryTmr
  tmr->msgOwner()->cds->removePrefixEntry(*this);

}
*/
unsigned int PrefixEntry::advValidLifetime() const
{
//  if (_advValidLifetime == VALID_LIFETIME_INFINITY || tmr == 0)
    return _advValidLifetime;

//  return static_cast<unsigned int> (tmr->remainingTime());
}

///Will reschedule the invalidation tmr if necessary
void PrefixEntry::setAdvValidLifetime(unsigned int validLifetime)
{
  _advValidLifetime = validLifetime;
/*
  assert(tmr != 0);

  if (tmr->isScheduled())
    tmr->cancel();

  if (validLifetime != VALID_LIFETIME_INFINITY)
    tmr->reschedule(tmr->msgOwner()->simTime() + validLifetime);
*/
}


NeighbourEntry::NeighbourEntry(const ipv6_addr& addr, size_t ifaceNo,
                               const char* destLinkAddr,
                               ReachabilityState state)
  :unicastAddr(addr), _LLaddress(destLinkAddr?destLinkAddr:""),
   interfaceNo(ifaceNo),  _isRouter(false), _state(state)
{}

///NS can never signify if the neighbour is a router so leave it as host
NeighbourEntry::NeighbourEntry(const ICMPv6NDMNgbrSol* ns)
  :unicastAddr(static_cast<IPv6Datagram*> (ns->encapsulatedMsg())->srcAddress()),
  _LLaddress(ns->srcLLAddr()),
   interfaceNo(static_cast<IPv6Datagram*> (ns->encapsulatedMsg())->inputPort()),
  _isRouter(false), _state(STALE)
{}

NeighbourEntry::NeighbourEntry(const ICMPv6NDMRedirect* redirect)
  :unicastAddr(redirect->targetAddr()), _LLaddress(redirect->targetLLAddr()),
 interfaceNo(static_cast<IPv6Datagram*> (redirect->encapsulatedMsg())->inputPort()),
 _isRouter(false), _state(redirect->targetLLAddr() != ""?STALE:INCOMPLETE)
{}

///RFC 2461 Sec. 7.2.5
///Returns whether pending packets should be sent
bool NeighbourEntry::update(const ICMPv6NDMNgbrAd* na)
{
  bool sendPending = false;

  if (state() == INCOMPLETE && na->targetLLAddr() == "")
  {
    Dout(dc::addr_resln, "Ignoring NA as no LL address option included "<<*na);
    return false;
  }

  if (state() == INCOMPLETE)
  {
    setLLAddr(na->targetLLAddr());

    if (na->solicited())
      setState(REACHABLE);
    else
      setState(STALE);

    sendPending = true;

    //Set the Router flag
  }
  else // state() != INCOMPLETE
  {
    if (!na->override() && na->targetLLAddr() != "" &&
        na->targetLLAddr() != linkLayerAddr())
    {
      Dout(dc::addr_resln, " not updating NE any further as link layer address in"
           <<" NA does not match cached version "<<*na);
      if (state() == REACHABLE)
        setState(STALE); //Don't update the LLaddr
      else
        return false; //Ignore totally
      return false;
    }
    else if (na->override() ||
             (!na->override() && na->targetLLAddr() == linkLayerAddr()) ||
             na->targetLLAddr() == "")
    {
      bool updatedAddr = false;
      if (na->targetLLAddr() != "" && na->targetLLAddr() != linkLayerAddr())
      {
        updatedAddr = true;
        setLLAddr(na->targetLLAddr());
      }

      if (na->solicited())
        setState(REACHABLE);
      else if (updatedAddr)
        setState(STALE);

      //Check the isRouter flags like the case at the very beginning again
    }
  }

  if (na->isRouter() && na->isRouter() != isRouter())
  {
    Dout(dc::notice, "Should really convert this whole object into a RouterEntry instead "<<*na);
    setIsRouter(true);
  }
  else if (!na->isRouter() && na->isRouter() != isRouter())
  {
    Dout(dc::notice, "Do the reverse RouterEntry->NeighbourEntry "<<*na);
    setIsRouter(false);
    //remove from DRL and remove reference to this from all DE
  }
  //Perhaps this ability can be implemented by containing a pointer to a
  //RouterEntry so all the pointers remain valid regardless of how often their
  //roles change. RouterEntry contains a pointer to NE.  DRL contains the RE
  //pointer whilst the and DE contains RE, Sounds like good idea. TODO

  return sendPending;
}

void NeighbourEntry::update(const ICMPv6NDMNgbrSol* ns)
{
  if (linkLayerAddr() != ns->srcLLAddr())
  {
    setLLAddr( ns->srcLLAddr());
    setState(NeighbourEntry::STALE);
  }
}

void NeighbourEntry::update(const ICMPv6NDMRedirect* redirect)
{
  if (redirect->targetLLAddr() != "" &&
      linkLayerAddr() != redirect->targetLLAddr())
  {
    setLLAddr(redirect->targetLLAddr());
    setState(NeighbourEntry::STALE);
  }
}

std::ostream& operator<<(std::ostream& os, const NeighbourEntry& ne)
{
  os<<ne.addr()<<" ifIndex="<<hex<<ne.ifIndex()<<dec<<" isRouter="
    <<(ne.isRouter()?"true":"false")<<" NUD="<<ne.state()<<" lladdr="
    <<ne.linkLayerAddr();
  return os;
}

std::ostream& operator<<(std::ostream& os, const RouterEntry& re)
{
  return os<<(const NeighbourEntry&)re<<" lifetime="<<re.invalidTime();
}

///RouterEntries are always created with STALE in NC acc. to 6.3.4 Disc
RouterEntry::RouterEntry(const ipv6_addr& addr, size_t ifaceNo,
                         const char* linkAddr, unsigned int lifetime)
  :NeighbourEntry(addr, ifaceNo, linkAddr, STALE), lastRAReceived(0),
   _invalidTime(lifetime)
{
  setIsRouter(true);
}

RouterEntry::RouterEntry(const ICMPv6NDMRedirect* redirect)
  :NeighbourEntry(redirect),
   //Give it this for now since spec says nothing about this
  _invalidTime(VALID_LIFETIME_INFINITY)//, tmr(0)
{
  setIsRouter(true);
}

RouterEntry::~RouterEntry()
{
/*
  if (tmr && tmr->isScheduled())
    tmr->cancel();
  delete tmr;
*/
}

unsigned int RouterEntry::invalidTime() const
{
//  if (_invalidTime == VALID_LIFETIME_INFINITY || tmr == 0)
    return _invalidTime;

  //return _invalidTime;
//  assert(tmr->remainingTime() > 0);
//

//  return static_cast<unsigned int> (tmr->remainingTime());
}
/*
///Expiry func called to rmove the router entry when router lifetime expires
void RouterEntry::routerExpired(RouterExpiryTmr* otmr)
{
  assert(otmr == tmr);

#if defined TESTIPv6 && defined ROUTERTIMER
  cout<<tmr->msgOwner()->nodeName()<<" "<<*this<<" removed at "
      <<tmr->msgOwner()->simTime()<<endl;
#endif

  //this deletes everything i.e. routerEntry->routerExpiryTmr
  tmr->msgOwner()->cds->removeRouterEntry(addr());
}
*/
///Reschedules the invalidation time if necessary
void RouterEntry::setInvalidTmr(unsigned int newLifetime)
{
  Dout(dc::router_timer, simulation.simTime()<<" "<<*this<<" lifetime set to "<<newLifetime);
  _invalidTime = newLifetime;
/*
  assert(tmr != 0);
  if (tmr == 0)
    return;

  if (tmr->isScheduled())
    tmr->cancel();

  if (newLifetime != VALID_LIFETIME_INFINITY)
    tmr->reschedule(tmr->msgOwner()->simTime() + newLifetime);
*/
}

std::ostream& operator<<(std::ostream& os, const DestinationEntry& de)
{
    os <<"ngbr: ";
    if (de.neighbour.lock().get())
      os <<*(de.neighbour.lock().get());
    else
      os <<" none";
    return os;
}

}// end  namespace IPv6NeighbourDiscovery
