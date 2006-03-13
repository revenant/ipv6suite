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
 * @file MIPv6MStateCorrespondentNode.h
 * @author Eric Wu, Johnny Lai
 * @date 16.4.2002

 * @brief Implements functionality of Correspondent Node
 *
 */

#ifndef MIPV6MSTATECORRESPONDENTNODE_H
#define MIPV6MSTATECORRESPONDENTNODE_H

#ifndef MIPV6MOBILTIYSTATE_H
#include "MIPv6MobilityState.h"
#endif // MIPV6MOBILTIYSTATE_H

class IPv6Mobility;
class IPv6Datagram;

namespace MobileIPv6
{

  class MIPv6CDS;

/**
   @class MIPv6MStateCorrespondentNode
   @brief Define behaviour of CN role in IPv6Mobility module
   @ingroup MobilityRoles
*/


class MIPv6MStateCorrespondentNode : public MIPv6MobilityState
{
 public:

  MIPv6MStateCorrespondentNode(IPv6Mobility* mod);
  ~MIPv6MStateCorrespondentNode();

  virtual bool processMobilityMsg(IPv6Datagram* dgram);

  //returns true if packet was modified
  bool cnSendPacketCheck(IPv6Datagram& dgram);

 protected:
  virtual bool processBU(IPv6Datagram* dgram, BU* bu);

  void processTI(TIMsg* timsg, IPv6Datagram* dgram);

  // send binding missing
  void sendBM(const ipv6_addr& srcAddr, const ipv6_addr& destAddr, BM* bm);

  cTimerMessage* periodTmr;
};

} // end namespace MobileIPv6

#endif

