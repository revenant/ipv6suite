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
;
  }

  //TODO
  bool bu_entry::isPerformingRR()
  {
    if (cotiRetransTmr)
    {
      return cotiRetransTmr->isScheduled();
    }
    else
      return false;
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
     regDelay(0),
     hoti_cookie(0), coti_cookie(0),
     hoti_timeout(0), coti_timeout(0)
/*    , isPerformingRR(false), 
     last_hoti_sent(0), last_coti_sent(0)
     careof_token(UNSPECIFIED_BIT_64), home_token(UNSPECIFIED_BIT_64),
     // cell residency related information
     _dirSignalCount(0), _successDirSignalCount(0), hotSuccess(false), 
     cotSuccess(false), _hotiSendDelayTimer(0), _cotiSendDelayTimer(0),
*/
  {
    setExpires(lifetime());
  }


    bool bu_entry::testSuccess() const
    {
      return !hotiRetransTmr->isScheduled() && !cotiRetransTmr->isScheduled();
    }

    void bu_entry::resetTestInitTimeout(const MIPv6HeaderType& ht)
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
