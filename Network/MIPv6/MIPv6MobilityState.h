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
class cTimerMessage;

namespace
{
  const unsigned int MIPv6_PERIOD = 1 ; //seconds
};

class MobilityHeaderBase;
class BRR;
class HOTI;
class COTI;
class HOT;
class COT;
class BU;
class BA;
class BE;

namespace MobileIPv6
{

  class MIPv6CDS;

/**
   @class MIPv6MobilityState
   @brief Base class for IPv6Mobility simple module behaviour
   @ingroup MobilityRoles

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

  ///Pass some stage specific initialisation code here
  virtual void initialize(int stage = 0);

  //@return true if it processed the message false otherwise
  virtual bool processMobilityMsg(IPv6Datagram* dgram) = 0;

  /**
   @brief RFC 3775 9.3.1 checks on packets with a received hoa dest option	
   @param hoa home address from the home address destination option inside dgram
   @param dgram received datagram

   @return true if further processing of datagram, false if datagram should be
   deleted and dropped by IPv6LocalDeliver

   @see MIPv6TLVOptHomeAddress::processOption which calls this and does actual
   processing
  */
  bool processReceivedHoADestOpt(ipv6_addr hoa, IPv6Datagram* dgram);

 protected:
  MIPv6MobilityState(IPv6Mobility* mod);

  ///@name process specific type of mobility message
  //@{
  /// process binding update
  virtual bool processBU(BU* bu, IPv6Datagram* dgram) = 0;

  bool preprocessBU(BU* bu, IPv6Datagram* dgram,
                    ipv6_addr& hoa, ipv6_addr& coa, bool& hoaOptFound);

  /// send binding acknowledgement
  /// @param hoa is the home address in hoa option if different from srcAddr
  void sendBA(const ipv6_addr& srcAddr, const ipv6_addr& destAddr, const ipv6_addr& hoa,
              BA* ba, simtime_t timestamp = 0, unsigned int ifIndex = 0);


  /**
   @brief RFC 3775 9.3.3 send Binding Error depending on status of dgram
   @param status 1 for missing binding, 2 for unrecognised mobility header
   @param dgram is received dgram with error    

   @note Currently assuming the swapping of hoa opt with src addr in dgram has
   not occurred @see processReceivedHoADestOpt and
   MIPv6TLVOptHomeAddress::processOption where this occurs
  */

  void sendBE(int status, IPv6Datagram* dgram);

  virtual void defaultResponse(MobilityHeaderBase* mhb, IPv6Datagram* dgram);
  //@}

  ///@name handle management of binding cache
  //@{
  /// add or update binding cache entry in binding cache
  virtual void registerBCE(BU* bu, const ipv6_addr& hoa, IPv6Datagram* dgram);

  /// delete binding update (CN) or primary care-of address
  /// de-registration (HA);
  virtual bool deregisterBCE(BU* bu, const ipv6_addr& hoa, unsigned int ifIndex);
  //@}

  MobilityHeaderBase* mobilityHeaderExists(IPv6Datagram* dgram);
  ///returns false if type 2 routing header could not be added
  bool addRoutingHeader(const ipv6_addr& hoa, IPv6Datagram* dgram);

  IPv6Datagram* constructDatagram(const ipv6_addr& dest, const ipv6_addr& src, 
				  cMessage* const msg, unsigned int ifIndex,
				  simtime_t timestamp = 0) const;

  IPv6Mobility* mob;
  MIPv6CDS* mipv6cds;

private:
  MIPv6MobilityState(void);
};

} // end namespace MobileIPv6

#endif // MIPV6MOBILTIYSTATE_H
