// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/HdrExtRteProc.h,v 1.2 2005/02/10 05:27:42 andras Exp $
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
	@file HdrExtRteProc.h
	@brief Routing header operations.
	@author Johnny Lai
	@date 21.8.01
*/

#include <list>
#include <cassert>
#include "IPv6Headers.h"
#include "HdrExtProc.h"

class IPv6Datagram;

/**
   @class HdrExtRte
   @brief Type 0 Routing extension header.
 */
class HdrExtRte
{
public:

  explicit HdrExtRte(unsigned char rt_type = IPv6_TYPE0_RT_HDR);

  explicit HdrExtRte(const HdrExtRte& src);

  virtual ~HdrExtRte()
    {
      delete [] rt0_hdr->addr;
      delete rt0_hdr;
    }

  /*v*/ bool operator==(const HdrExtRte& rhs);

  /*v*/ HdrExtRte& operator=(const HdrExtRte& rhs);

  /*v*/ std::ostream& operator<<(std::ostream& os);

  /*v*/ HdrExtRte* dup() const
    {
      return new HdrExtRte(*this);
    }

//  /**
//        @Overridden cObject functions
//      */
//     //@{
//   virtual cObject* dup() const { return new HdrExtRte(*this); }
//   virtual const char *className() const { return "HdrExtRte"; }
//   virtual std::string info();
//   virtual void writeContents(ostream& os);
//   //@}

  /**
      @name Overridden HdrExtProc functions
  */
  ///@{
  ///processed at every dest in routing header
  virtual bool processHeader(cSimpleModule* mod,
                             IPv6Datagram* pdu,
                             int cumul_len);
  ///@}

  //Returns the ith address based starting at 1
  const ipv6_addr& address(size_t index);

  void addAddress(const ipv6_addr& addr);

  /// Returns Type 0 routing value of IPV6_TYPE0_RT_HDR
  unsigned char routingType() const
  {
    assert(rt0_hdr->routing_type == IPv6_TYPE0_RT_HDR ||
           rt0_hdr->routing_type == IPv6_TYPE2_RT_HDR);

    return rt0_hdr->routing_type;
  }

  void setRoutingType(unsigned char type)
  {
    rt0_hdr->routing_type = type;

  }

  size_t length(void)
  {
    return (n_addr()*IPv6_ADDRESS_LEN + IPv6_RT_HDR_LEN);
  }

  unsigned char segmentsLeft() const
  {
    return rt0_hdr->segments_left;
  }
  unsigned char hdrExtLen() const
  {
    return rt0_hdr->hdr_ext_len;
  }

  void setHdrExtLen(unsigned char len)
    {
      rt0_hdr->hdr_ext_len = len;
    }

  unsigned char n_addr() const
    {
      return hdrExtLen()/2;
    }

protected:

  void setSegmentsLeft(int segments)
  {
    assert(0 < segments && segments < 256);

    rt0_hdr->segments_left = segments;
  }
  void setHdrExtLen(int octetUnits)
  {
    assert(0 < octetUnits && octetUnits < 256);
    rt0_hdr->hdr_ext_len = octetUnits;
  }

  ipv6_ext_rt0_hdr* rt0_hdr;
};

namespace MobileIPv6
{

/**
 * @class MIPv6RteOpt
 * @brief Type 2 Routing Extension Header
 *
 * Routing Header used for route optimisation of packets exchanged between
 * correspondent nodes(CN) and Mobile Node(MN).  This is much more efficient
 * than tunneling via Home Agent(HA).
 */

class MIPv6RteOpt: public HdrExtRte
{
public:

  explicit MIPv6RteOpt(const ipv6_addr& home_addr = IPv6_ADDR_UNSPECIFIED)
    :HdrExtRte(IPv6_TYPE2_RT_HDR)
    {
      addAddress(home_addr);
      //setSegmentsLeft(1);
      //setHdrExtLen(2);
    }

  virtual ~MIPv6RteOpt()
    {}

  virtual bool processHeader(cSimpleModule* mod, IPv6Datagram* pdu,
                             int cumul_len);
};

} //namespace MobileIPv6

/**
   @class HdrExtRteProc
   @brief Processing of HdrExtRte and MIPv6RteOpt (Routing Extension Headers)
 */

class HdrExtRteProc : public HdrExtProc
{
public:

  explicit HdrExtRteProc(void);

  explicit HdrExtRteProc(const HdrExtRteProc& src);

  ~HdrExtRteProc(void);

  bool operator==(const HdrExtRteProc& rhs);

  HdrExtRteProc& operator=(const HdrExtRteProc& rhs);

  virtual std::ostream& operator<<(std::ostream& os);

  virtual HdrExtRteProc* dup() const
    {
      return new HdrExtRteProc(*this);
    }

  /**
      @name Overridden HdrExtProc functions
  */
  ///@{
  ///processed every routing header that it contains
  virtual bool processHeader(cSimpleModule* mod, IPv6Datagram* pdu);
  ///@}

  bool addRoutingHeader(HdrExtRte* rh);

  bool isSegmentsLeft(void);

  // return a routing header; the default routing header returned is type 0
  HdrExtRte* routingHeader(unsigned char rt_type = IPv6_TYPE0_RT_HDR);

  virtual size_t length() const;

private:
  typedef std::list<HdrExtRte*>RoutingHeaders;
  typedef RoutingHeaders::iterator RHIT;

  mutable RoutingHeaders rheads;
  ipv6_ext_rt_hdr& rt_hdr;
};


