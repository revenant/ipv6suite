//
// Copyright (C) 2002 CTIE, Monash University
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
    @file MIPv6DestOptMessages.cc
    @brief MIPv6 Destination Options and Sub-Options
    @see draft-ietf-mobileip-ipv6-16.txt, section 5.3 and section 5.5
    @author Eric Wu
    @date 10/4/2002
*/

#include "IPv6LocalDeliver.h"
#include "MIPv6DestOptMessages.h"
#include "opp_utils.h"
#include "IPv6Mobility.h"

namespace MobileIPv6
{

/**
 * RFC 3775 9.3.1
 *
 * @see see 9.3.3 for MIPv6MobilityState::sendBE for reason why we keep coa
 * inside hoa dest option after processing
 */

bool MIPv6TLVOptHomeAddress::processOption(cSimpleModule* mod, IPv6Datagram* dgram)
{
  IPv6Mobility* mob = check_and_cast<IPv6Mobility*>(OPP_Global::findModuleByType(mod, "IPv6Mobility"));

  if (mob->processReceivedHoADestOpt(opt.home_addr, dgram))
  {
    bool process = true;
    process = !(dgram->transportProtocol() == IP_PROT_IPv6_MOBILITY);

    if (process)
    {
    ipv6_addr coa = dgram->srcAddress();

#ifdef TESTMIPv6
    cout<<"Processed DestOpt HomeAddress ha=:"<<opt.home_addr<<" coa="
	<<dgram->srcAddress()<<"\n";
#endif //TESTMIPv6
    dgram->setSrcAddress(opt.home_addr);

    //It doesn't say swap in the spec. but we need coa sometimes as its the
    //original src addr on wire see 9.3.3 in MIPv6MobilityState::sendBE
    opt.home_addr = coa;
    }
    else
    {
      //skip destopt processing for mobility messages as we want the values from
      //the wire
      
      //doesn't say to do this in spec other than to skip the binding cache
      //tests but this way leads to less code in mobility and consistent
      //handling especially for bu where we need both hoa/coa and it assumes hoa
      //in hoa option (9.5.1)
    }
    return true;
  }
  return false;
}

std::ostream& MIPv6TLVOptHomeAddress::operator<<(std::ostream& os)
{
  return os<<" destination option: home addr option hoa="<<homeAddr()<<"\n";
}

} //namespace MobileIPv6

