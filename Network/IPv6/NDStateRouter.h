// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/NDStateRouter.h,v 1.1 2005/02/09 06:15:58 andras Exp $
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
   @file NDStateRouter.h
   @brief Definition of NDStateRouter class that implements Neighbour Discovery
   mehanisms defined in RFC2461 and AutofConf in RFC2462 for a router.

   @author Johnny Lai
   @date 24.9.01
*/

#if !defined NDSTATEROUTER_H__
#define NDSTATEROUTER_H__

#ifndef NDSTATES_H
#include "NDStateHost.h"
#endif //NDSTATES_H

#ifndef INTERFACE6ENTRY_H
#include "Interface6Entry.h"
#endif //INTERFACE6ENTRY_H

#ifdef USE_HMIP
#ifndef HMIPV6ICMPV6NDMESSAGE_H
#include "HMIPv6ICMPv6NDMessage.h"
#endif //HMIPV6ICMPV6NDMESSAGE_H
#endif //USE_HMIP

#include <memory> //auto_ptr

namespace IPv6NeighbourDiscovery
{
  class RtrTimer;
  
/**
   @class NDStateRouter
   
   @brief Implements functionality that is specific to a router.  Inherits
   common functionality from NDStateHost
*/
  class NDStateRouter: public NDStateHost
  {
  public:

    NDStateRouter(NeighbourDiscovery* mod);  
    ~NDStateRouter();
    
    virtual std::auto_ptr<ICMPv6Message> processMessage(std::auto_ptr<ICMPv6Message> msg);    

    virtual void print(){};

    ///Sec. 8.2
    void sendRedirect(IPv6Datagram* theDgram, const ipv6_addr& nextHop, bool& redirected);
  
  protected:
    ///Sec 6.2.2
    virtual void enterState();
    ///Sec 6.2.5
    virtual void leaveState();    

    ///Sec 6.2.2
    virtual void initialiseInterface(size_t ifIndex);
    ///Sec 6.2.5
    virtual void disableInterface(size_t ifIndex);

    ///Routers check consistency of other routers Sec. 6.2.7
    virtual bool valRtrAd(RA* ad);      

    bool valRtrSol(RS* rs);
    ///Sec 6.2.6
    void processRtrSol(RS* rs);  
    ///Sec 6.2.4
    virtual void sendUnsolRtrAd( RtrTimer* tmr);
    
    ///Sec 6.2.6
    void sendRtrAd(RS* rtrSol);    

    // vritual create handling RA; New RA may be override if new
    // handler (inherited from NDStateRouter) is defined 
    // ifidx is used by HMIPv6NDStateRouter
    virtual ICMPv6NDMRtrAd* createRA(const Interface6Entry::RouterVariables& rtr, size_t ifidx);

    std::vector<RtrTimer*> advTmrs;  

#ifdef USE_HMIP

    /**
     * Receive Router Adv and search for MAP options.  If found send a RtrAdv
     * with those MAP options after incrementing the distance (Not storing MAP
     * options)
     * 
     */

    virtual std::auto_ptr<RA> processRtrAd(std::auto_ptr<RA> msg);
#endif //USE_HMIP

  private:

    /** 
        Both unsolicited and solicited rtr adv are returned by this function.
        Not allowing override because it does things all routers should do when
        sending rtrAdv especially

        @todo routers forward map options if IPv6Suite built with hmip
        support. We should forward MAP options if router has map support turned
        on or perhaps even add a forward map option toggle per interface.
     */
    IPv6Datagram* createDatagram(unsigned int ifIndex, const ipv6_addr& destAddr);


    // constants changable within different protocols e.g IPv6/MIPv6
    double MIN_DELAY_BETWEEN_RAS;
    double MAX_INITIAL_RTR_ADVERT_INTERVAL;  
    size_t MAX_INITIAL_RTR_ADVERTISEMENTS;
    size_t MAX_FINAL_RTR_ADVERTISEMENTS;
    double MAX_RA_DELAY_TIME;

#ifdef USE_HMIP
    HierarchicalMIPv6::MAPOptions transitMapOpts;
#endif //USE_HMIP

    ///unicast send module
    cModule** outputMod;
    ///unicast send gate
    static unsigned int outputUnicastGate;

  };  
  
  typedef std::vector<RtrTimer*> RTI;
#ifdef USE_HMIP
  typedef HierarchicalMIPv6::MAPOptions::iterator MAPIt;
#endif // USE_HMIP
}

#endif //NDSTATEROUTER_H__
