// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/ICMPv6Message.cc,v 1.4 2005/02/11 12:23:46 andras Exp $
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
    @file ICMPv6Message.cc
    @brief Implementation of ICMPv6Message and ICMPv6Echo class
    @author Johnny Lai
    @date 14.9.01

    network byte ordering by Greg Daley
    icmp6 checksum inspired by linux/net/ipv6/ndisc.c
    by  Pedro Roque             <roque@di.fc.ul.pt>
    and Mike Shaver             <shaver@ingenia.com>

*/

#include <omnetpp.h>
#include <protocol.h>
#include <cpacket.h>

#include <cstring>

#if defined __CN_PAYLOAD_H
extern "C" {
#include <netinet/icmp6.h>
};
#endif //__CN_PAYLOAD_H

#include "ICMPv6Message.h"
#include "IPv6Datagram.h"
#include "IPInterfacePacket.h" //For Protocol IP_PROT_IPv6_ICMP

///Number of octets excluding the error datagram that is usually appended
///i.e. the Type|CODE|CHECKSUM|UNUSED/POINTER/MTU/OTHER as defined in RFC2463
const static int ICMPv6_FIXED_ERROR_SIZE = 8;
///Any ICMP type with MSBit set is an Informational ICMP message
const static int ICMPv6_INFO_MESSAGES = 128;


// ICMPv6Message
ICMPv6Message::ICMPv6Message(const ICMPv6Type otype, const ICMPv6Code ocode,
                             IPv6Datagram* errorPdu, size_t optInfo)
  :cMessage("", IP_PROT_IPv6_ICMP), _type(otype), _code(ocode), _checksum(0), _opt_info(optInfo)
//Added PR_ICMP to protocol.h of omnetpp
{
  //All setLength calls are in units of octects unless otherwise noted.
  setLength(ICMPv6_FIXED_ERROR_SIZE);
  setKind(_type);
  if (errorPdu)
  {
    encapsulate(errorPdu);
    setName(errorPdu->name());
  }
}

ICMPv6Message::ICMPv6Message(const ICMPv6Message& src)
{
  setName(src.name());
  operator= (src);
}

ICMPv6Message::~ICMPv6Message()
{}

ICMPv6Message& ICMPv6Message::operator=(const ICMPv6Message& rhs)
{
  if (this != &rhs)
  {
    cMessage::operator=(rhs);
    _checksum = rhs._checksum;
    _opt_info = rhs._opt_info;
    _type = rhs._type;
    _code = rhs._code;
  }

  return* this;
}

#if defined __CN_PAYLOAD_H
struct network_payload *ICMPv6Message::networkOrder() const{
   unsigned char data[ETH_FRAME_LEN];
   int icmphdrlen = 0;
   struct network_payload *payload = NULL;
   struct network_payload *packet;
   cMessage *enc = NULL;


   ev << "ICMPv6Message::networkOrder()" << endl;

   memset(data, 0 , sizeof(struct icmp6_hdr));

   icmphdrlen = sizeof(struct icmp6_hdr);
   ((struct icmp6_hdr *)data)->icmp6_type = _type;
   ((struct icmp6_hdr *)data)->icmp6_code = _code;
   // leave checksum as 0 until IPv6 header applied.

   switch (_type){
      case ICMPv6_TIME_EXCEEDED:
      case ICMPv6_DESTINATION_UNREACHABLE:
         // encapsulated message:
         // add data after icmp6_data32[0] (which is zero)
         // must not exceed (min IPv6 MTU) calculate cksum after trunc
         //enc = encapsulatedMsg();
         icmphdrlen = sizeof(struct icmp6_hdr); // + data??
         break;
      case ICMPv6_PACKET_TOO_BIG:
         // icmp6_mtu = mtu of next hop link ( <= 1500 on ethernet)
         // encapsulated message:
         // add data after icmp6_data32[0] (mtu)
         // must not exceed (min IPv6 MTU) calculate cksum after trunc
         //enc = encapsulatedMsg();
         icmphdrlen = sizeof(struct icmp6_hdr); // + data??
         break;
      case ICMPv6_PARAMETER_PROBLEM:
         // icmp6_pptr = pointer offset in octets
         // encapsulated message:
         // add data after icmp6_pptr (pointer)
         // must not exceed (min IPv6 MTU) calculate cksum after trunc
         //enc = encapsulatedMsg();
         icmphdrlen = sizeof(struct icmp6_hdr); // + data??
         break;
      case ICMPv6_ECHO_REQUEST:
         // icmp6_pptr = pointer offset in octets
         // Encapsulated data....???
         // add data after icmp6_id and icmp6_seq (after icmp6_data32[0]);
         //enc = encapsulatedMsg();
         icmphdrlen = sizeof(struct icmp6_hdr); // + data??
         break;
      case ICMPv6_ECHO_REPLY:
         // icmp6_pptr = pointer offset in octets
         // Encapsulated data....from echo request message
         // must be same as echo request... (fragments?? )
         // add data after icmp6_id and icmp6_seq (after icmp6_data32[0]);
         //enc = encapsulatedMsg();
         icmphdrlen = sizeof(struct icmp6_hdr); // + data??
         break;
      case ICMPv6_ROUTER_SOL:
         // ND options.  watch out for alignment.
         // add option data after icmp6_data32[0] (which is zero)
#if 0
struct nd_router_solicit      /* router solicitation */
  {
    struct icmp6_hdr  nd_rs_hdr;
    /* could be followed by options */
  };
#endif
         icmphdrlen = sizeof(struct nd_router_solicit); // + options ??
   ev << "ICMP6 hdrlen" << icmphdrlen << endl;
         break;
      case ICMPv6_NEIGHBOUR_SOL:
         // ND options.  watch out for alignment.
#if 0
struct nd_neighbor_solicit    /* neighbor solicitation */
  {
    struct icmp6_hdr  nd_ns_hdr;
    struct in6_addr   nd_ns_target; /* target address */
    /* could be followed by options */
  };

#define nd_ns_type               nd_ns_hdr.icmp6_type
#define nd_ns_code               nd_ns_hdr.icmp6_code
#define nd_ns_cksum              nd_ns_hdr.icmp6_cksum
#define nd_ns_reserved           nd_ns_hdr.icmp6_data32[0]
#endif
         // target address option - 128 bits after icmp6_data32[0]
         // add option data after target address
         icmphdrlen = sizeof(struct nd_neighbor_solicit); // + options ??
         break;
      case ICMPv6_REDIRECT:
          // add option data after dst address
#if 0
struct nd_redirect            /* redirect */
  {
    struct icmp6_hdr  nd_rd_hdr;
    struct in6_addr   nd_rd_target; /* target address */
    struct in6_addr   nd_rd_dst;    /* destination address */
    /* could be followed by options */
  };

#define nd_rd_type               nd_rd_hdr.icmp6_type
#define nd_rd_code               nd_rd_hdr.icmp6_code
#define nd_rd_cksum              nd_rd_hdr.icmp6_cksum
#define nd_rd_reserved           nd_rd_hdr.icmp6_data32[0]
#endif
        icmphdrlen = sizeof(struct nd_redirect); // + options ??
        break;
      case ICMPv6_ROUTER_AD:
         // ND options.  watch out for alignment.
#if 0
struct nd_router_advert       /* router advertisement */
  {
    struct icmp6_hdr  nd_ra_hdr;
    uint32_t   nd_ra_reachable;   /* reachable time */
    uint32_t   nd_ra_retransmit;  /* retransmit timer */
    /* could be followed by options */
  };
#define nd_ra_type               nd_ra_hdr.icmp6_type
#define nd_ra_code               nd_ra_hdr.icmp6_code
#define nd_ra_cksum              nd_ra_hdr.icmp6_cksum
#define nd_ra_curhoplimit        nd_ra_hdr.icmp6_data8[0]
#define nd_ra_flags_reserved     nd_ra_hdr.icmp6_data8[1]
#define ND_RA_FLAG_MANAGED       0x80
#define ND_RA_FLAG_OTHER         0x40
#define ND_RA_FLAG_HOME_AGENT    0x20
#define nd_ra_router_lifetime    nd_ra_hdr.icmp6_data16[1]
#endif
         // use ra_header....
         // add options after retrans timer.
         icmphdrlen = sizeof(struct nd_router_advert); // + options ??
         break;
      case ICMPv6_NEIGHBOUR_AD:
         // ND options.  watch out for alignment.
#if 0
struct nd_neighbor_advert     /* neighbor advertisement */
  {
    struct icmp6_hdr  nd_na_hdr;
    struct in6_addr   nd_na_target; /* target address */
    /* could be followed by options */
  };
 nd_na_flags_reserved     nd_na_hdr.icmp6_data32[0]
 ND_NA_FLAG_ROUTER
 ND_NA_FLAG_SOLICITED
 ND_NA_FLAG_OVERRIDE
#endif
         // add option data after icmp6_data32[0] (which is zero)
         icmphdrlen = sizeof(struct nd_neighbor_advert); // + options ??
   ev << "ICMP6 hdrlen" << icmphdrlen << endl;
         break;
      case ICMPv6_UNSPECIFIED:
      default:
         ev << "unknown ICMP type : " <<_type << endl;
         return NULL;
         break;
   }
   if(! (packet = new struct network_payload(ETH_FRAME_LEN , 0))){
       return NULL;
   }
   packet->set_data(data, icmphdrlen);
      // BEWARE that encapsulated packets for param problem etc
      // may not be replayed if they have no decoding instance...

   ev << "ICMP6 hdrlen" << icmphdrlen << endl;
   ev << "ICMP6 len " << packet->len() << endl;
   if(enc && (payload = enc->networkOrder())){
      packet->set_payload(payload, payload->len());
   }
   ev << "ICMP6 len " << packet->len() << endl;
   return packet;
}


/// inspired by linux/net/ipv6/ndisc.c
unsigned short ICMPv6Message::networkCheckSum(unsigned char*icmpmsg,
         struct in6_addr *src, struct in6_addr *dest, int icmplen) const
{
//#ifdef ULLONG_MAX
    unsigned long long workspace = 0;
    struct icmp6_hdr tmphdr;
    int i;
    memcpy( &tmphdr,(struct icmp6_hdr *)icmpmsg, sizeof(struct icmp6_hdr));
    tmphdr.icmp6_cksum = 0;

    // calculate pseudo ipv6  hdr checksum

    for( i = 0; i < 4; i++){
        workspace += src->s6_addr32[i];
        workspace += dest->s6_addr32[i];
    }
    workspace += htonl(icmplen);
    workspace += htonl(IPPROTO_ICMPV6);

    // calculate pseudo icmpv6 hdr checksum
    workspace += ((uint32_t *) &tmphdr )[0];
    workspace += tmphdr.icmp6_data32[0];

    // calculate across rest of icmpv6 message

    //Assuming end on 32 bit boundary
    for( i = sizeof(struct icmp6_hdr); i < icmplen; i+=4){
          workspace += *((uint32_t *)&(icmpmsg[i]));
    }
    // merge down 64 bits to 32 bits.
    workspace = (workspace & 0xffffffff) + ((workspace >> 32) & 0xfffffff);
    workspace = (workspace & 0xffffffff) + ((workspace >> 32) & 0xfffffff);

    // merge down 32 bits to 16 bits
    workspace = (workspace & 0xffff) + ((workspace >> 16) & 0xffff);
    workspace = (workspace & 0xffff) + ((workspace >> 16) & 0xffff);
    // we have preserved all the bits.
    return ~((unsigned short) workspace);
}
#endif //defined __CN_PAYLOAD_H
/**
 */
bool ICMPv6Message::operator==(const ICMPv6Message& rhs) const
{
  if (*this == rhs)
    return true;

  return (_type == rhs._type) && (_code == rhs._code) && (_checksum == rhs._checksum)
    && (_opt_info == rhs._opt_info);

}

std::string ICMPv6Message::info()
{
  return std::string();
}

/* XXX not strictly necessary -- removed to reduce complexity  --AV
void ICMPv6Message::encapsulate(IPv6Datagram* errorPdu)
{
  cPacket::encapsulate(errorPdu);
  setName(errorPdu->name());
}

IPv6Datagram *ICMPv6Message::decapsulate()
{
  return static_cast<IPv6Datagram*> (cPacket::decapsulate());
}

IPv6Datagram *ICMPv6Message::encapsulatedMsg() const
{
  return static_cast<IPv6Datagram *> (cPacket::encapsulatedMsg());
}
*/

bool ICMPv6Message::isErrorMessage() const
{
  return static_cast<int> (_type) < ICMPv6_INFO_MESSAGES?true:false;
}





/**
   Construct an ICMP echo request/reply

   @param id is the identifier for this ping packet
   @param seq is the sequence number for this ping packet
   @param request is true if this packet is a ICMPEchoRequest otherwise ICMPEchoReply
 */
ICMPv6Echo::ICMPv6Echo(int id, int seq, bool request)
  :ICMPv6Message(request?ICMPv6_ECHO_REQUEST:ICMPv6_ECHO_REPLY)
{
  setIdentifier(id);
  setSeqNo(seq);
}

// ICMPv6Echo
ICMPv6Echo::ICMPv6Echo(const ICMPv6Echo& src)
  :ICMPv6Message(src)
{
      //setName(src.name());
      //operator=( src );
}

ICMPv6Echo& ICMPv6Echo::operator=(const ICMPv6Echo& rhs )
{
  ICMPv6Message::operator=(rhs);
      //Identifier and sequence number are set when _opt_info is copied in ICMPv6Message
  return* this;
}

std::string ICMPv6Echo::info()
{
  return std::string();
}


#if defined USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>

class IPv6Address;

/**
 * @class ICMPMessageTest
 * @ingroup TestCases
 * @brief Test ICMPv6 messages
 */

class ICMPMessageTest: public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( ICMPMessageTest );
  CPPUNIT_TEST( testEqualCtorDtor );
  CPPUNIT_TEST_SUITE_END();
public:

  ICMPMessageTest();

  void testEqualCtorDtor();

  void setUp();
  void tearDown();

private:
  IPv6Address* ip_addr1;
  IPv6Address* ip_addr2;

  ICMPv6Message a1, a2;
  ICMPv6Message *b, *c;
  IPv6Datagram* new1;
  IPv6Datagram* new2;

};

CPPUNIT_TEST_SUITE_REGISTRATION( ICMPMessageTest );

#include "IPv6Datagram.h"
#include "IPv6Address.h"

static ipv6_addr addr1 = {0x12345678,0xabcdef00,0x1234,0};
static unsigned int int_addra2[] = {0,0xabcdffff,0, 0x01010101};

ICMPMessageTest::ICMPMessageTest()
  :TestFixture(), ip_addr1(0), ip_addr2(0),
  a1(ICMPv6_DESTINATION_UNREACHABLE), a2(a1)
{}

void ICMPMessageTest::testEqualCtorDtor()
{

  CPPUNIT_ASSERT(a1 == a2);

  b = new ICMPv6Message(a1);
  c = new ICMPv6Message(a2);
  CPPUNIT_ASSERT(*b==*c);

  new1->encapsulate(b);
  CPPUNIT_ASSERT(*b == *(dynamic_cast<ICMPv6Message*> (new1->encapsulatedMsg())));

  CPPUNIT_ASSERT(new1->transportProtocol() == IP_PROT_IPv6_ICMP);
  new2 = new1->dup();

  CPPUNIT_ASSERT(*c == *(dynamic_cast<ICMPv6Message*> (new2->encapsulatedMsg())));
  delete new2;
}

void ICMPMessageTest::setUp()
{
  ip_addr1 = new IPv6Address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd/10");
  ip_addr2 = new IPv6Address("ef00:abcd:ef00:ffff:0:0f0f:0:0/10");

  new1 = new IPv6Datagram(addr1, IPv6Address(int_addra2));
}

void ICMPMessageTest::tearDown()
{
  static int count = 0;
  count++;

  delete new1;

  delete ip_addr1;
  delete ip_addr2;

}
#endif //defined USE_CPPUNIT

