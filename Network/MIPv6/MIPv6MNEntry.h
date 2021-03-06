// -*- C++ -*-
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
 * @file MIPv6MNEntry.h
 * @author Johnny Lai
 * @date 23 May 2002
 *
 * @brief Basic entries for Mobile Node
 *
 * Extracted from MIPv6CDSMobileNode.h
 *
 */

#ifndef MIPV6MNENTRY_H
#define MIPV6MNENTRY_H 1

#ifndef BOOST_WEAK_PTR_HPP
#include <boost/weak_ptr.hpp>
#endif //BOOST_WEAK_PTR_HPP

#ifndef LIST
#define LIST
#include <list>
#endif //LIST

#ifndef STRING
#define STRING
#include <string>
#endif

#ifndef IOSFWD
#define IOSFWD
#include <iosfwd>
#endif //IOSFWD

#ifndef IPV6_ADDRESS_H
#include "ipv6_addr.h"
#endif //IPV6_ADDRESS_H

#ifndef CCALLBACKMESSAGE_H
#include "cCallbackMessage.h"
#endif

#ifndef IPV6MOBILITY_H
#include "IPv6Mobility.h"
#endif // IPV6MOBILITY_H

#ifndef CSTDLIB
#define CSTDLIB
#include <cstdlib>
#endif

#include "MobilityHeader_m.h"

enum MIPv6HeaderType;

using std::rand;


namespace IPv6NeighbourDiscovery
{
  class PrefixEntry;
  class RouterEntry;
}

namespace
{
  //@name MN constants
  //@{
  extern const simtime_t INITIAL_BINDACK_TIMEOUT_FIRSTREG = 1.5;
  extern const int INITIAL_BINDACK_TIMEOUT = 1;
  extern const int INITIAL_SOLICIT_TIMER = 2;
  extern const unsigned int MAX_BINDACK_TIMEOUT = 32;
  ///No more than three BU every second
  extern const simtime_t MAX_UPDATE_RATE = 1/3;

  extern const int MAX_NONCE_LIFETIME = 240;
  extern const int MAX_TOKEN_LIFETIME = 210;

  extern const simtime_t SEND_DELAY_INCREMENT = 0.025; // ms
  //@}
}

namespace MobileIPv6
{

  typedef cCallbackMessage TIRetransTmr;
  class bu_entry;

/**
 * @class MIPv6RouterEntry
 *
 * @brief Associates the corresponding ND Router List's entry with MIPv6
 * specific information
 *
 * Thes emtries form the home agents list for mobile nodes @see
 * MIPv6CDSMobileNode::mrl which is of type MIPv6RouterList.
 *
 * Since most of the information maintained by default routers list in ND is of
 * use and duplicated by home agents list.  It is better to combine the two.
 * Also because global addr of routers are not stored in ND default router list.
 *
 * @todo 1. manage lifetime of global addr i.e. the corresponding prefix
 * specified in the prefix advertisement with R bit set.  2. Multiple global
 * addr
 *
 */
  class MIPv6RouterEntry
  {
  public:

    MIPv6RouterEntry(boost::weak_ptr<IPv6NeighbourDiscovery::RouterEntry> re,
                     bool HA, const ipv6_prefix& g_addr, unsigned int lifetime,
                     int pref = 0)
      :re(re), _isHA(HA), _prefix(g_addr), _preference(pref), _lifetime(0)
      {
        setLifetime(lifetime);
      }

    ///Return global address of router
    const ipv6_addr& addr() const { return prefix().prefix; }

    /**
     * Returns the on link global prefix for this router with its full global
     * address intact i.e. exactly the same info as a prefix information option
     * with router bit set.  Truncate the prefix if used as a prefix in forming
     * care of addr.  If there is more than one only the very first is recorded
     * since this is used for creating care of addr
     *
     * @todo If you want multiple care of addr when multiple prefixes are valid
     * for care of addr then will need to create a list of valid prefixes
     */

    const ipv6_prefix& prefix() const { return _prefix; }

    /**
     * Set the global prefix for potential use to form the coa
     *
     */

    void setPrefix(const ipv6_prefix& pref) { _prefix = pref; }

    ///@name Home Agent properties
    //@{

    ///Returns true if prefix also contains HA addr (we have no use for global
    ///addr of plain routers?) and RtrAdv had ha bit set
    bool isHomeAgent() const { return _isHA; }

    ///preference is valid for HA's only
    const int& preference() const { return _preference; }
    void setPreference(int pref) { _preference = pref; }

    ///Seperate lifetime for HA can differ from router's lifetime
    void setLifetime(unsigned int lifetime);
    unsigned int lifetime() const;
    //@}

    ///Add's this prefix if not linked with this router entry
    void addOnLinkPrefix(IPv6NeighbourDiscovery::PrefixEntry* pe);

    /**
     * This will create a router->prefixes relationship so we can track which
     * prefixes belong to which router and remove them accordingly
     *
     * Delete these onlink prefixes when default router changes as we move from
     * one subnet to another.  This was not required in IPv6 because we were
     * always on same link but now on link prefixes will change and old ones
     * should not be in prefix list anymore.
     */
    typedef std::list<ipv6_prefix> OnLinkPrefixList;
    typedef OnLinkPrefixList::iterator  OPLI;

    OnLinkPrefixList prefixes;


    boost::weak_ptr<IPv6NeighbourDiscovery::RouterEntry> re;

  private:
    bool _isHA;
    ipv6_prefix _prefix;
    // preference fo this home agent; higher values indicate a more
    // preferable home agent
    int _preference;
    unsigned int _lifetime;
  };


  /**
   * @class bu_entry
   * @brief Binding Update entry
   *
   */
  class bu_entry
  {
  public:

    bu_entry(const ipv6_addr& dest, const ipv6_addr& hoa, const ipv6_addr& coa,
             unsigned int olifetime, unsigned int seq, double lastTime, bool homeReg
#ifdef USE_HMIP
             , bool mapReg = false
#endif //USE_HMIP
	     );

    ~bu_entry();

    std::ostream& operator<<(std::ostream& os) const;

    /// IP address of the node to which a binding update was sent
    const ipv6_addr& addr() const
      {
        return dest_addr;
      }

    /**
     * @brief home address for which that binding update was sent
     *
     * Can be either home address Sec. 10.7/10.9 or the MN's previous care of
     * address Sec. 10.11
     *
     * Not really efficient as we store too many addresses especilly if they
     * only point to the primary home address
     */

    const ipv6_addr& homeAddr() const
      {
        //  if (hoaAgent.get() == 0)
        //  return IPv6_ADDR_UNSPECIFIED;
        return hoa;
      }

    const ipv6_addr& careOfAddr() const
      {
        //  if (coaRouter.get() == 0)
        //  return IPv6_ADDR_UNSPECIFIED;
        return coa;
      }

    void setCareOfAddr(const ipv6_addr& caddr)
      {
        coa = caddr;
      }

    bool homeReg() const { return _homeReg; }

    /**
     * @brief home address of binding.
     *
     * Usually points to the primaryHomAgent then the
     * homeAddress is the 'home' home address.

     * For bul entries of non-primary HA it points to the same HA from which the
     * previous care of addr is created thus it should only point to the
     * previous HA that we want forwarding to our current coa, as the care of
     * address is now a previous care of address w.r.t. the MN.  hoa is formed
     * from that HA's global prefix.  I think this can only point to HomeAgents
     * because routers cannot forward using the tunneling mechanism of HAs.
     *
     *
     */

    //boost::weak_ptr<MIPv6RouterEntry> hoaAgent;

    /**
     * @brief care of address of binding
     *
     * Points to the HA or router from which the 'latest' care of address is
     * formed at the time a binding update was sent to this dest_addr
     */

    //boost::weak_ptr<MIPv6RouterEntry> coaRouter;

  private:
    /// IP address of the node to which a binding update was sent
    ipv6_addr dest_addr;

    ipv6_addr hoa;
    ipv6_addr coa;

    /// initial lifetime sent in update
    u_int16 _lifetime;

    u_int16 _expires;

  public:
    void setSequence(unsigned int seq) { seq_no = seq; }
    unsigned int sequence() const { return seq_no; }

    ///@name BUL lifetime
    //@{

    /// bu entry lifetime countdown.  Removal from BindingUpdateList once this
    /// reaches zero
    u_int16 expires() const { return _expires; }
    void setExpires(unsigned int exp);

    /// initial lifetime sent in update
    u_int16 lifetime() const { return _lifetime; }
    void setLifetime(unsigned int life);
    //@}

    /**
     * @name Retransmission timer
     *
     * Can use something like NDTimer timer for this
     *
     *
     */
    //@{

    /// time at which a binding update was last sent to this destination
    double last_time_sent;

    /**
     * @brief the state of any retransmissions needed for this binding update if Ack
     * (A) bit was set in this binding update.
     *
     * This is taken care of by BURetranTimer in MIPv6MStateMobileNode thus the
     * number of BUs sent will be stored here.  0 means a BA was received, > 0
     * the no. of BUs sent to this node inc. retransmissions
     */
    unsigned int state;

    //@}

  private:

    ///maximum Sequence Number sent in previous binding update
    unsigned int seq_no;

    bool _homeReg;

  public:

    /**
     * future BU should not be sent here.  Set flag when MN receives an ICMP
     * Parameter Problem code 2 when a BU sent.  See Sec 10.17
     *
     */
    bool problem;

#ifdef USE_HMIP

    bool isMobilityAnchorPoint() const { return mapReg; }

  private:

    bool mapReg;
#endif //USE_HMIP

    ///@name RR procedure members
    //@{
  public:

    bool isPerformingRR(bool earlyBU = false);
    bool ebuWaitingOnHOT();
    bool tentativeBinding();

    TIRetransTmr* hotiRetransTmr;
    TIRetransTmr* cotiRetransTmr;

    u_int16 homeNI;
    u_int16 careOfNI;

    int homeCookie() const
    { return hoti_cookie; }
    int careOfCookie() const
    { return coti_cookie; }
    void setHomeCookie(int cook)
    { hoti_cookie = cook; }
    void setCareofCookie(int cook)
    { coti_cookie = cook; }

    void increaseHotiTimeout()
      {
        if ( hoti_timeout == 0 )
          hoti_timeout = INITIAL_BINDACK_TIMEOUT;
        else if ( hoti_timeout == INITIAL_BINDACK_TIMEOUT)
          hoti_timeout = hoti_timeout * 2;
        else if ( hoti_timeout != MAX_BINDACK_TIMEOUT)
          hoti_timeout = pow( hoti_timeout ,2 );
      }

    void increaseCotiTimeout()
      {
        if ( coti_timeout == 0 )
          coti_timeout = INITIAL_BINDACK_TIMEOUT;
        else if ( coti_timeout == INITIAL_BINDACK_TIMEOUT)
          coti_timeout = coti_timeout * 2;
        else if ( coti_timeout != MAX_BINDACK_TIMEOUT)
          coti_timeout = pow( coti_timeout ,2 );
      }

    simtime_t homeInitTimeout() const
    { return hoti_timeout; }
    simtime_t careOfInitTimeout() const
    { return coti_timeout; }

    void cancelTestInitTimeout(const MIPv6HeaderType& ht);

    bool testSuccess(bool earlyBU = false) const;

    cOutVector* regDelay;

    bool buReceived;
  private:
    int hoti_cookie;
    int coti_cookie;

    double hoti_timeout;
    double coti_timeout;

    //@}
  };

  std::ostream& operator<<(std::ostream& os, const MIPv6RouterEntry& re);

  std::ostream& operator<<(std::ostream& os, const MobileIPv6::bu_entry& bule);

} //namespace MobileIPv6


#endif /* MIPV6MNENTRY_H */
