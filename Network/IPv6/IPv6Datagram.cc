// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/IPv6Datagram.cc,v 1.6 2005/02/10 05:59:32 andras Exp $
//
// Copyright (C) 2001, 2004 CTIE, Monash University
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
    @file IPv6Datagram.cc
    @brief Implementation of IPv6Datagram class
    @author Johnny Lai
    @date 19.8.01

*/


#include <omnetpp.h>

#include <cassert>
#include <sstream>
#include <boost/cast.hpp>

#include <string>

#include "IPv6Datagram.h"
#include "IPv6Headers.h"
#include "ICMPv6Message.h"
#include "IPInterfacePacket.h" //For ProtocolID values
#include "HdrExtProc.h"
#include "HdrExtFragProc.h"
#include "HdrExtRteProc.h"
#include "HdrExtDestProc.h"

#if defined __CN_PAYLOAD_H
extern "C"
{
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
};
#endif //__CN_PAYLOAD_H


using boost::polymorphic_downcast;


static const ipv6_hdr IPV6_INITIAL_HDR =
{
    0x60000000,    //version 6, traffic class of 0, flow label of 0
    0,             //payload of 0
    59,            //No next header yet
    0,             //Hop Limit set to uninitialised
    IPv6_ADDR_UNSPECIFIED,
    IPv6_ADDR_UNSPECIFIED
};

/**
   Protocol is set to PR_IPV6 to mean IPv6 packet (used to discriminate
   between v4 & v6)
*/
IPv6Datagram::IPv6Datagram(const ipv6_addr& src, const ipv6_addr& dest,
                           cPacket* pdu, const char* name)
  :/*cPacket(NULL, PR_IPV6, 0),*/ header(IPV6_INITIAL_HDR), route_hdr(0),
                                  frag_hdr(0), dest_hdr(0)
{
  cPacket::setProtocol( PR_IPV6);
  setLength(IPv6_HEADER_LENGTH);
  //This signifies a packet that has not arrived at a host
  setInputPort(-1);
  if (src != IPv6_ADDR_UNSPECIFIED)
    header.src_addr = src;
  if (dest != IPv6_ADDR_UNSPECIFIED)
    header.dest_addr = dest;

  if (pdu != 0)
  {
    encapsulate(pdu);
    setName(pdu->name());
  }
}

/**
The accessory objects are invoked from the source object to copy the
headers using the list of accessory objects.  If they don't exist then
this object will do full parsing of the objects.  This is about the
most flexible approach I can think of without incurring great
performance penalities.

@note If duplicating a lot of datagrams you should first call
parseAllHeaders before using dup/copy ctor and continually use that
same instance to dup multiple times.  This is create all the accessory
objects if they have not been created as variable sized ext headers
are only understood by them on how much memory to allocate.

 */

IPv6Datagram::IPv6Datagram(const IPv6Datagram& src)
  :route_hdr(0), frag_hdr(0), dest_hdr(0)
{
  setName(src.name());
  operator=(src);
}

IPv6Datagram::~IPv6Datagram()
{
//Don't need to do this as a cArray once it takes possession will delete this itself
//  delete frag_hdr;
//  delete route_hdr;

  for (EHI it = ext_hdrs.begin(); it != ext_hdrs.end(); it++)
    delete *it;

  ext_hdrs.clear();

  //for each element in ext_hdrs find out what type it is and have
  //a massive switch statement to do the deletion as appropriate for the
  //variable length fields.
#ifdef EXCESSIVE_DUP
  if (strcmp("encapsulating", name())==0||strcmp("encapsulated", name())==0)
    cerr <<" I've been deleted "<<name()<<endl;
#endif  //EXCESSIVE_DUP

}

bool IPv6Datagram::operator==(const IPv6Datagram& rhs) const
{
  if (header == rhs.header)
  {
    if (ext_hdrs.size() == rhs.ext_hdrs.size())
    {
      EHI it, rit;
      for (it = ext_hdrs.begin(), rit = rhs.ext_hdrs.begin();
           it != ext_hdrs.end(); rit++, it++)
        (*it)->operator==(*(*it));
      return true;
    }
  }
  return false;
}


const IPv6Datagram& IPv6Datagram::operator=(const IPv6Datagram& rhs)
{
  //Check for assignment to self
  if (&rhs == this)
    return *this;

  // XXX --AV  IPDatagram::operator=(rhs);
  cPacket::operator=(rhs);

  header = rhs.header;
  for (EHI it = ext_hdrs.begin(); it != ext_hdrs.end(); it++)
    delete *it;
  ext_hdrs.clear();
  //As long as each wrapper object defines suitable copy ctor should be fine
  for (EHI it = rhs.ext_hdrs.begin(); it != rhs.ext_hdrs.end(); it++)
    ext_hdrs.push_back((*it)->dup());

  return *this;
}

/**
   Return the next ext hdr wrapper after fromHdrExt.

   @param fromHdrExt is the preceding extension If fromHdrExt is 0 then the
   first extension header is returned.
   @return the next hdr ext wrapper following fromHdrExt or 0 if no extension
   headers proceed.
*/

HdrExtProc* IPv6Datagram::getNextHeader(const HdrExtProc* fromHdrExt) const
{
  if (!ext_hdrs.empty())
  {
    if ( 0 == fromHdrExt)
      return *ext_hdrs.begin();

    EHI it;
    for (it = ext_hdrs.begin(); it != ext_hdrs.end(); it++)
    {
      if (*it == fromHdrExt)
      {
        if (++it != ext_hdrs.end())
          return *(it);
        else
          return 0;
      }
    }
  }
  return 0;
}

std::string IPv6Datagram::info()
{
  ostringstream os;
  os << "Source=" << header.src_addr
     << " Dest=" << header.dest_addr;

  //For some reason the buf is filled with escape characters after this point
  //However the stdoutput from os is fine.
  //int ev_limit = os.str().size();

  os << " len="<<length()<<" HL=" <<dec<<hopLimit()
     << " prot="<<dec<<transportProtocol();

  //Can't use this as they're all pointers
  //std::copy(ext_hdrs.begin(), ext_hdrs.end(), ostream_iterator<HdrExtProc*>(os));
  EHI it;
  for (it = ext_hdrs.begin(); it != ext_hdrs.end(); it++)
    (*it)->operator<<(os);

#if defined TESTIPv6
#ifdef EXTRA_VERBOSE
  cerr << os.str()<<endl;
#endif //VERBOSE
  ev<<os.str();
#endif //TESTIPv6
  return os.str();
}

// output function for datagram
void IPv6Datagram::writeContents( ostream& os )
{
    os << "IPv6 Datagram:"
       << "\nSender: " << senderModuleId()
       // << "\nBit length: " << length()
       << "Byte len: " << totalLength()
       << "Ext header len:" << extensionLength()
       << "\nTransport Prot: " << (int)transportProtocol()
       << "\n";
//Should use writeTo of ext_hdr and info of contained objects?
//    ext_hdrs.writeContents(os);

}

HdrExtRteProc* IPv6Datagram::acquireRoutingInterface()
{
  if (!route_hdr)
  {
    HdrExtProc* proc = findHeader(EXTHDR_ROUTING);
    if (proc != 0)
      return route_hdr = polymorphic_downcast<HdrExtRteProc*> (proc);
  }

  return route_hdr = static_cast<HdrExtRteProc*> (addExtHdr(EXTHDR_ROUTING));
}

HdrExtFragProc* IPv6Datagram::acquireFragInterface()
{
  if (!frag_hdr)
  {
    HdrExtProc* proc = findHeader(EXTHDR_FRAGMENT);
    if (proc != 0)
      return frag_hdr = polymorphic_downcast<HdrExtFragProc*> (proc);
  }

  return frag_hdr = static_cast<HdrExtFragProc*>(addExtHdr(EXTHDR_FRAGMENT));
}

HdrExtDestProc* IPv6Datagram::acquireDestInterface()
{
  if (!dest_hdr)
  {
    HdrExtProc* proc = findHeader(EXTHDR_DEST);
    if (proc != 0)
      return dest_hdr = polymorphic_downcast<HdrExtDestProc*> (proc);
  }

  return dest_hdr = static_cast<HdrExtDestProc*>(addExtHdr(EXTHDR_DEST));
}

IPProtocolId IPv6Datagram::transportProtocol() const
{
  if (ext_hdrs.empty())
    return (IPProtocolId) header.next_header;
  else
    return (IPProtocolId) (*(ext_hdrs.rbegin()))->nextHeader();
}

void IPv6Datagram::setTransportProtocol(const IPProtocolId& prot)
{
  //        ISVALID_IPV6_PROTOCOLFIELDID(prot);
  if (ext_hdrs.empty())
    header.next_header = prot;
  else
    (*(ext_hdrs.rbegin()))->setNextHeader(prot);
}



/**
   Encapsulate an upper layer protocol packet.
   @arg transportPacket to encapsulate.  It is possible that transport packet
   encapsulate other PDUs.
*/
void IPv6Datagram::encapsulate(cPacket* transportPacket)
{
  cPacket::encapsulate(transportPacket);
  if (transportPacket->protocol() == PR_IPV6)
    setTransportProtocol(IP_PROT_IPv6);
  else
    setTransportProtocol((IPProtocolId)transportPacket->protocol());
  setName(transportPacket->name());
}

cPacket* IPv6Datagram::decapsulate()
{
  //May need to do special things if ICMPv6 will not be encapsulated
  //as sub class of CPacket or current IPv4 ICMP impl.
  return (cPacket *)(cPacket::decapsulate());
}


/*
  void IPv6Datagram::setFragmentOffset(int offset);
  {
  assert(offset <= 1<<13 - 1);
  int pos = parseHeader(NEXTHDR_FRAGMENT);
  if (pos >= 0)
  {
  //Only Top 13 bits of 16 bit frag_off is where offset is stored
  ext_hdrs[pos].frag_hdr.frag_off = offset << 3;
  }
  else //Create and add fragmentation header after Routing, Dest opts,
  //Hop-by-Hop in that order
  {
  bool insertAfter = false;
  ext_hdrs.insert
  if (ext_hdrs.size() > 0)
  {
  int pos = parseHeader(NEXTHDR_ROUTING);
  if (pos >= 0)
  {
  insertAfter = true;
  }
  }

  }
  }
*/

#ifdef __CN_PAYLOAD_H

struct network_payload *IPv6Datagram::networkOrder() const
{
   struct network_payload *packet;
   cPacket *enc;
   //ipv6_addr src;
   in6_addr  srcnw, destnw;

   struct network_payload *payload = NULL;
   unsigned char data[ETH_FRAME_LEN];

   ev << "IPv6Datagram::networkOrder()" << endl;

   memset(data, 0 , sizeof(struct ip6_hdr));

   srcAddress().in6_nwaddr(&srcnw);
   destAddress().in6_nwaddr(&destnw);

   memcpy(&(((struct ip6_hdr*)data)->ip6_src),&srcnw, sizeof(struct in6_addr));
   memcpy(&(((struct ip6_hdr*)data)->ip6_dst),&destnw,sizeof(struct in6_addr));
   ((struct ip6_hdr*)data)->ip6_hlim = (unsigned char) hopLimit();
   // add the protocol version without touching the flow header;
   ((struct ip6_hdr*)data)->ip6_vfc  &= 0x0f;
   ((struct ip6_hdr*)data)->ip6_vfc  |= (( 6 ) << 4);
   // todo: handle extension headers:
   // currently assume separate headers for each.
   // in model, extension headers are part of IPv6....

   if(! (packet = new struct network_payload(ETH_FRAME_LEN, 0))){
      ev <<"IPv6Datagram::networkOrder() unable to create packet" << endl;
      return NULL;
   }
   if( ( enc = static_cast<cPacket*> (encapsulatedMsg())) &&
       (payload = enc->networkOrder())){
      unsigned char ip6proto = 0;
      ev << "protocol number (Sim) " << ((cPacket *)enc)->protocol() << endl;
      switch(((cPacket *)enc)->protocol()){
         case IPPROTO_IP :
         case IPPROTO_TCP:
         case IPPROTO_UDP :
         case IPPROTO_IPV6 :
         case IPPROTO_ROUTING:
         case IPPROTO_FRAGMENT:
         case IPPROTO_RSVP :
         case IPPROTO_GRE:
         case IPPROTO_ESP:
         case IPPROTO_AH :
         case IPPROTO_NONE :
         case IPPROTO_DSTOPTS :
         case IPPROTO_MTP:
         case IPPROTO_ENCAP :
         case IPPROTO_PIM :
         case IPPROTO_COMP :
         case IPPROTO_RAW:
            ev << "protocol number (IANA) " <<
                    ((cPacket *)enc)->protocol() << endl;
            ip6proto = ((cPacket *)enc)->protocol();
            break;
         case IPPROTO_ICMPV6 :
            ev << "protocol number (IANA) " <<
                    ((cPacket *)enc)->protocol() << endl;
            // watch out for truncation of the data in the icmp message
            // TODO Fix this.
            ((struct icmp6_hdr *)payload->get_start())->icmp6_cksum =
             ((ICMPv6Message *)enc)->networkCheckSum(payload->get_start(),
                                          &srcnw,&destnw,payload->len()/*??*/);
            ip6proto = ((cPacket *)enc)->protocol();
            break;
         case  PR_IP:
            ip6proto = IPPROTO_IP;
            break;
         case PR_TCP:
            ip6proto = IPPROTO_TCP;
            break;
         case PR_UDP:
            ip6proto = IPPROTO_UDP;
            break;
         case PR_IPV6:
            ip6proto = IPPROTO_IPV6;
            break;
         case PR_ICMP:
            ip6proto = IPPROTO_ICMPV6;
            break;
         default:
             //ip6proto = findHeader();
             ip6proto = IPPROTO_NONE;
             break;
      }
      ((struct ip6_hdr*)data)->ip6_nxt  = ip6proto;
      ((struct ip6_hdr*)data)->ip6_plen  = htons(payload->len() ); // octets?
      ev<< "IPv6 payload len  " << payload->len()  << endl;
      ev<< "IPv6 packet lenpe " << packet->len() << endl ;
      packet->set_data(data,sizeof(struct ip6_hdr));
      ev<< "IPv6 packet lense " << packet->len() << endl ;
      packet->set_payload(payload, payload->len());
      //packet->set_
   }
   else{
      ev <<"IPv6Datagram::networkOrder() no encaps message" << endl;
      ((struct ip6_hdr*)data)->ip6_plen = 0;
      ((struct ip6_hdr*)data)->ip6_nxt  = IPPROTO_NONE;
      ev<< "IPv6 packet lenpn " << packet->len() << endl ;
      packet->set_data(data, sizeof(struct ip6_hdr) );
      ev<< "IPv6 packet lensn " << packet->len() << endl ;
   }
   ev<< "IPv6 packet len " << packet->len() << endl ;

   return packet;


}
#endif /* __CN_PAYLOAD_H */
/// @name Private Functions
//@{
/**
      Add an extension header to this packet.  This will be appended
      to the end of the current ext hdrs list.  Length of datagram is
      updated automatically.

      @param hdr_type Identify what ext_hdr type you are appending
      @return The wrapper object of the created ext hdr.
*/
HdrExtProc* IPv6Datagram::addExtHdr(const IPv6ExtHeader& hdr_type)
{
  HdrExtProc* proc = 0;

  switch(hdr_type)
  {
    case EXTHDR_HOP:
      break;
    case EXTHDR_DEST:
      proc = new HdrExtDestProc();
      break;
    case EXTHDR_ROUTING:
      proc = new HdrExtRteProc();
      break;
    case EXTHDR_FRAGMENT:
      ev.printf("(%s)%s: Sorry Fragment is not properly implemented or tested",
                className(), fullName());
      //Satisfy unit test for now
      proc = new HdrExtFragProc();
      break;
      //Not supported i.e. is ignored completely. (log warning msg)
    case EXTHDR_ESP: case EXTHDR_AUTH:
      opp_error("(%s)%s: Sorry ESP or AUTH ext hdr not supported yet",
                className(), fullName());
      break;
    default:
      //Should not be adding anything else besides an ext hdr
      opp_error("(%s)%s: Unrecoverable error: Unknown ext hdr type: %d",
                className(), fullName(), (int)hdr_type);
      assert(false);
      break;
    }

  if (proc != 0)
  {
    IPProtocolId prot = transportProtocol();

    if (ext_hdrs.empty())
      header.next_header = hdr_type;
    else
      (*(ext_hdrs.rbegin()))->setNextHeader(hdr_type);

    ext_hdrs.push_back(proc);
    ext_hdr_len = proc -> cumul_len(*this);
        //Extension headers + encapsulated length of upper layer pdu
    setPayloadLength(ext_hdr_len + length());
    setTransportProtocol(prot);
  }

  return proc;
}


HdrExtProc* IPv6Datagram::findHeader(const IPv6ExtHeader& next_hdr) const
{
  EHI it;

  if (header.next_header == next_hdr && !ext_hdrs.empty())
    return *ext_hdrs.begin();


  if (!ext_hdrs.empty())
    for (it = ext_hdrs.begin(); it != ext_hdrs.end(); it++)
      if ((int)(*it)->nextHeader() == (int)next_hdr)
      {
        return *(++it);
      }

  return 0;

//   switch (next_hdr)
//   {
//     case NEXTHDR_HOP:   // Hop-by-hop option header.
//     case NEXTHDR_ROUTING:        // Routing header.
//     case NEXTHDR_FRAGMENT:    // Fragmentation/reassembly header.
//     case NEXTHDR_DEST:     // Destination options header.

//       //Not supported i.e. is ignored completely. (log warning msg)
//     case NEXTHDR_ESP:    // Encapsulating security payload.
//     case NEXTHDR_AUTH: // Authentication header.
//       //Make sure that if the next header is an extension
//       //header that it exists in ext_hdrs.
//       break;
//     default:
//       //Every other next header value should not be in the
//       //extension headers list
//       break;
//   }
}

/**
   Update pos, and links to next procHdr.
   Sorts headers according to preferred order of RFC 2460
   not implemented yet as clients should add headers in this order anyway
*/
// void IPv6Datagram::reLinkProcHdrs() const
// {
//   //Do custom sorting of proc_hdrs according to positions
//   //specified inside object.
// }
//@}


std::ostream& operator<<(std::ostream& os, const IPv6Datagram& pdu)
{
  os << "Source="<<pdu.header.src_addr
     <<" Dest=" << pdu.header.dest_addr;
  os <<" len="<<pdu.length()<<" HL=" <<dec<<pdu.hopLimit()
     <<" prot="<<dec<<pdu.transportProtocol();

  //Should use writeTo of ext_hdr and info of contained objects?
  //ext_hdrs.writeContents(os);
  //Can't use this as they're all pointers
  //std::copy(ext_hdrs.begin(), ext_hdrs.end(), ostream_iterator<HdrExtProc*>(os));
  IPv6Datagram::EHI it;
  for (it = pdu.ext_hdrs.begin(); it != pdu.ext_hdrs.end(); it++)
    (*it)->operator<<(os);

  return os;

}


#if defined USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>

class IPv6Address;

/**
 * @class DatagramTest
 * @ingroup TestCases
 * @brief Tests IPv6Datagram and related extension headers
 *
 * Addition and removal of extensions to the IPv6Datagram only routing headers
 * currently.
 *
 *  Untested - the padding functionality of Hop-by-Hop options payload/hdr opt
 * length calculations and checksums are correct against precalculated
 * values. (Unnecessary?))
 *
 */

class DatagramTest: public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( DatagramTest );
  CPPUNIT_TEST( testEqualCtorDtor );
  CPPUNIT_TEST( testHeaders );
  CPPUNIT_TEST( testEncapsulation );
  CPPUNIT_TEST( testIPv6Header );
  CPPUNIT_TEST_SUITE_END();
public:

  DatagramTest();


  void testEqualCtorDtor();

  void testHeaders();

  void testEncapsulation();

#ifdef USE_MOBILITY
  void testBindingUpdateAndHAOption();
#endif // USE_MOBILITY

  void testIPv6Header();

  void setUp();
  void tearDown();

private:
  IPv6Address* ip_addr1;
  IPv6Address* ip_addr2;

  IPv6Datagram a1, a2, b, c;
  IPv6Datagram* new1;
  IPv6Datagram* new2;

};

CPPUNIT_TEST_SUITE_REGISTRATION( DatagramTest );

#include "ICMPv6NDMessage.h" //NDHOPLIMIT
#include "IPv6Address.h"

#ifdef USE_MOBILITY
#include "MIPv6MobilityHeaders.h"
#include "MIPv6DestOptMessages.h"
#endif// USE_MOBILITY

ipv6_addr addr1 = {0x12345678,0xabcdef00,0x1234,0};
unsigned int int_addra1[] = {0x12345678,0xabcdef00,0x1234,0};
unsigned int int_addra2[] = {0,0xabcdffff,0, 0x01010101};
unsigned int *int_addr1 = int_addra1;
unsigned int *int_addr2 = int_addra2;

DatagramTest::DatagramTest()
  :TestFixture(), ip_addr1(0), ip_addr2(0)
      , a1(IPv6Address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), IPv6Address("ef00:abcd:ef00:ffff:0:0f0f:0:0")), a2(a1)
, b(IPv6Address(int_addr2), IPv6Address(addr1)), c(IPv6Address(int_addr2), IPv6Address(int_addr1))
{}

void DatagramTest::testEqualCtorDtor()
{

  ipv6_addr test_addr_string_initialise =
    {
0xabcdabcd,0xabcdabcd,0xabcdabcd, 0xabcdabcd
    };

  //Test IPv6Address char* ctor and equals operator(ipv6_addr)
  CPPUNIT_ASSERT(!(ip_addr1->operator==(*ip_addr2)));
  CPPUNIT_ASSERT(*ip_addr1 == test_addr_string_initialise);

  //Test IPv6Address ipv6_addr ctor
  *ip_addr2 = test_addr_string_initialise;
  CPPUNIT_ASSERT((ip_addr2->operator==(*ip_addr1)));

  //test address struct == int* address
  CPPUNIT_ASSERT( addr1 == int_addr1);


  //Test copy construction
  CPPUNIT_ASSERT(a1 == a2);

  delete new1;
  new1 = 0;

  CPPUNIT_ASSERT(*new2 == a1);

  new1 = new2->dup();
  CPPUNIT_ASSERT(*new2 == *new1);
  delete new2;
  new2 = 0;

  CPPUNIT_ASSERT(a1 == *new1);


  CPPUNIT_ASSERT(b == c);
  b = a1;
  CPPUNIT_ASSERT ( b == a1);
  //Strange it didn't fail before.  Because well they should be the same as
  //neither a1 nor a2 are on lhs.
  CPPUNIT_ASSERT ( a1 == a2);

  new1->setHopLimit( IPv6NeighbourDiscovery::NDHOPLIMIT);
  IPv6Datagram* dup = new1->dup();
  CPPUNIT_ASSERT(dup->hopLimit() ==  IPv6NeighbourDiscovery::NDHOPLIMIT);

  delete dup;
}

/**
   Add/remove routes from Routing ext header and test them for illegal
   i.e. multicast address
*/
void DatagramTest::testHeaders()
{

  CPPUNIT_ASSERT(*new2 == *new1);
  new2->acquireFragInterface();
  CPPUNIT_ASSERT(a1 != *new2);
  *new1 = *new2; //Test cArray copy
  CPPUNIT_ASSERT(new1->ext_hdrs.size() > 0);

  delete new2;
  new2 = new IPv6Datagram(*new1);
  CPPUNIT_ASSERT(*new1==*new2);

  IPv6Datagram* dup = a1.dup();
  HdrExtRteProc* rp = dup->acquireRoutingInterface();
  HdrExtRte* rt = new HdrExtRte;
  rt -> addAddress(addr1);
  rt -> addAddress(IPv6Address(int_addr1));
  CPPUNIT_ASSERT(rt -> segmentsLeft() == 2);
  CPPUNIT_ASSERT(rt->address(1) == rt->address(2));
  CPPUNIT_ASSERT(rp->addRoutingHeader(rt));

  //processHeader tests the hop limit if its <= 1 it is rejected and ICMP hop
  //limit exceeded sent
  dup->setHopLimit(2);

  IPv6Datagram* dup2 = dup->dup();
  std::stringstream os, os2;
  os<< *dup<<endl;
  os2<< *dup2<<endl;
#if defined UNITTESTOUTPUT
  cout<<"dup is "<<os.str();
  cout<<"dup2 is "<<os2.str();
#endif
  CPPUNIT_ASSERT(os.str() == os2.str());

  CPPUNIT_ASSERT(*(dup2->ext_hdrs.begin()) != *(dup->ext_hdrs.begin()));

  delete dup2;
  delete dup;
}

void DatagramTest::testEncapsulation()
{
  IPv6Datagram* tunDgram = new IPv6Datagram(IPv6Address(int_addr1),
                                            IPv6Address(int_addr2), new2->dup());
  CPPUNIT_ASSERT((int)tunDgram->transportProtocol() == NEXTHDR_IPV6);

  //tunDgram->encapsulatedMsg()->setOwner(tunDgram);

//  HdrExtRteProc* rp = tunDgram->acquireRoutingInterface();
//  rp -> addAddress(addr1);
//  rp -> addAddress(IPv6Address(int_addr1));

  IPv6Datagram* dupTunDgram = tunDgram->dup();
  std::stringstream os, os2;
  os<< *tunDgram<<endl;
  os2<< *dupTunDgram<<endl;
  CPPUNIT_ASSERT(os.str() == os2.str());
#if defined UNITTESTOUTPUT
  cout << "tun dgram is "<<os.str();
  cout << "duplicated tun dgram is "<<os2.str();
#endif

  delete tunDgram;

  CPPUNIT_ASSERT((*(IPv6Datagram*)dupTunDgram->encapsulatedMsg()) == *new2);
  os <<"Duplicated tunnel dgram after tunDgram deleted "<< *dupTunDgram
     << " tunneled packet "<<(*(IPv6Datagram*)dupTunDgram->encapsulatedMsg())
     <<endl;
#if defined UNITTESTOUTPUT
  cout <<"Duplicated tunnel dgram after tunDgram deleted "<< *dupTunDgram
       << " tunneled packet "<<(*(IPv6Datagram*)dupTunDgram->encapsulatedMsg())
       <<endl;
#endif

  CPPUNIT_ASSERT(dupTunDgram->frag_hdr == 0 && dupTunDgram->route_hdr == 0);
  CPPUNIT_ASSERT(dupTunDgram->transportProtocol() == IP_PROT_IPv6);

  delete dupTunDgram;
}

#ifdef USE_MOBILITY
//Exercising interface of MIPv6MHBindingUpdate and default values
void DatagramTest::testBindingUpdateAndHAOption()
{
  bool flags = false;
  unsigned int seq = 0xF8FEFABB;
  unsigned int lifetime = 12345;

  using namespace MobileIPv6;

  MIPv6MHBindingUpdate* bu = new MIPv6MHBindingUpdate(flags, flags, flags, flags, seq, lifetime, addr1);

  IPv6Datagram* dgram = new IPv6Datagram(a2.srcAddress(), a2.destAddress(), bu);

  bu = boost::polymorphic_downcast<MIPv6MHBindingUpdate*>(dgram->encapsulatedMsg());
  CPPUNIT_ASSERT(bu != 0);

  CPPUNIT_ASSERT(bu->ack() == flags);
  CPPUNIT_ASSERT(bu->homereg() == flags);
  CPPUNIT_ASSERT(bu->saonly() == flags);
  CPPUNIT_ASSERT(bu->dad() == flags);
  CPPUNIT_ASSERT(bu->sequence() == seq);
  CPPUNIT_ASSERT(bu->expires() == lifetime);
  CPPUNIT_ASSERT(bu->ha() == addr1);

  HdrExtDestProc* destProc = dgram->acquireDestInterface();

  destProc->addOption(new MIPv6TLVOptHomeAddress(addr1));

  IPv6TLVOptionBase* destOpt = destProc->getOption(IPv6TLVOptionBase::MIPv6_HOME_ADDRESS_OPT);

  CPPUNIT_ASSERT(destOpt != 0);

  MIPv6TLVOptHomeAddress* haOpt = boost::polymorphic_downcast<MIPv6TLVOptHomeAddress*>(destOpt);

  CPPUNIT_ASSERT(haOpt->homeAddr() == addr1);

  IPv6Datagram* dgramCopy = dgram->dup();
  delete dgram;
  dgram = 0;

  bu = boost::polymorphic_downcast<MIPv6MHBindingUpdate*>(dgramCopy->encapsulatedMsg());
  CPPUNIT_ASSERT(bu != 0);

  CPPUNIT_ASSERT(bu->ack() == flags);
  CPPUNIT_ASSERT(bu->homereg() == flags);
  CPPUNIT_ASSERT(bu->saonly() == flags);
  CPPUNIT_ASSERT(bu->dad() == flags);
  CPPUNIT_ASSERT(bu->sequence() == seq);
  CPPUNIT_ASSERT(bu->expires() == lifetime);
  CPPUNIT_ASSERT(bu->ha() == addr1);

  destProc = boost::polymorphic_downcast<HdrExtDestProc*> (dgramCopy->findHeader(EXTHDR_DEST));
  CPPUNIT_ASSERT(destProc != 0);

  IPv6TLVOptionBase* tlvOpt = destProc->getOption(IPv6TLVOptionBase::MIPv6_HOME_ADDRESS_OPT);
  CPPUNIT_ASSERT(tlvOpt != 0);

  CPPUNIT_ASSERT(boost::polymorphic_downcast<MIPv6TLVOptHomeAddress*>(tlvOpt)->homeAddr() == addr1);


}
#endif // USE_MOBILITY

void DatagramTest::testIPv6Header()
{
  CPPUNIT_ASSERT(new1->version() == 6);
#if defined UNITTESTOUTPUT
  cerr<<"headerverflow="<<new1->header.ver_traffic_flow<<endl;

  cout<<"flow="<<new1->flowLabel()<<"traf class"<<new1->trafficClass()<< endl;
#endif
  unsigned int trafficClass = 251;
  new1->setTrafficClass(trafficClass);
#if defined UNITTESTOUTPUT
  cout<<"traf class"<<new1->trafficClass()<< endl;
#endif
  CPPUNIT_ASSERT(new1->trafficClass() == trafficClass);
  unsigned int flowLabel = 0xf89;
  new1->setFlowLabel(flowLabel);
#if defined UNITTESTOUTPUT
  cerr<<"headerverflow="<< new1->header.ver_traffic_flow<<endl;
  cerr<<"flow="<<new1->flowLabel()<<endl;
#endif
  CPPUNIT_ASSERT(new1->flowLabel()==flowLabel);
  CPPUNIT_ASSERT(new1->version() == 6);
  new1->setTrafficClass(trafficClass);
  trafficClass = 241;
  new1->setTrafficClass(trafficClass);
  flowLabel =0xcabcd;
  new1->setFlowLabel(flowLabel);
  CPPUNIT_ASSERT(new1->trafficClass() == trafficClass);
  CPPUNIT_ASSERT(new1->flowLabel()==flowLabel);
  CPPUNIT_ASSERT(new1->version() == 6);
}

void DatagramTest::setUp()
{
  ip_addr1 = new IPv6Address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd/10");
  ip_addr2 = new IPv6Address("ef00:abcd:ef00:ffff:0:0f0f:0:0/10");

  new1 = new IPv6Datagram(a1);
  new2 = new IPv6Datagram(*new1);
}

void DatagramTest::tearDown()
{
  static int count = 0;
  count++;

  delete new1;
  delete new2;

  delete ip_addr1;
  delete ip_addr2;

}
#endif //defined USE_CPPUNIT
