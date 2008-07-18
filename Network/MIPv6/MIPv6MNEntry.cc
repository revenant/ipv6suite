//
// Copyright (C) 2002, 2004 CTIE, Monash University
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
 * @file   MIPv6MNEntry.cc
 * @author Johnny Lai
 * @date   06 Sep 2002
 *
 * @brief  Implementation of MIPv6RouterEntry and bu_entry classes
 *
 */


#include "MIPv6MNEntry.h"

#include "NDEntry.h"
#include <algorithm>

namespace MobileIPv6
{
  std::ostream& operator<<(std::ostream& os, const MobileIPv6::MIPv6RouterEntry& re)
  {
    return os<<"gaddr="<<re.addr()<<" isHA="<<(re.isHomeAgent()?
                                               "HA":"not HA")
             <<" lifetime="<<re.lifetime()<<" pref="<<re.preference()<<" "
             <<*(re.re.lock());
  }

  std::ostream& operator<<(std::ostream& os, const MobileIPv6::bu_entry& bule)
  {
    return bule.operator<<(os);
  }

  void MIPv6RouterEntry::addOnLinkPrefix(IPv6NeighbourDiscovery::PrefixEntry* pe)
  {
    assert(pe != 0);
    const ipv6_prefix prefix(pe->prefix(), pe->prefix().prefixLength());
    if (std::find(prefixes.begin(), prefixes.end(), prefix) == prefixes.end())
      prefixes.push_back(prefix);
  }

  /**
   * Will overide the router's lifetime iff router lifetime is longer than ha
   * lifetime and ha lifetime is not 0.  Its not implemented like this now
   * however it could be if there is no point in having two different lifetimes.
   */

  void MIPv6RouterEntry::setLifetime(unsigned int lifetime)
  {
    _lifetime = lifetime;
  }

  unsigned int MIPv6RouterEntry::lifetime() const
  {
    return _lifetime;
  }

  std::ostream& bu_entry::operator<<(std::ostream& os) const
  {
    return os<<"addr="<<addr()<<" hoa="<<homeAddr()<<" coa="<<careOfAddr()
      <<" h="<<homeReg()<<" sequence="<<sequence()<<" exp="<<expires()
      <<" lifetime="<<lifetime()<<" problem="<<problem<<" state="<<state
      <<" last="<<last_time_sent
#ifdef USE_HMIP
      <<" m="<<isMobilityAnchorPoint()
#endif //USE_HMIP
	     <<" honi="<<homeNI<<" coni="<<careOfNI<<" ackrec="<<buReceived
;
  }



  bu_entry::bu_entry(const ipv6_addr& dest, const ipv6_addr& hoa, 
		     const ipv6_addr& coa, unsigned int olifetime, unsigned int seq,
		     double lastTime, bool homeRegn
#ifdef USE_HMIP
	   , bool mapReg
#endif //USE_HMIP
		     )

    :dest_addr(dest), hoa(hoa), coa(coa), _lifetime(olifetime),
     last_time_sent(lastTime), state(0), seq_no(seq), _homeReg(homeRegn),
     problem(false)
#ifdef USE_HMIP
    , mapReg(mapReg)
#endif //USE_HMIP
     ,hotiRetransTmr(0), cotiRetransTmr(0), homeNI(0), careOfNI(0),
     regDelay(0), buReceived(false),
     hoti_cookie(0), coti_cookie(0),
     hoti_timeout(0), coti_timeout(0)
  {
    setExpires(lifetime());
  }

  bu_entry::~bu_entry()
  {
    if (cotiRetransTmr)
    {
      if (cotiRetransTmr->isScheduled())
	cotiRetransTmr->cancel();
      delete cotiRetransTmr;
    }
    if (hotiRetransTmr)
    {
      if (hotiRetransTmr->isScheduled())
	hotiRetransTmr->cancel();
      delete hotiRetransTmr;
    }
  }


  void bu_entry::setLifetime(unsigned int life)
  {
    //    if (lifetime == MAX_TOKEN_LIFETIME)
    //  _lifetime = 0xffff;

    _lifetime = life;
    setExpires(_lifetime);
  }

  void bu_entry::setExpires(unsigned int exp)
  {
    _expires = exp;
  }

  //Returns true if HOTI has been transmitted and waiting for HOT in EBU case as
  //HOTI is always scheduled in EBU.
  bool bu_entry::ebuWaitingOnHOT()
  {
    return (hotiRetransTmr && hotiRetransTmr->isScheduled() && (hotiRetransTmr->remainingTime() <= homeInitTimeout()));
  }

  bool bu_entry::isPerformingRR(bool earlyBU)
  {
    if (!earlyBU)
    {
      if (cotiRetransTmr && hotiRetransTmr)
        return cotiRetransTmr->isScheduled() || hotiRetransTmr->isScheduled();
    }
    else
    {
      //cerr<<simulation.simTime()<<" homeNI="<<homeNI<<" remaining time="<<hotiRetransTmr->remainingTime()<<" timeout="<<homeInitTimeout() <<" cotiRetransTmr="<< cotiRetransTmr->isScheduled()<<endl;
      return (cotiRetransTmr && cotiRetransTmr->isScheduled()) || ebuWaitingOnHOT();
 
    }
    return false;	
  }

  bool bu_entry::tentativeBinding()
  {
    bool earlyBU = true;
    return isPerformingRR(earlyBU) && careOfNI == 0;
  }

    bool bu_entry::testSuccess(bool earlyBU) const
    {
      if (!earlyBU)
	return !hotiRetransTmr->isScheduled() && !cotiRetransTmr->isScheduled() &&
	  homeNI != 0 && careOfNI != 0;
      return hotiRetransTmr->isScheduled() && !cotiRetransTmr->isScheduled() && 
	careOfNI != 0 && homeNI != 0;
    }

    void bu_entry::cancelTestInitTimeout(const MIPv6HeaderType& ht)
      {
        if ( ht == MIPv6MHT_HOT )
	{
          hoti_timeout = 0;
	  hotiRetransTmr->cancel();
	}
        else if ( ht == MIPv6MHT_COT )
	{
          coti_timeout = 0;
	  cotiRetransTmr->cancel();
	}
        else
          assert(false);
      }

} //namespace MobileIPv6
