// -*- C++ -*-
//
// Copyright (C) 2001 CTIE, Monash University
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
    @file IPv6Headers.cc
    @brief Implementation of global operators  of for IPv6 headers.
    @author Johnny Lai
    @date 19.8.01

    @test Output structures to screen to see if they match Header structures in
    RFC2460.  Compute sizeof to test if it's same as RFC2460 in terms of
    size. (not a requirement as we are not really sending this across wire).



*/
#include "IPv6Headers.h"


const unsigned int MAX_HOPLIMIT = 255;
const int IPv6_TYPE0_RT_HDR = 0;
const int IPv6_TYPE2_RT_HDR = 2;

const char ipv6_pad1_opt::zero = 0;

ipv6_ext_hdr::~ipv6_ext_hdr(){}

ipv6_option::~ipv6_option(){}

/* XXX not needed
bool operator== (const ipv6_hdr& lhs, const ipv6_hdr& rhs)
{
  return lhs.ver_traffic_flow == rhs.ver_traffic_flow &&
    lhs.payload_length == rhs.payload_length && lhs.next_header == rhs.next_header &&
    lhs.hop_limit == rhs.hop_limit && lhs.src_addr == rhs.src_addr &&
    lhs.dest_addr == rhs.dest_addr;

}
*/

bool operator==(const ipv6_ext_hdr& lhs, const ipv6_ext_hdr& rhs)
{
  if (lhs.next_header == rhs.next_header)
    return true;
  else
    return false;
}

bool operator==(const ipv6_ext_rt_hdr& lhs, const ipv6_ext_rt_hdr& rhs)
{
  return lhs.next_header == rhs.next_header && lhs.hdr_ext_len == rhs.hdr_ext_len &&
    lhs.routing_type == rhs.routing_type && lhs.segments_left == rhs.segments_left;
}



