// -*- C++ -*-
// Copyright (C) 2002, 2004 CTIE, Monash University 
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

/**
   @class MIPv6MStateCorrespondentNode
   @brief Defines behaviour of MNs in IPv6Mobility module 	
   @ingroup MobilityStates
*/


class MIPv6MStateCorrespondentNode : public MIPv6MobilityState
{
 public:
  static MIPv6MStateCorrespondentNode* instance(void);

  virtual void processMobilityMsg(IPv6Datagram* dgram, 
                                  MIPv6MobilityHeaderBase*& mhb,
                                  IPv6Mobility* mod);
  
 protected:
  virtual bool processBU(IPv6Datagram* dgram, BU* bu, IPv6Mobility* mod);

  void processTI(TIMsg* timsg, IPv6Datagram* dgram, IPv6Mobility* mod);

  // send binding missing
  void sendBM(const ipv6_addr& srcAddr, const ipv6_addr& destAddr, BM* bm, 
              IPv6Mobility* mod);

  static MIPv6MStateCorrespondentNode* _instance;  
};

} // end namespace MobileIPv6

#endif
