//
// Copyright (C) 2002 CTIE, Monash University
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
    @file MIPv6DestOptMessages.cc
    @brief MIPv6 Destination Options and Sub-Options
    @see draft-ietf-mobileip-ipv6-16.txt, section 5.3 and section 5.5
    @author Eric Wu
    @date 10/4/2002
*/

#include "IPv6LocalDeliver.h"
#include "MIPv6DestOptMessages.h"

namespace MobileIPv6
{

/**
 * @todo Implement this according to 5.3
 *
 * @warning don't ever touch HAOption.  We don't need to save the care of addr.
 * It should already be in BC.  If it is not in BC then drop packet.  Read spec.

 * Another anomaly that the spec does not clarify is the fact that no BC
 * entry exists prior to sending the first BU.  Thus we cannot drop those packets
 * which contain an MIPv6 BU message which requests to cache a new binding.
 */

bool MIPv6TLVOptHomeAddress::processOption(cSimpleModule* mod, IPv6Datagram* dgram)
{
  ///Drop if no binding for this home addr exists with matching care of addr as
  ///src addr of this packet.  Do this only after Return Routability Procedure
  ///has been implmented.  Send BM if dropped.  Check that no other HAOpt
  ///exists.

  //ipv6_addr coa = dgram->srcAddres();
  if (dgram->transportProtocol() != IP_PROT_IPv6_MOBILITY)
  {
#ifdef TESTMIPv6
  cout<<"Processed DestOpt HomeAddress ha=:"<<opt.home_addr<<" coa="
      <<dgram->srcAddress()<<"\n";
#endif //TESTMIPv6
    dgram->setSrcAddress(opt.home_addr);
  }

  //It doesn't say swap in the spec. but it's simplest method rather than check
  //whether payload is MIPv6 protocol but perhaps that's more precise and clear
  //opt.home_addr = coa;
  return true;
}

} //namespace MobileIPv6

//Duplicated class !!!!
// namespace MobileIPv6
// {

// // class MIPv6TLVOptHomeAddress definition

// MIPv6TLVOptHomeAddress::MIPv6TLVOptHomeAddress(const ipv6_addr& addr)
//   : IPv6TLVOptionBase(MIPv6_HOME_ADDRESS_OPT, 18),
//     _homeAddress(addr)
// {}

// MIPv6TLVOptHomeAddress::~MIPv6TLVOptHomeAddress(void)
// {
//   _subOptions.empty();
// }

// bool MIPv6TLVOptHomeAddress::
// processOption(cSimpleModule* mod, IPv6Datagram* pdu)
// {
//   ipv6_addr careofaddr = pdu->srcAddress();

//   // sec 8.1 - swap the original value of the source address field
//   // of IPv6 header with home address field in the home address option
//   pdu->setSrcAddress(_homeAddress);

//   _homeAddress = careofaddr;

//   return true;
// }

// } // end namespace MobileIPv6
