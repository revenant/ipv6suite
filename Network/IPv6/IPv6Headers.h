// -*- C++ -*-
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
 * @file   IPv6Headers.h
 * @author Johnny Lai
 * @date   19.8.01
 *
 * @brief Provides the basic structures used to build the various ipv6 protocol
 * messages.
 * @test

 Output structures to screen to see if they match Header structures in RFC2460.
 Compute sizeof to test if it's same as RFC2460 in terms of size. (not a
 requirement as we are not really sending this across wire).



*/


#ifndef IPV6HEADERS_H
#define IPV6HEADERS_H

#ifndef IPv6_ADDR_H
#include "ipv6_addr.h"
#endif //IPv6_ADDR_H

#include <assert.h>

///@name IPv6 Constants and Header Enumerated Types
//@{
extern const unsigned int MAX_HOPLIMIT;

namespace
{
  const unsigned int IPv6_HEADER_LENGTH = 40; //octets
  const unsigned int IPv6_MIN_MTU = 1280;
  const unsigned int IPv6_ADDRESS_LEN = 16;
  const unsigned int IPv6_MAX_VERSION = 15;
  const unsigned int IPv6_FRAG_HDR_LEN = 8;

//Implmentation Specific constants
  const int NEXT_HDR_FOUND = -2;
  const int HEADER_NOT_FOUND = -1;
  const int IPv6_FRAG_MASK = 0x1;
  const int IPv6_RT_HDR_LEN = 8;  //Inc. the reserve bytes
};

extern const int IPv6_TYPE0_RT_HDR;
extern const int IPv6_TYPE2_RT_HDR;


/**
   Possible Next Header values to identify the contents of the following
   header/payload
*/
enum IPv6NextHeader
{
  NEXTHDR_HOP = 0, // Hop-by-hop option header.
  NEXTHDR_TCP = 6, // TCP segment.
  NEXTHDR_UDP = 17,
  NEXTHDR_IPV6 = 41,     // IPv6 in IPv6
  NEXTHDR_ROUTING = 43,  // Routing header.
  NEXTHDR_FRAGMENT = 44, // Fragmentation/reassembly header.

  // Notsupported //
  NEXTHDR_ESP = 50,  // Encapsulating security payload.
  NEXTHDR_AUTH = 51, // Authentication header.
  // NotSupported //

  NEXTHDR_ICMP = 58,    // ICMP for IPv6.
  NEXTHDR_NONE = 59,    // No next header
  NEXTHDR_DEST = 60,    // Destination options header.
  NEXTHDR_MIPV6 = 135   /// RFC 3775 MIPv6 extension header
};

/*
  #define ISVALID_IPV6_NEXT_HEADER(x)  { switch(x)                  \
  {                                                                 \
  case NEXTHDR_HOP: NEXTHDR_TCP: NEXTHDR_UDP: NEXTHDR_IPV6:         \
  NEXTHDR_ROUTING: NEXTHDR_FRAGMENT: NEXTHDR_ICMP: NEXTHDR_NONE:    \
  NEXTHDR_DEST: NEXTHDR_ESP: NEXTHDR_AUTH:                          \
  break;                                                            \
  default:                                                          \
  assert(false);                                                    \
  break;                                                            \
  } }
*/

/**
   Possible IPv6 upper protocol values
*/
/*
  enum IPv6ProtocolFieldId
  {
  IPv6_PROT_TCP = 6, // TCP segment.
  IPv6_PROT_UDP = 17,
  IPv6_PROT_IPV6 = 41,  // IPv6 in IPv6
  IPv6_PROT_ICMP = 58,  // ICMP for IPv6.
  IPv6_PROT_NONE = 59,  // No next header
  };
*/
/*
  #define ISVALID_IPV6_PROTOCOLFIELDID(x)  switch(x) \
  { \
  case IPv6_PROT_TCP: IPv6_PROT_UDP: IPv6_PROT_IPV6: \
  IPv6_PROT_ICMP: IPv6_PROT_NONE: IPv6_PROT_DEST: \
  break; \
  default: \
  assert(false); \
  break; \
  }
*/

/**
   Possible extension header types are
*/

enum IPv6ExtHeader
{
  EXTHDR_UNINITIALISED = -1,
  EXTHDR_HOP = 0,       // Hop-by-hop option header.
  EXTHDR_ROUTING = 43,  // Routing header.
  EXTHDR_FRAGMENT = 44, // Fragmentation/reassembly header.
  EXTHDR_DEST = 60,     // Destination options header.
  //Not supported i.e. is ignored completely. (log warning msg)
  EXTHDR_ESP = 50,      // Encapsulating security payload.
  EXTHDR_AUTH = 51      // Authentication header.
};
//@}

/**
    IPv6 header format:
    Version 4 Traffic Class 8 Flow Label 20
    Payload Length 16 Next header 8 Hop Limit 8
    Source Address 128
    Destination Address 128
*/
struct ipv6_hdr
{
  /**
     version is 6 for IPv6
     traffic class is 0 by default, upper layers can modify it.
     flow label is 0 as not supported and ignored
  */
  unsigned int ver_traffic_flow;
  unsigned short payload_length;
  unsigned char next_header;
  unsigned char hop_limit;
  ipv6_addr src_addr;
  ipv6_addr dest_addr;
};

bool operator== (const ipv6_hdr& lhs, const ipv6_hdr& rhs);

/**
   Extension headers are unlimited in number i.e. 0 -> 00.
   Authentication and Encapsulating Security Payload Ext. headers not implemented.
*/
struct ipv6_ext_hdr
{
  ipv6_ext_hdr(unsigned char next_header = 0, unsigned char hdr_ext_len = 0)
    :next_header(next_header), hdr_ext_len(hdr_ext_len)
    {}
  unsigned char next_header;
  unsigned char hdr_ext_len;
  virtual ~ipv6_ext_hdr();// = 0;
};


/**
   @class ipv6_option
   Primitive base for all TLV options used by destination and hop-by-hop
   extension headers
*/
struct ipv6_option
{
  virtual ~ipv6_option() = 0;
};

/**
   Hop by Hop Options Header/Destination Options Header
   Next Header Value: 0/60

   Format
   Next Header 8| Hdr Ext Len 8 (8 octets) exclude first 8 octets|
   one or more Options where Options Length%8 octets=0

   Common Options available are Pad1, PadN(RFC2460)
*/
struct ipv6_ext_opts_hdr: public ipv6_ext_hdr
{
  ///Dynamically allocated array of pointers to options
  ipv6_option** options;
};

/**
   Type Length Value (TLV) encoded options by Hop-by-Hop/Destination Option
   Extension headers.

   Format
   Option Type 8 | Option Length 8 value(length of option minus the first two
   octets)
*/
struct ipv6_tlv_option: public ipv6_option
{
  ipv6_tlv_option(unsigned char otype = 0, unsigned char olen = 0)
    :type(otype), len(olen)
    {}
  unsigned char type;
  unsigned char len;
};

/**
   Pad1 is just an octet(8 bits) of 0
*/

struct ipv6_pad1_opt: public ipv6_option
{
  static const char zero;
};

#ifndef C_ASSERT
#define C_ASSERT
#include <cassert>
#endif //C_ASSERT


/**
   Pad out the whole header to a multiple of 8 octets in length.

   PadN format:
   Option Type 8 value 1|opt data len 8 value(N-2)|N-2 octets of 0's
*/
struct ipv6_padn_opt:public ipv6_tlv_option
{
  ipv6_padn_opt(unsigned char n)
    :ipv6_tlv_option(1, n-2)
    {
      assert(n >= 2);
    }
  //N-2 octets of 0
};


/**
   Routing Ext Header
   Next Header Value: 43
   Format
   Next Header 8|Hdr Ext Len 8|Routing Type 8|Segments Left 8| type-specific data

*/
struct ipv6_ext_rt_hdr: public ipv6_ext_hdr
{
  ipv6_ext_rt_hdr(unsigned char next_header = 0, unsigned char hdr_ext_len = 0,
                  unsigned char routing_type = 0, unsigned char segments_left = 0)
    :ipv6_ext_hdr(next_header, hdr_ext_len),
//    :next_header(0), hdr_ext_len(hdr_ext_len),
     routing_type(routing_type), segments_left(segments_left)
    {}
// unsigned char next_header;
// unsigned char hdr_ext_len;
  unsigned char routing_type;
  unsigned char segments_left;
//Routing Data
};

/**
   Type 0 Header
   Reserved 32| Address[1..n] Vector of address
*/
struct ipv6_ext_rt0_hdr: public ipv6_ext_rt_hdr
{
//  struct ipv6_ext_rt _hdr rt_hdr;
  ipv6_ext_rt0_hdr(unsigned char next_header = 0, unsigned char hdr_ext_len = 0,
                   unsigned char routing_type = IPv6_TYPE0_RT_HDR,
                   unsigned char segments_left = 0, ipv6_addr* haddr = 0)
    :ipv6_ext_rt_hdr(next_header, hdr_ext_len, routing_type, segments_left),
     reserved(0), addr(haddr)
    {}
  unsigned int reserved;
  ipv6_addr* addr;
};

/**
   Fragmentation Ext header
   Next Header Value: 44
   Next Header 8| Reserved 8 always 0| Fragment Offset 13| Reserved2 2 always 0|
   M flag 1-= more fragments; 0-= last fragment| Identification 32

*/

struct ipv6_ext_frag_hdr: public ipv6_ext_hdr
{
  ipv6_ext_frag_hdr()
    :frag_off(0), frag_id(0)
    {}
  unsigned short frag_off;
  unsigned int frag_id;
};


/**
   ICMPv6 header
   Next Header Value: 58
   Type 8| Code 8| Checksum 16|
   Message body *
//Refer to icmpv6.h in linux/icmpv6.h kernel code
*/
struct ipv6_icmp_hdr
{

};

bool operator==(const ipv6_ext_hdr& lhs, const ipv6_ext_hdr& rhs);
bool operator==(const ipv6_ext_rt_hdr& lhs, const ipv6_ext_rt_hdr& rhs);

#endif //IPV6HEADERS_H


