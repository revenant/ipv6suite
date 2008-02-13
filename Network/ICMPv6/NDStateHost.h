// -*- C++ -*-
//
// Copyright (C) 2001, 2003 CTIE, Monash University
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
   @file NDStateHost.h
   @brief Definition of NDStateHost class that implements Neighbour Discovery
   mehanisms defined in RFC2461 and AutofConf in RFC2462 for a host.

   @author Johnny Lai
   @date 24.9.01
*/

#if !defined NDSTATEHOST_H
#define NDSTATEHOST_H

#ifndef NDSTATES_H
#include "NDStates.h"
#endif //NDSTATES_H
#ifndef LIST
#define LIST
#include <list>
#endif //LIST
#ifndef VECTOR
#define VECTOR
#include <vector>
#endif //VECTOR
#ifndef MAP
#include <map>
#endif //MAP


class RoutingTable6;
class InterfaceTable;
class IPv6Datagram;
struct ipv6_addr;
class cTimerMessage;


namespace IPv6NeighbourDiscovery
{
  extern const short MAX_RTR_SOLICITATIONS;
  extern const double RTR_SOLICITATION_INTERVAL;
  extern const double MAX_RTR_SOLICITATION_DELAY;

  struct InterfaceStatus
  {
    InterfaceStatus()
      :manualLinkLocal(false), initStarted(false)
      {}
    ~InterfaceStatus()
      {}
    /// link local addr manually assigned - requires DAD for all addr conf
    bool manualLinkLocal;
    /// autoconf initialisation started for iface
    bool initStarted;

  };

  class NDTimer;

  /**
     @class NDStateHost

     @brief Implements functionality specific for a host
  */
  class NDStateHost: public NDState
  {
  public:

    NDStateHost(NeighbourDiscovery* mod);
    virtual ~NDStateHost();

    virtual std::auto_ptr<ICMPv6Message> processMessage(std::auto_ptr<ICMPv6Message> msg);

    //1st stage of autoconf
    void nodeInitialise();

    virtual void print(){};

  protected:
    virtual void enterState();
    virtual void leaveState();

    ///Second stage of interface initialisation (usually Addr Conf)
    virtual void initialiseInterface(size_t ifIndex);
    virtual void disableInterface(size_t ifIndex);

    void processNgbrAd(std::auto_ptr<NA> msg);
    void processNgbrSol(std::auto_ptr<NS> msg);

    ///Hosts simple checks Sec. 6.1.2
    virtual bool valRtrAd(RA* ad);

    ///Sec. 7.1.1
    static bool valNgbrSol(NS* sol);
    ///Sec. 7.1.2
    static bool valNgbrAd(NA* ad);
    ///Sec. 8.1
    static bool valRedirect(Redirect* re);

    bool linkLocalAddrAssigned(size_t ifIndex) const;

    /// To check if the interface has been initialized with at least one global
    //scope address
    bool globalAddrAssigned(size_t ifIndex) const;

    ///Use the prefixes received from router advertisement to create a new
    ///address which may go through DAD
    bool prefixAddrConf(size_t ifIndex, const ipv6_addr& prefix,
                        size_t prefix_len, unsigned int preferredLifetime,
                        unsigned int validLifetime);

    ///Sec 6.3.4
    virtual std::auto_ptr<RA> processRtrAd(std::auto_ptr<RA> msg);
    ///Sec 8.3
    void processRedirect(std::auto_ptr<Redirect> msg);

    ///@name Duplicate Address Detection
    //@{

    ///Prepare for DAD of tentative addr created from Router Advertisement
    ///prefixes
    void detectDupAddress(size_t ifIndex, const IPv6Address& tentativeAddr);

    ///Perform dupAddrDet on manually assigned addresses of interface
    void dupAddrDetOtherAddr(size_t ifIndex);

    ///Conduct DAD based on information in timer and retry
    void dupAddrDetection(NDTimer* tmr);

    /// Called when no response was received
    void dupAddrDetSuccess(NDTimer* tmr);

    /// Invoked to test whether received Neighbour solicitations and
    /// advertisements' target address is a tentative address of this node and
    /// process them accordingly Sec. 5.4.3-5 inclusive of RFC 2462.
    bool checkDupAddrDetected(const ipv6_addr& targetAddr, IPv6Datagram* recDgram);

    //@}

    //Returns the linkLocalAddr of the corresponding ifIndex
    IPv6Address linkLocalAddr(size_t ifIndex);

    ///please icc as it does not like pointers to non public functions used by
    ///other classes even if those classes inherit from this one.
  public:
    ///Sec 6.3.7
    void sendRtrSol(NDTimer* tmr, unsigned int ifIndex = 0);
  protected:
    void sendUnsolNgbrAd(size_t ifIndex,const ipv6_addr& target);

#if OPTIMISTIC_DAD
    ///opt dad Sec. 3.3
    ipv6_addr generateAddress(const ipv6_prefix& prefix) const;
#endif //OPTIMISTIC_DAD

    ///Chaining of callback commands onto DAD completion functions.
    void invokeCallback(const ipv6_addr& tentativeAddr);
    void addCallbackToAddress(const ipv6_addr& tentativeAddr, cTimerMessage*);
    bool callbackAdded(const ipv6_addr& tentativeAddr, int message_id);
    cTimerMessage* addressCallback(const ipv6_addr& tentativeAddr);
    //returns no. of callbacks removed
    int removeAllCallbacks();

  protected:
    std::vector<InterfaceStatus> ifStats;

    InterfaceTable *ift;
    RoutingTable6 *rt;

    bool stateEntered;

    cModule* addrResln;
    static size_t addrResGate;
    bool* rtrSolicited;
    bool managedFlag;
    bool otherFlag;

    //How to identify the response of a timed sol
    //use the source address, msg type to match.  Shouldn't be
    //that many outstanding timeout mesgs
    typedef std::list<cTimerMessage*> TimerMsgs;
    TimerMsgs timerMsgs;
    typedef TimerMsgs::iterator TMI;
  private:
    ///Whenever DAD completes on an address it should check here for callbacks
    ///that it should invoke. (Alternative was adding it to each
    ///cTimerMessage for chaining purposes but would mostly be unused so inefficient waste of memory)
    //std::map<ipv6_addr, TimerMsgs> addressCallbacks;
    std::map<ipv6_addr, cTimerMessage*> addressCallbacks;
  };

}
#endif //NDSTATEHOST_H
