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
 * @file MIPv6MStateMobileNode.h
 * @author Eric Wu, Johnny Lai
 * @date 16.4.2002

 * @brief Implements functionality of Mobile Node
 *
 */

#ifndef MIPV6MSTATEMOBILENODE_H
#define MIPV6MSTATEMOBILENODE_H

#ifdef USE_MOBILITY
struct ipv6_addr;

#ifndef LIST
#define LIST
#include <list>
#endif //LIST

#endif

#ifndef MIPV6MSTATECORRESPONDENTNODE_H
#include "MIPv6MStateCorrespondentNode.h"
#endif //MIPV6MSTATECORRESPONDENTNODE_H

class IPv6Datagram;
class IPv6Mobility;
class IPv6Forward;
class IPv6Encapsulation;

namespace HierarchicalMIPv6
{
  class HMIPv6CDSMobileNode;
}

namespace EdgeHandover
{
  class EHCDSMobileNode;
}

namespace MobileIPv6
{

// this is used for cellResidency scheme, where we compute success
// rate for direct signaling transmission and that we need some
// information before computations
extern const simtime_t CELL_RESI_THRESHOLD;

class BURetranTmr;
class  MIPv6CDSMobileNode;

/**
 * @class MIPv6MStateMobileNode
 * @brief Handling of mobility messages from MN perspective
 * @ingroup MobilityRoles
 */

class MIPv6MStateMobileNode : public MIPv6MStateCorrespondentNode
{
 public:
  MIPv6MStateMobileNode(IPv6Mobility* mob);

  virtual ~MIPv6MStateMobileNode(void);

  virtual void initialize(int stage = 0);

  virtual bool processMobilityMsg(IPv6Datagram* dgram);

  ///@name Implementation details
  //@{
  bool removeBURetranTmr(BURetranTmr* buTmr, bool all = false);
  //@}

  ///Sends a binding update to primary HA and HA/CN in BUL too
  void sendBUToAll(const ipv6_addr& coa, const ipv6_addr hoa, size_t lifetime);

  ///Send a binding update to dest for homeAddr with coa as the care of addr
  bool sendBU(const ipv6_addr& dest, const ipv6_addr& coa, const ipv6_addr& hoa,
              size_t lifetime, bool homeReg, size_t ifIndex
#ifdef USE_HMIP
              , bool mapReg = false
#endif //USE_HMIP
              , simtime_t timestamp = 0);

  //update the tunnels based on BU just done
  bool updateTunnelsFrom(ipv6_addr budest, ipv6_addr coa, unsigned int ifIndex,
		    bool homeReg, bool mapReg);

  void sendInits(const ipv6_addr& dest, const ipv6_addr& coa);

#ifdef USE_HMIP
  bool sendMapBU(const ipv6_addr& dest, const ipv6_addr& coa, const ipv6_addr& hoa,
                 size_t lifetime, size_t ifIndex);
#endif //USE_HMIP

  bool previousCoaForward(const ipv6_addr& coa, const ipv6_addr& hoa);

  // Loki cannot accept a TypeList with the same argument type because
  // TYPE_LIST works on GenScatterHierarchy. This class creates a
  // class hierarchy based on arguments in that macro. Thus we get the
  // ambiguous base class error if we have the same types. See Modern
  // CPP design for explanation of GenscatterHierarchy

  // addrs[0] = dest
  // addrs[1] = coa
  void sendHoTI(const std::vector<ipv6_addr> addrs, simtime_t);
  void sendCoTI(const std::vector<ipv6_addr> addrs, simtime_t);

  void recordHODelay(const simtime_t buRecvTime, ipv6_addr addr);

  //return true if further processing of packet required
  bool mnSendPacketCheck(IPv6Datagram& dgram, ::IPv6Forward* frwd);

 protected:
  ///handle Binding Acks according to draft 16 10.14
  void processBA(BA* ba, IPv6Datagram* dgram);

  void processBE(BE* bm, IPv6Datagram* dgram);

  void processBRR(BRR* br, IPv6Datagram* dgram);

  void processTest(MobilityHeaderBase* t, IPv6Datagram* dgram);

  // TODO: it would probably make more sense to add buRetransTmr in
  // each of the BUL entry instead of storing a list of buRetransTmrs
  // in the state class.

  typedef std::list<MobileIPv6::BURetranTmr*> BURetranTmrs;
  typedef BURetranTmrs::iterator BURTI;

  BURetranTmrs buRetranTmrs;

  MIPv6CDSMobileNode* mipv6cdsMN;
  HierarchicalMIPv6::HMIPv6CDSMobileNode* hmipv6cds;
  EdgeHandover::EHCDSMobileNode* ehcds;
  IPv6Encapsulation* tunMod;

private:

  void  parseXMLAttributes();

  MIPv6MStateMobileNode();
};

} // end namespace MobileIPv6

#endif // end __MIPV6MSTATEMOBILENODE_H__
