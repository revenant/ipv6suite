// -*- C++ -*-
//
// Copyright (C) 2001 CTIE, Monash University
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
    @file HdrExtRteProc.cc
    @brief Routing header operations.
    @author Johnny Lai
    @date 21.8.01
    @test
    Add routes from Routing ext header and test them for illegal
    i.e. multicast address
*/

#include "HdrExtRteProc.h"

#include <cstring>
#include <sstream>
#include <memory> //auto_ptr
#include <boost/cast.hpp>

#include "IPv6Datagram.h"
#include "LocalDeliver6Core.h"
#include "ICMPv6Message.h"
#include "IPv6Headers.h"



HdrExtRte::HdrExtRte(unsigned char rt_type)
{
  rt0_hdr = new ipv6_ext_rt0_hdr;
  setRoutingType(rt_type);
}

HdrExtRte::HdrExtRte(const HdrExtRte& src)
{
  rt0_hdr = new ipv6_ext_rt0_hdr;
  operator=(src);
}

bool HdrExtRte::operator==(const HdrExtRte& rhs)
{
  if (this == &rhs)
    return true;

  if (rt0_hdr == rhs.rt0_hdr)
    return memcmp(rt0_hdr->addr, rhs.rt0_hdr->addr, n_addr())?false:true;

  return false;
}

HdrExtRte& HdrExtRte::operator=(const HdrExtRte& rhs)
{
  if (this != &rhs)
  {
    if (rt0_hdr->addr != 0)
      delete [] rt0_hdr->addr;
      rt0_hdr->addr = 0;

      rt0_hdr->hdr_ext_len = 0;
      rt0_hdr->segments_left = 0;

      if (rhs.n_addr() > 0)
      {
        ipv6_addr* addr = new ipv6_addr[rhs.n_addr()];
        //memcpy(addr, rhs.rt0_hdr->addr, rhs.segmentsLeft()*sizeof(ipv6_addr));
        std::copy(rhs.rt0_hdr->addr, rhs.rt0_hdr->addr + rhs.n_addr(), addr);
        rt0_hdr->hdr_ext_len = rhs.rt0_hdr->hdr_ext_len;
        rt0_hdr->segments_left = rhs.rt0_hdr->segments_left;
        rt0_hdr->next_header = rhs.rt0_hdr->next_header;
        rt0_hdr->addr = addr;
      }
  }
  return *this;
}

std::ostream& HdrExtRte::operator<<(std::ostream& os)
{
  os <<" Rte0Hdr " << "n_segs="<<(int) n_addr()<<" segments left="
     <<(int) segmentsLeft()<<" len(octet)="<<length()<<"\n";
  for (int i = 0; i < n_addr(); i++)
    os << "seg #" << i<< " addr="<<rt0_hdr->addr[i]<<'\n';
  return os;

//  os <<'\0';
//  os.str().copy(buf, string::npos);
};

const ipv6_addr& HdrExtRte::address(size_t index)
{
  assert(0 != index && index <= (size_t)rt0_hdr->hdr_ext_len/2);
  assert(rt0_hdr->hdr_ext_len%2 == 0);
  return rt0_hdr->addr[index - 1];
}

///Should not be called by intermediate nodes that process this route header
void HdrExtRte::addAddress(const ipv6_addr& append_addr)
{
  //This code should work even when addr array is not initialised yet i.e. its 0
  assert(rt0_hdr->routing_type == IPv6_TYPE0_RT_HDR ||
         rt0_hdr->routing_type == IPv6_TYPE2_RT_HDR);

  ipv6_addr* addr = new ipv6_addr[segmentsLeft() + 1];
  //memcpy(addr, rt0_hdr->addr, segmentsLeft()*sizeof(ipv6_addr));
  std::copy(rt0_hdr->addr, rt0_hdr->addr + segmentsLeft(), addr);
  setSegmentsLeft(segmentsLeft() + 1);
  setHdrExtLen(hdrExtLen() + 2);
  addr[segmentsLeft() - 1 ] = append_addr;
  if (rt0_hdr->addr != 0)
    delete [] rt0_hdr->addr;
  rt0_hdr->addr = addr;
}

/**
 * invoked at LocalDeliver
 * Messages sent:
 *   -ICMP Error Messages
 *    -ICMP Packet Too Big after proc. routing header and link MTU is less
 *     than packet size
 *    -ICMP Parameter prob. code 0 point to unrecognised Routing type if
 *     segments left is > 0 otherwise discard packet.
 *
 * @warning why does dynamic cast fail is it RTTI failing and how to
 * fix it (has to be localdeliver mod, is the caller).
 */

bool HdrExtRte::processHeader(cSimpleModule* mod, IPv6Datagram* thePdu,
                              int cumul_len)
{
  static const string ICMPErrorGate("errorOut");

  std::auto_ptr<IPv6Datagram> pdu(thePdu);

  //LocalDeliver6Core* core = check_and_cast<LocalDeliver6Core*>(mod);
  LocalDeliver6Core* core = static_cast<LocalDeliver6Core*>(mod);

  assert(!pdu->destAddress().isMulticast());

  if (rt0_hdr->segments_left == 0)
  {
    pdu.release();
    return true;
  }

  int odd = rt0_hdr->hdr_ext_len%2;

  if (odd)
  {
    //Send an ICMP Parameter Problem, Code 0, message to the Source Address,
    //pointing to Hdr Ext Len field, and discard packet.
    core->send(new ICMPv6Message(ICMPv6_PARAMETER_PROBLEM, 0, pdu->dup(),
                                 cumul_len - length()// +
//                                 sizeof(struct ipv6_ext_rt_hdr::next_header)
                                 ),
               ICMPErrorGate.c_str());
    return false;

  }


  if (rt0_hdr->segments_left > n_addr())
  {
    //Send send ICMP Par Prob 0 , mess to Src Address, point to
    //segments left field and discard packet
    core->send(new ICMPv6Message(ICMPv6_PARAMETER_PROBLEM, 0, pdu->dup(),
                                 cumul_len),// -
//                                   sizeof(ipv6_ext_segments_left)),
               ICMPErrorGate.c_str());
    return false;
  }

  rt0_hdr->segments_left--;
  int index = n_addr() - rt0_hdr->segments_left ;

  //index - 1 because index in spec is from 1 to n_addr but here it's
  //actually from 0
  if (rt0_hdr->addr[index - 1].isMulticast() ||
      pdu->destAddress().isMulticast())
  {
    //drop packet
    cerr<<"Drop Routing Header packet with multicast destination addresses"<<endl;
    return false;
  }

  ipv6_addr swap_addr = rt0_hdr->addr[index - 1];
  rt0_hdr->addr[index - 1] = pdu->destAddress();
  pdu->setDestAddress(swap_addr);

  if (pdu->hopLimit() <= 1)
  {
    //drop and send Time Exceeded
    core->send(new ICMPv6Message(ICMPv6_TIME_EXCEEDED, ND_HOP_LIMIT_EXCEEDED,
                                pdu->dup()), ICMPErrorGate.c_str());
    return false;
  }

  //Routing will decrement hopLimit
  //pdu->setInputPort(-1);
  //pdu->setHopLimit(pdu->hopLimit() - 1);
#if defined TESTIPv6
  if (core)
    core->send(pdu.release(), "routingOut");
  else //Required during unit testing
    pdu.release();
#else
  core->send(pdu.release(), "routingOut");
#endif //TESTIPv6

  return true;
}

// void HdrExtRte::assign(const HdrExtRte& rhs)
// {
//   HdrExtProc::assign(rhs);
//   assert(rt0_hdr->routing_type == IPV6_TYPE0_RT_HDR);
//   if (rhs.segmentsLeft())
//     {
//       ipv6_addr* addr = new ipv6_addr[segmentsLeft()];
//       memcpy(addr, rhs.rt0_hdr->addr, segmentsLeft()*sizeof(ipv6_addr));
//       rt0_hdr->addr = addr;
//     }
// }

/* Never used as address are never removed only swapped around
void HdrExtRte::removeAddress(const ipv6_addr& addr)
{
}
*/

namespace MobileIPv6
{

/**
 * @todo make sure that the address is indeed home address of this MN
 *
 */

bool MIPv6RteOpt::processHeader(cSimpleModule* mod, IPv6Datagram* pdu,
                                int cumul_len)
{
  assert(rt0_hdr->segments_left == 1);

#if defined TESTMIPv6 || defined DEBUG_BC
  cout<<" RteType2 hdr with home addres="<<rt0_hdr->addr[0]
      <<" swapped with dest addr=<<"<<pdu->destAddress()<<"\n";
#endif //defined TESTMIPv6 || defined DEBUG_BC

  return true;
}

} //namespace MobileIPv6

// class HdrExtRteProc definition

HdrExtRteProc::HdrExtRteProc(void)
  :HdrExtProc(EXTHDR_ROUTING), rt_hdr(*boost::polymorphic_downcast<ipv6_ext_rt_hdr*>(ext_hdr))
{
  ext_hdr = new ipv6_ext_hdr();
}

HdrExtRteProc::HdrExtRteProc(const HdrExtRteProc& src)
  :HdrExtProc(EXTHDR_ROUTING), rt_hdr(*boost::polymorphic_downcast<ipv6_ext_rt_hdr*>(ext_hdr))
{
  ext_hdr = new ipv6_ext_hdr();
  operator=(src);
}

HdrExtRteProc::~HdrExtRteProc(void)
{
  for (RHIT it = rheads.begin(); it != rheads.end(); it++)
    delete *it;

  rheads.clear();
}

bool HdrExtRteProc::operator==(const HdrExtRteProc& rhs)
{
  if (this == &rhs)
    return true;

  if (HdrExtProc::operator==(rhs))
    if (rheads == rhs.rheads)
      return true;

  return false;
}

HdrExtRteProc& HdrExtRteProc::operator=(const HdrExtRteProc& rhs)
{
  if (this != &rhs)
  {
    *ext_hdr = *(rhs.ext_hdr);

    RHIT it;

    if (rheads.size() != 0)
    {
      for (it = rheads.begin(); it != rheads.end(); it++)
        delete *it;

      rheads.clear();
      }

    for (it = rhs.rheads.begin(); it != rhs.rheads.end(); it++)
      rheads.push_back((*it)->dup());
  }

  return *this;
}

std::ostream& HdrExtRteProc::operator<<(std::ostream& os)
{
  return HdrExtProc::operator<<(os);
}

bool HdrExtRteProc::addRoutingHeader(HdrExtRte* rh)
{
  for (RHIT it = rheads.begin(); it != rheads.end(); it++)
  {
    if((*it)->routingType() == rh->routingType())
      return false;
  }

  rheads.push_back(rh);
  return true;
}


HdrExtRte* HdrExtRteProc::routingHeader(unsigned char rt_type)
{
  for (RHIT it = rheads.begin(); it != rheads.end(); it++)
  {
    if((*it)->routingType() == rt_type)
      return (*it);
  }

  return 0;
}

size_t HdrExtRteProc::length() const
{
  size_t len = 0;

  //accumulate(rheads.begin(), rheads.end(), _1->length());
  for (RHIT it = rheads.begin(); it != rheads.end(); it++)
    len += (*it)->length();
  return len;
}

bool HdrExtRteProc::processHeader(cSimpleModule* mod, IPv6Datagram* pdu)
{
  int cuml_len_to_rh =  cumul_len(*pdu);

 for (RHIT it = rheads.begin(); it != rheads.end(); it++)
 {
   // cummulated length up to a particular routing header
   cuml_len_to_rh += (*it)->length();

   if ( (*it)->segmentsLeft() != 0)
   {
     bool success = (*it)->processHeader(mod, pdu, cuml_len_to_rh);

     if(!success)
       return false;
   }
 }

 return true;
}

bool HdrExtRteProc::isSegmentsLeft(void)
{
 for (RHIT it = rheads.begin(); it != rheads.end(); it++)
 {
   if ((*it)->segmentsLeft() != 0)
     return true;
 }

 return false;
}
