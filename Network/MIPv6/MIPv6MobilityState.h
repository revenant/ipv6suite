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
 * @file MIPv6MobilityState.h
 * @author Eric Wu, Johnny Lai
 * @date 16.4.2002

 * @brief MIPv6MobilityState handles mobility messages. Depending on
 * the type of mobility messages, MIPv6MobilityState switches to the
 * appropriate state (ie. MN, CN or HA); It also manages binding cache
 * replacement policy
 *
 *
 */

#ifndef MIPV6MOBILTIYSTATE_H
#define MIPV6MOBILTIYSTATE_H

#include <omnetpp.h> // for simtime_t

#include "ipv6_addr.h" // for including Ipv6_Addr.H_UNSPECIFIED

class IPv6Mobility;
class IPv6Datagram;

namespace MobileIPv6
{

// class forward declaration

class MIPv6MobilityHeaderBase;
class MIPv6MHBindingRequest;
class MIPv6MHTestInit;
class MIPv6MHTest;
class MIPv6MHBindingUpdate;
class MIPv6MHBindingAcknowledgement;
class MIPv6MHBindingMissing;

// typedef

typedef MIPv6MHBindingRequest         BR;
typedef MIPv6MHTestInit               TIMsg;
typedef MIPv6MHTest                   TMsg;
typedef MIPv6MHBindingUpdate          BU;
typedef MIPv6MHBindingAcknowledgement BA;
typedef MIPv6MHBindingMissing         BM;

/**
   @class MIPv6MobilityState
   @brief Base class for IPv6Mobility simple module behaviour
   @ingroup MobilityStates

   Classes in this group do not contain any data local data members other than
   the static instance of themselves. The actual data is stored inside
   MobileIPv6::MIPv6CDS which is obtainable from the mod pointer that is passed
   in as an argument to every function. Should have refactored this from the
   beginning not to use State Pattern and treat them as normal classes.

   @note In appearance this is a state pattern but in reality does not change
   states unless we get a MAP which is also a mobile node??
*/

extern const int TENTATIVE_BINDING_LIFETIME;

class MIPv6MobilityState
{
 public:
  virtual ~MIPv6MobilityState(void);

  /**
   * We want MIPv6MobilityHeaderBase*& (2nd argument) because we need to also
   * delete the instance of mobility message after use. It is best to delete the
   * mobility message from the top function that calls this function so this
   * virtual function does not hold responsiblity to delete the instance for
   * extension purpose.  process mobility message
   *
   */
  virtual void processMobilityMsg(IPv6Datagram* dgram,
                                  MIPv6MobilityHeaderBase*& mhb,
                                  IPv6Mobility* mod);

  // go to next state according to the mobility message in the
  // datagram; return true if the datagram contains mobility message
  // or of the correct type
  static bool nextState(IPv6Datagram* dgram, IPv6Mobility* mod);

 protected:
  MIPv6MobilityState(void);

  ///@name process specificy type of mobility message
  //@{
  /// process binding update
  virtual bool processBU(IPv6Datagram* dgram, BU* bu, IPv6Mobility* mod) = 0;

  bool preprocessBU(IPv6Datagram* dgram, BU* bu, IPv6Mobility* mod,
                    ipv6_addr& hoa, ipv6_addr& coa);

  /// send binding acknowledgement
  void sendBA(const ipv6_addr& srcAddr, const ipv6_addr& destAddr,
              BA* ba,  IPv6Mobility* mod, simtime_t timestamp = 0);
  //@}

  ///@name handle management of binding cache
  //@{
  /// add or update binding cache entry in binding cache
  virtual void registerBCE(IPv6Datagram* dgram, BU* bu, IPv6Mobility* mod);

  /// delete binding update (CN) or primary care-of address
  /// de-registration (HA);
  virtual bool deregisterBCE(BU* bu, unsigned int ifIndex, IPv6Mobility* mod);
  //@}
};

} // end namespace MobileIPv6

#endif // MIPV6MOBILTIYSTATE_H
