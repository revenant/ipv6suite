// -*- C++ -*-
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
   @file IPv6Datagram.h
   @brief Definition of class IPv6Datagram
   @author Johnny Lai
   @date 19.8.01
   @test see DatagramTest

*/

#ifndef IPV6DATAGRAM_H
#define IPV6DATAGRAM_H

#ifndef CASSERT
#define CASSERT
#include <cassert>
#endif //CASSERT

#ifndef LIST
#define LIST
#include <list>
#endif //LIST

#ifndef IPv6HEADERS_H
#include "IPv6Headers.h"
#endif //IPv6HEADERS_H

#ifndef IPv6_ADDR_H
#include "ipv6_addr.h"
#endif //IPv6_ADDR_H

#include "IPProtocolId_m.h"

/**
   Forward declarations
*/
class cPacket;
class HdrExtFragProc;
class HdrExtRteProc;
class HdrExtDestProc;
class HdrExtHopOptProc;
class HdrExtProc;

/**
   @class IPv6Datagram

   @brief represents both RFC IPv6 datagram for packets as they traverse through
   the ISO protocol stack and across physical medium.

   The datagram is represented as ext header structures and the upper
   layer payload in an encapsulated cPacket object.  As the structs
   themselves do not have any copy constructors, assignment operators and
   destructors, wrappers derived from HdrExtProc have been created to
   deal with that.

   Thus in the simulation we will deal strictly with classes and only
   output the raw IPv6 packet structure when sending it to Upper/Lower
   layers or when MPI/PVM netPack/netUnpack is required (unimplemented
   yet).  IP layer will regenerate the c++ classes from the raw data so
   we can deal with them and deallocate them when finished.
*/

// XXX was public IPDatagram, should be cMessage!!!! --AV
class IPv6Datagram: public cPacket, boost::equality_comparable<IPv6Datagram>
{
public:
  friend class HdrExtRteProc;
  friend class HdrExtFragProc;
  friend class HdrExtProc;
  friend class HdrExtDestProc;
  friend class DatagramTest;

  friend std::ostream& operator<<(std::ostream&, const IPv6Datagram& pdu);


  /** @name Acquire Interfaces
      Acquire Extension Header wrapper objects from these functions.
      Wrapper object may be created if they do not exist yet.
  */
  //@{
  HdrExtRteProc* acquireRoutingInterface();
  HdrExtFragProc* acquireFragInterface();
  HdrExtDestProc* acquireDestInterface();
  HdrExtHopOptProc* acquireHopOptInterface();
  //@}

  /**
     Returns the following extension header from fromHdrExt or the very first
     one if NULL
     Returns NULL if there are is no successive header.
  */

  HdrExtProc* getNextHeader(const HdrExtProc* fromHdrExt) const;


  /**
     Creates a new ext hdr and wrapper around it. Return the wrapper to
     allow modification of ext hdr.
     @param type the type of ext hdr to create
     @return the wrapper for type
  */
  HdrExtProc* addExtHdr(const IPv6ExtHeader& type);


  /**
    Search for a header of type specified in next_hdr and return
    the header.  Returns the very first one.  To search the next one
    use getNextHeader

    @arg next_hdr The protocol block type you are looking for either
    an IPv6 extension header.  The upper layer
    protocol identifier is obtained from transportProtocol().

    @returns The extension header if found or 0
  */
  HdrExtProc* findHeader(const IPv6ExtHeader& next_hdr) const;


///@name constructors, destructors and operators
///@{
  IPv6Datagram(const ipv6_addr& src = IPv6_ADDR_UNSPECIFIED,
               const ipv6_addr& dest = IPv6_ADDR_UNSPECIFIED,
               cPacket* pdu = 0, const char* name = 0);

  IPv6Datagram(const IPv6Datagram& srcObj);
  virtual ~IPv6Datagram();
  const IPv6Datagram& operator=(const IPv6Datagram& d);
  bool operator==(const IPv6Datagram& rhs) const;
///@}

///@name Overridden cObject functions
///@{
  virtual IPv6Datagram* dup() const { return new IPv6Datagram(*this); }
  virtual const char *className() const { return "IPv6Datagram"; }
  virtual std::string info();
  virtual void writeContents(ostream& os);
///@}

///@name Redefined cMessage functions
///@{
  void encapsulate(cPacket *);
  cPacket *decapsulate();
#ifdef __CN_PAYLOAD_H
  struct network_payload *networkOrder() const;
#endif /* __CN_PAYLOAD_H*/
///@}

  /**
     @name Input and output interfaces (control information,
     not part of the IPv6 header)
  */
  //@{
  int outputPort() { return outputPortNo; }
  void setOutputPort(int p) { outputPortNo = p; }
  int inputPort() { return inputPortNo; }
  void setInputPort(int p) { inputPortNo = p; }
  //@}

  /**
     @name IPv6 Header Attributes
     Get/Set functions for IPv6 Header fields
  */
  //@{
  short version() const
    {
      //Top 4 bits of ver_traffic_flow
      assert(header.ver_traffic_flow >>28 <= static_cast<unsigned int> (IPv6_MAX_VERSION));
      return static_cast<unsigned char> (header.ver_traffic_flow >>28);
    }

  unsigned int trafficClass() const
    {
      return (header.ver_traffic_flow >> 20) & 0xFF;
    }

  void setTrafficClass(unsigned int traffic_class)
    {
      assert(traffic_class < 1<<8);
      if (traffic_class >= 1<<8)
        return;
      header.ver_traffic_flow &= 0xF00FFFFF;
      header.ver_traffic_flow |= (traffic_class << 20);
    }

  unsigned int flowLabel() const
    {
      return header.ver_traffic_flow & 0xFFFFF;
    }

  void setFlowLabel(unsigned int label)
    {
      assert(label < 1<<20);
      if (label >= 1<<20)
        return;
      header.ver_traffic_flow &= 0xFFF00000;
      header.ver_traffic_flow |= label;
    }

  /// Payload length excludes the fixed IPv6 header length
  /// i.e. sizeof(ipv6_hdr) refer to RFC2460 Section 3.0
  int payloadLength() const { return header.payload_length;}
  void setPayloadLength(int length)
    {
      assert(length >= 0);
      header.payload_length = length;
    }

  /// length of IPv6 extension headers (Not part of IPv6 spec)
  short extensionLength() const { return ext_hdr_len;}

  /// length of Datagram in bytes (Not part of IPv6 spec)
  size_t totalLength() const
    {
      //Doesn't exceed 2^16-1 as that's jumbogram size
      assert(IPv6_HEADER_LENGTH + header.payload_length <= 1<<16 - 1  );
      return IPv6_HEADER_LENGTH + header.payload_length;
    }

  void setTotalLength(unsigned int len)
    {
#ifdef DEBUG
      //Doesn't exceed 2^16-1 as that's jumbogram size
      assert(len >= IPv6_HEADER_LENGTH);
      assert(len <= 1<<16 - 1  );
#endif
      header.payload_length = len - IPv6_HEADER_LENGTH;
    }

  short hopLimit() const { return header.hop_limit; }
  void setHopLimit(short ttl)
    {
      assert(ttl >= 0 && ttl <= (short) MAX_HOPLIMIT);
      header.hop_limit= ttl;
    }

  IPProtocolId transportProtocol() const;
  void setTransportProtocol(const IPProtocolId& prot);

  // header checksum not required

  /// source and destination address
  const ipv6_addr& srcAddress() const { return header.src_addr; }

  void setSrcAddress(const char* src)
    {
      header.src_addr = c_ipv6_addr(src);
    }
  void setSrcAddress(const ipv6_addr& src){ header.src_addr = src; }

  const ipv6_addr& destAddress() const { return header.dest_addr; }

  void setDestAddress(const char* dest)
    {
      header.dest_addr = c_ipv6_addr(dest);
    }

  void setDestAddress(const ipv6_addr& dest){ header.dest_addr = dest; }
  //@}

private:
  int outputPortNo;
  int inputPortNo;

  ipv6_hdr header;
  typedef std::list<HdrExtProc*> ExtHdrs;
  typedef ExtHdrs::iterator EHI;
  mutable ExtHdrs ext_hdrs;

  /**
     To test consistency of payload length against hdr len.
  */
  int ext_hdr_len;

  /**
     Cache constructed interfaces in this object. New objects
     duplicated will not have these accessory interface pointers
     duplicated otherwise will get leaks.  Off course can
     duplicate these interface objects too but will decrease
     performance after all don't create it unless we need to use
     it.

     Mutable because we are changing it only once depending on
     when constructed.  The constructed object may change
     internally but what we point to won't i.e. never replacing
     with entirely diff object.
  */
  mutable HdrExtRteProc* route_hdr;
  mutable HdrExtFragProc* frag_hdr;
  mutable HdrExtDestProc* dest_hdr;

  /**
     Add an extension header to this packet @hdr_type Identify
     what ext_hdr type you are appending @ext_hdr The ext_hdr
     itself.  Any memory that has been allocated in ext_hdr for
     pointers should not be deallocated.  This class will take
     care of that when it is destroyed.

     @return position in ext_hdrs
  */
  // size_t addExtHeader(const IPv6ExtHeader& hdr_type, const ipv6_ext_hdr& ext_hdr);

  ///Update pos, and links to next procHdr
  //void reLinkProcHdrs() const;

};

std::ostream& operator<<(std::ostream&, const IPv6Datagram& pdu);


#endif //IPV6DATAGRAM_H
