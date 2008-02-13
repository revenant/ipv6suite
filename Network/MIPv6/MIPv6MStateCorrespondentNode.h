// -*- C++ -*-
// Copyright (C) 2002, 2004 CTIE, Monash University
// Copyright (C) 2006 by Johnny Lai
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

#ifndef TYPES_TYPEDEF_H
#include "types_typedef.h"
#endif //TYPES_TYPEDEF_H

class IPv6Mobility;
class IPv6Datagram;
class MobilityHeaderBase;

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
  virtual ~MIPv6MStateCorrespondentNode();

  virtual bool processMobilityMsg(IPv6Datagram* dgram);

  //returns true if packet was modified
  bool cnSendPacketCheck(IPv6Datagram& dgram);

 protected:
  virtual bool processBU(BU* bu, IPv6Datagram* dgram);

  void processTI(MobilityHeaderBase* timsg, IPv6Datagram* dgram);

  //used to expire lifetimes of bce, MIPv6CDS::expireLifetimes
  cTimerMessage* periodTmr;

  ///nonces
  //@{
  void nonceGeneration();

  cTimerMessage* nonceGenTmr;
  unsigned int noncesIndex;
  //8 derived from MAX_NONCE_LIFETIME/30 where 30 is period of new nonce
  //generation
  u_int16 nonces[8];
  //@}
};

} // end namespace MobileIPv6

#endif

