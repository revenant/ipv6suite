// -*- C++ -*-
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
 * @file MIPv6NDStateHost.h
 * @author Johnny Lai
 * @date 11 Apr 2002

 * @brief Changes to NDStateHost to handle processing of new Router Adv format
 * inc. adding to HA List

 *
 */

#ifndef MIPV6NDSTATEHOST_H
#define MIPV6NDSTATEHOST_H

#ifndef NDSTATEHOST_H
#include "NDStateHost.h"
#endif // NDSTATEHOST_H

class IPv6Mobility;

namespace MobileIPv6
{

  class MIPv6CDSMobileNode;
  class MIPv6RouterEntry;
  class RtrAdvMissedTmr;
  class MIPv6MStateMobileNode;

/**
 * @class MIPv6NDStateHost
 * @brief Extend the NeighbourDiscovery mechanisms for hosts in MIPv6

 * Changes to NDStateHost to handle processing of new Router Adv format
 * inc. adding to HA List
 */

class MIPv6NDStateHost : public IPv6NeighbourDiscovery::NDStateHost
{
 public:
  MIPv6NDStateHost(NeighbourDiscovery* mod);
  virtual ~MIPv6NDStateHost(void);
    
  //virtual void processMessage(ICMPv6Message* msg);
  
  ///Handle RtrAd with the HomeAgent bit set
  virtual std::auto_ptr<RA> processRtrAd(std::auto_ptr<RA> msg);

/*
  ///Changes to Sending Rtr Solicitations 
  ///@deprecated in draft 24
  void sendRtrSol(NDTimer* tmr);
*/
  ///Determine if this node is away from home subnet
  bool awayFromHome();

  ///Invoked by other processes whenever movement is detected
  void movementDetectedCallback(cTimerMessage* tmr);

  virtual void print(){};
  
 protected:
  virtual void handover(boost::shared_ptr<MIPv6RouterEntry> newRtr);

  ///Check and do returning home case
  void returnHome();
  
  ///Implementation of callback invoked by IPv6Encapsulation
  void checkDecapsulation(IPv6Datagram* dgram);

  ///Used by modules != NeighbourDiscovery to initiate a sendRtrSol
  void scheduleSendRtrSol(NDTimer* tmr);

  ///Check that no outstanding rtrSol is already happening before starting one
  void initiateSendRtrSol(unsigned int ifIndex);

  void relinquishRouter(boost::shared_ptr<MIPv6RouterEntry> oldRtr,
                        boost::shared_ptr<MIPv6RouterEntry> newRtr);

  void recordL2LinkUpTime(simtime_t delay);

  void recordL2LinkDownTime(simtime_t linkdownTime);

  // virtual void enterState();
//   virtual void leaveState();    
  
//   virtual void initialiseInterface(size_t ifIndex);
//   virtual void disableInterface(size_t ifIndex);
  
  ///Hosts simple checks Sec. 6.1.2
  //virtual bool valRtrAd(RA* ad);  

public:
  ///gcc34 does not allow friendship to be granted to a protected function of base class? Guess friendship and inheritance are orthogonal
  ///Need access to sendBU
  //friend void IPv6NeighbourDiscovery::NDStateHost::dupAddrDetSuccess(NDTimer* tmr);  
  void sendBU(const ipv6_addr& ncoa);
protected:

  MIPv6CDSMobileNode* mipv6cdsMN;
  bool awayCheckDone;
  ///Timer that will trigger whenever a router adv is missed according to
  ///router adv. interval option
  RtrAdvMissedTmr* missedTmr;
  IPv6Mobility* mob;
  MIPv6MStateMobileNode* mstateMN;
  
  ///Used by other modules to schedule a sendRtrSol from ND
  cTTimerMessageCBA<NDTimer, void>* schedSendRtrSolCB;
  cTimerMessage* schedSendUnsolNgbrAd;
private:
  ///missedRtrAdv variable to store interval from new router advertisement when
  ///unsure if new router is on different subnet
  simtime_t potentialNewInterval;
  //Needed by dupAddrDetSuccess to call sendBU
  ipv6_addr futureCoa;
  bool ignoreInitiall2trigger;
}; 

} //namespace MobileIPv6

#endif // MIPV6NDSTATEHOST_H
