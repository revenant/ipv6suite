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
 * @file   MIPv6CDSMobileNode.cc
 * @author Johnny Lai
 * @date   05 May 2002
 *
 * @brief  Implementation of MIPv6CDSMobileNode class
 *
 */


#include "sys.h"
#include "debug.h"

#include "MIPv6CDSMobileNode.h"

#include <algorithm> //find_if
#include <functional>

#include "MIPv6MNEntry.h"
#include "Interface6Entry.h"
#include "cTTimerMessageCB.h"
#include "MIPv6Timers.h"

namespace MobileIPv6
{

  MIPv6CDSMobileNode::MIPv6CDSMobileNode(size_t interfaceCount)
    :futureCoa(IPv6_ADDR_UNSPECIFIED), away(false), moved(false), _pcoaLifetime(5),
     eagerHO(false)
  {
  }

  MIPv6CDSMobileNode::~MIPv6CDSMobileNode()
  {}


  const ipv6_addr& MIPv6CDSMobileNode::homeAddr(boost::shared_ptr<MIPv6RouterEntry> ha) const
  {
    assert(ha.get()!=0);

    //If ha does not exist anymore i.e. lifetime expired then return our
    //real home addr anyway?
    if (ha == primaryHA())
      return homeAddr();

    //For a more efficient implementation we could form it instead of
    //storing it?

    bu_entry* bule = findBU(ha->addr());
    if (bule != 0)
      return bule->homeAddr();

    ///Perhaps we should always
    return homeAddr();
  }

  /*
   * @brief returns the latest care of address even if it has not been registered yet.
   *
   * @arg old true by default to maintain compability will return coa registered with pHA
   *
   * if arg is true and a newer coa has been created but awaiting dad then that
   * coa is returned. This may or may not be registered with ha depending on
   * whether opt dad is done in which case there should not be a diffrence.
   * new coa is written to by setFutureCoa which MIPv6NDStateHost::processRtrAdv does
   *
   * @todo this is any ugly hack but should suffice for now
   */
  const ipv6_addr& MIPv6CDSMobileNode::careOfAddr(bool old) const
  {
    if (primaryHA())
    {
      bu_entry* bule = findBU(primaryHA()->addr());
      if (futureCoa != IPv6_ADDR_UNSPECIFIED)
      {
        if (bule != 0)
          assert(bule->careOfAddr() != futureCoa);
        if (!old)
        {
          Dout(dc::debug|flush_cf,
               "MIPv6CDSMobileNode::careOfAddr returning futurecoa "<<futureCoa);
          return futureCoa;
        }
      }
      if (bule != 0)
      {
#if defined CWDEBUG
        Dout(dc::debug|flush_cf,
             "MIPv6CDSMobileNode::careOfAddr returning bule->coa "
             <<bule->careOfAddr());
#endif //CWDEBUG
        return bule->careOfAddr();
      }
      //Return primary hoa when no binding update sent to HA (we are at
      //home then?)
      return homeAddr();
    }

    assert(false);

    return IPv6_ADDR_UNSPECIFIED;
  }

  ipv6_addr MIPv6CDSMobileNode::formCareOfAddress(
    boost::weak_ptr<MIPv6RouterEntry> re, Interface6Entry* ie) const
  {
    assert(re.lock().get() != 0);
    const ipv6_prefix& pref = re.lock().get()->prefix();
    assert(pref.length <= EUI64_LENGTH);
    IPv6Address coaObj(pref);
    coaObj.truncate();
    ipv6_addr coa = coaObj;
    assert(ie->interfaceIDLength() == EUI64_LENGTH);
    coa.normal = ie->interfaceID()[0];
    coa.low = ie->interfaceID()[1];
    return coa;
  }

  /**
   * @warning make sure that this is indeed a new ha before inserting otherwise
   * there will be two HA objects pointing to the one logical HA.
   *
   * @note forget about the name it really means any mip6 router
   */

  void MIPv6CDSMobileNode::insertHomeAgent(MIPv6RouterEntry* ha)
  {
    //Can do assert check here to ensure that it's not inserted previously
    assert(findRouter(ha->addr()).get() == 0);
    mrl.push_back(boost::shared_ptr<MIPv6RouterEntry>(ha));
  }
} //namespace MobileIPv6

namespace
{
  using MobileIPv6::MIPv6RouterEntry;

  struct findAgentByLocalAddr:
    public std::binary_function<ipv6_addr, boost::shared_ptr<MIPv6RouterEntry>, bool >
  {
    bool operator()(const ipv6_addr& ll_addr,
                    const boost::shared_ptr<MIPv6RouterEntry>& bre) const
      {
        return bre->re.lock().get()->addr() == ll_addr;
      }
  };

} //namespace

namespace MobileIPv6
{

  boost::shared_ptr<MIPv6RouterEntry>
  MIPv6CDSMobileNode::findRouter(const ipv6_addr& ll_addr)
  {
    MRLI it = find_if(mrl.begin(), mrl.end(),
                      std::bind1st(findAgentByLocalAddr(), ll_addr));
    if (it != mrl.end())
      return *it;
     return boost::shared_ptr<MIPv6RouterEntry>();
  }

  bool MIPv6CDSMobileNode::findRouter(const ipv6_addr& ll_addr, MRLI& it)
  {
    it = find_if(mrl.begin(), mrl.end(),
                      std::bind1st(findAgentByLocalAddr(), ll_addr));
    if (it != mrl.end())
      return true;
    return false;
  }

  /**
   * @brief predicate using ipv6_prefix.prefix so the prefix length doesn't matter
   *
   * @note forget about the name it really means any mip6 router in mrl
   */

  struct findAgentByGlobalAddr:
    public std::binary_function<ipv6_prefix, boost::shared_ptr<MIPv6RouterEntry>, bool >
  {
    bool operator()(const ipv6_prefix& g_addr,
                    const boost::shared_ptr<MIPv6RouterEntry>& bre) const
      {
        Dout(dc::debug|flush_cf, __FUNCTION__<<" prefix="<<g_addr<<" entry is"
             <<*bre.get());
        return bre->prefix().prefix == g_addr.prefix;

         // && bre->isHomeAgent();
      }
  };

  /*
    @note forget about the name it really means any mip6 router in mrl
   */
  bool MIPv6CDSMobileNode::removeHomeAgent(boost::weak_ptr<MIPv6RouterEntry> ha)
  {
    MRLI it = find_if(mrl.begin(), mrl.end(),
                      std::bind1st(findAgentByGlobalAddr(), ha.lock().get()->prefix()));
    if (it != mrl.end())
    {
      mrl.erase(it);
      return true;
    }
    return false;
  }

  /*
    @note forget about the name it really means any mip6 router in mrl
  */
  boost::shared_ptr<MIPv6RouterEntry>
  MIPv6CDSMobileNode::findHomeAgent(const ipv6_addr& g_addr)
  {
    MRLI it = find_if(mrl.begin(), mrl.end(),
                      std::bind1st(findAgentByGlobalAddr(),
                                   ipv6_prefix(g_addr, EUI64_LENGTH)));
    if (it != mrl.end())
      return *it;
    return boost::shared_ptr<MIPv6RouterEntry>();
  }


const ipv6_prefix&  MIPv6CDSMobileNode::homePrefix() const
{
  return _homeAddr;
}

  /**
   * Please check if an entry with the same destination does not exist already
   * before inserting a new one.
   *
   */

  void MIPv6CDSMobileNode::addBU(bu_entry* bu)
  {
    assert(findBU(bu->addr()) == 0);
    bul.push_back(boost::shared_ptr<bu_entry>(bu));
  }

  struct findBUByDestAddr:
    public std::binary_function<ipv6_addr, boost::weak_ptr<bu_entry>, bool>
  {
    bool operator()(const ipv6_addr& dest,
                    const boost::weak_ptr<bu_entry>& bule) const
      {
        return bule.lock().get()->addr() == dest;
      }
  };

  /**
   * To be really complete we should accept the home addr as well as the
   * destAddr since different homeAddr are treated as different binding updates.
   * But that is only if we have multiple home subnet home prefixes (which we
   * won't have for now)
   */

  bu_entry* MIPv6CDSMobileNode::findBU(const ipv6_addr& destAddr) const
  {
    //Its a bit of a hack making bul mutable but don't want to remove constness
    //to every other function that uses this.  Don't know why STL find_if
    //doesn't accept a const iterators perhaps bind1st can't deduce it and have
    //to declare binder1st with explicit const iterators.
    const BULI it =
      find_if(bul.begin(), bul.end(), std::bind1st(findBUByDestAddr(), destAddr));

    if (it != bul.end())
       return (*it).get();

    return boost::weak_ptr<bu_entry>().lock().get();
  }

  bool MIPv6CDSMobileNode::removeBU(const ipv6_addr& addr)
  {
    return false;
  }

  void MIPv6CDSMobileNode::setAwayFromHome(bool notAtHome)
  { away = notAtHome; }

  TFunctorBaseA<cTimerMessage>* MIPv6CDSMobileNode::setupLifetimeManagement()
  {
    return makeCallback(this, &MIPv6CDSMobileNode::expireLifetimes);
  }

  void MIPv6CDSMobileNode::expireLifetimes(cTimerMessage* tmr)
  {
    unsigned int dec = static_cast<MIPv6PeriodicCB*> (tmr)->interval;
    MIPv6CDS::expireLifetimes(tmr);

    //do for bul entries
    for (BULI it = bul.begin(); it != bul.end();  )
    {
      if ((*it)->expires() > dec)
      {
        (*it)->setExpires((*it)->expires() - dec);
        it++;
      }
      else
      {
        it = bul.erase(it);
      }
    }
    for (MRLI it = mrl.begin(); it != mrl.end();  )
    {
      if ((*it)->lifetime() > dec)
      {
        (*it)->setLifetime((*it)->lifetime() - dec);
        it++;
      }
      else
      {
        it = mrl.erase(it);
      }
    }
  }

  void MIPv6CDSMobileNode::setFutureCoa(const ipv6_addr& ncoa)
  {
    cerr<<"futureCoa is" <<futureCoa<<endl;
    assert((ncoa == IPv6_ADDR_UNSPECIFIED && futureCoa != IPv6_ADDR_UNSPECIFIED) ||
           (ncoa != IPv6_ADDR_UNSPECIFIED && futureCoa == IPv6_ADDR_UNSPECIFIED));
    futureCoa = ncoa;
  }

} //namespace MobileIPv6

