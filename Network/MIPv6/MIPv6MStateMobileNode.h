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

#ifndef LIST
#define LIST
#include <list>
#endif //LIST

#ifndef MIPV6MSTATECORRESPONDENTNODE_H
#include "MIPv6MStateCorrespondentNode.h"
#endif //MIPV6MSTATECORRESPONDENTNODE_H

#ifndef CTIMERMESSAGECB_H
#include "cTimerMessageCB.h" //schedSendBU
#endif //  CTIMERMESSAGECB_H



class IPv6Datagram;
class IPv6Mobility;

namespace MobileIPv6
{

// this is used for cellResidency scheme, where we compute success
// rate for direct signaling transmission and that we need some
// information before computations
extern const simtime_t CELL_RESI_THRESHOLD;

class BURetranTmr;

/**
 * @class MIPv6MStateMobileNode
 * @brief Handling of mobility messages from MN perspective
 * @ingroup MobilityStates
 */

class MIPv6MStateMobileNode : public MIPv6MStateCorrespondentNode
{
 public:
  static MIPv6MStateMobileNode* instance(void);

  virtual ~MIPv6MStateMobileNode(void);

  virtual void processMobilityMsg(IPv6Datagram* dgram,
                                  MIPv6MobilityHeaderBase*& mhb,
                                  IPv6Mobility* mod);

  ///@name Implementation details
  //@{
  void removeBURetranTmr(BURetranTmr* buTmr);
  //@}

  ///Sends a binding update to primary HA and HA/CN in BUL too
  void sendBUToAll(const ipv6_addr& coa, const ipv6_addr hoa, size_t lifetime,
                   IPv6Mobility* mob);

  ///Send a binding update to dest for homeAddr with coa as the care of addr
  bool sendBU(const ipv6_addr& dest, const ipv6_addr& coa, const ipv6_addr& hoa,
              size_t lifetime, bool homeReg, size_t ifIndex, IPv6Mobility* mod
#ifdef USE_HMIP
              , bool mapReg = false
#endif //USE_HMIP
              , simtime_t timestamp = 0);

  void sendInits(const ipv6_addr& dest, const ipv6_addr& coa,
                 IPv6Mobility* mod);

#ifdef USE_HMIP
  bool sendMapBU(const ipv6_addr& dest, const ipv6_addr& coa, const ipv6_addr& hoa,
                 size_t lifetime, size_t ifIndex, IPv6Mobility* mod);
#endif //USE_HMIP

  bool previousCoaForward(const ipv6_addr& coa, const ipv6_addr& hoa, IPv6Mobility* mob);

  // Loki cannot accept a TypeList with the same argument type because
  // TYPE_LIST works on GenScatterHierarchy. This class creates a
  // class hierarchy based on arguments in that macro. Thus we get the
  // ambiguous base class error if we have the same types. See Modern
  // CPP design for explanation of GenscatterHierarchy

  // addrs[0] = dest
  // addrs[1] = coa
  void sendHoTI(const std::vector<ipv6_addr> addrs, IPv6Mobility* mob, simtime_t);
  void sendCoTI(const std::vector<ipv6_addr> addrs, IPv6Mobility* mob, simtime_t);

  void recordHODelay(const simtime_t buRecvTime, ipv6_addr addr, IPv6Mobility* mob);

 protected:
  ///handle Binding Acks according to draft 16 10.14
  void processBA(BA* ba, IPv6Datagram* dgram, IPv6Mobility* mod);

  void processBM(BM* bm, IPv6Datagram* dgram, IPv6Mobility* mod);

  void processBR(BR* br, IPv6Datagram* dgram, IPv6Mobility* mod);

  void processTestMsg(TMsg* t, IPv6Datagram* dgram, IPv6Mobility* mod);

  ///Update the BU list with this recently sent BU
  bool updateBU(const BU* bu, IPv6Mobility* mod);

  ///Called by layer 2 whenever movement is detected there
  void l2MovementDetectionCB(cMessage* msg);


  static MIPv6MStateMobileNode* _instance;
  MIPv6MStateMobileNode(void);

private:
  ///Schedule a self message to send BU from any module
  void scheduleSendBU(IPv6Datagram* dgram, IPv6Mobility* mob);

  typedef std::list<BURetranTmr*> BURetranTmrs;
  typedef BURetranTmrs::iterator BURTI;

  // TODO: it would probably make more sense to add buRetransTmr in
  // each of the BUL entry instead of storing a list of buRetransTmrs
  // in the state class.

  BURetranTmrs buRetranTmrs;

  Loki::cTimerMessageCB
  <void, TYPELIST_4(cMessage*, const char*, cSimpleModule*, cTimerMessage*)>*
  schedSendBU;
};

} // end namespace MobileIPv6

#endif // end __MIPV6MSTATEMOBILENODE_H__
