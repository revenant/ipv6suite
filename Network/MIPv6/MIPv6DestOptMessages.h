// -*- C++ -*-
// Copyright (C) 2002 CTIE, Monash University
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
    @file MIPv6DestOptMessages.h
    @brief MIPv6 Destination Options and Sub-Options
    @see draft-ietf-mobileip-ipv6-16.txt, section 5.3 and section 5.5
    @author Eric Wu
    @date 10/4/2002
*/

#ifndef __MIPv6DESTOPTMESSAGES_H__
#define __MIPv6DESTOPTMESSAGES_H__

#ifndef HDREXTDESTPROC_H
#include "HdrExtDestProc.h"
#endif // HDREXTDESTPROC_H

#ifndef IPv6_ADDRESS_H
#include "ipv6_addr.h"
#endif // IPv6_ADDRESS_H

/**
   @namespace MobileIPv6
   Contains all the logical entities used in implementing Draft 16 of MIPv6
*/
namespace MobileIPv6
{

/**
   @struct mipv6_home_address_opt

   @brief Home Address Destination option

   home address must not be a multicast/link-local/loopback/ipv4
   derived/unspecified. 8n+6.

   After routing header, before fragment/ AH/ ESP header
*/
  struct mipv6_home_address_opt: public ipv6_tlv_option
  {
    mipv6_home_address_opt(const ipv6_addr& addr)
      :ipv6_tlv_option((unsigned char)IPv6TLVOptionBase::MIPv6_HOME_ADDRESS_OPT,
                       (unsigned char) 16),home_addr(addr){};
    ipv6_addr home_addr;
  };

  /**
   * @class MIPv6TLVOptHomeAddress
   * @brief Implementation of Home option
   *
   * @todo check validity of home addr in ctor via asserts according to 5.3 of MIPv6 draft 16
   */

  class MIPv6TLVOptHomeAddress:public IPv6TLVOptionBase
  {
  public:

    MIPv6TLVOptHomeAddress(const ipv6_addr& haddr)
      :IPv6TLVOptionBase(IPv6TLVOptionBase::MIPv6_HOME_ADDRESS_OPT, (unsigned char) 16), opt(haddr)
      {}

    virtual std::ostream& operator<<(std::ostream& os);

    virtual MIPv6TLVOptHomeAddress* dup() const
      {
        return new MIPv6TLVOptHomeAddress(opt.home_addr);
      }

    const ipv6_addr& homeAddr(void) { return opt.home_addr; }

    ///Process Home Address Option according to 5.3
    virtual bool processOption(cSimpleModule* mod, IPv6Datagram* dgram);
  private:
    mipv6_home_address_opt opt;
  };

};

#endif // __MIPv6DESTOPTMESSAGES_H__
