// -*- C++ -*-
// Copyright (C) 2004 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file ICMPv6NDOptions.h
 * @author Johnny Lai
 * @date 18 Jul 2004
 *
 * @brief Definition of Neighbour Discovery Option classes
 *
 * Extracted from ICMPv6NDMessage.h
 */

#ifndef ICMPV6NDOPTIONS_H
#define ICMPV6NDOPTIONS_H

#ifndef ICMPV6NDOPTIONBASE_H
#include "ICMPv6NDOptionBase.h"
#endif

#ifndef NDENTRY_H
#include "NDEntry.h" //class AdvPrefixList/PrefixEntry;
#endif //NDENTRY_H

class IPv6Datagram;

namespace IPv6NeighbourDiscovery
{

/**
 * @addtogroup ICMPv6NDOptions
 * @{
 */


/**
 * @class ICMPv6NDOptPrefix
 * @brief Prefix Option
 */

class ICMPv6NDOptPrefix: public IPv6NeighbourDiscovery::ICMPv6_NDOptionBase,
                         boost::equality_comparable<ICMPv6NDOptPrefix>
{
public:

  ICMPv6NDOptPrefix(size_t prefix_len = 0,
                    const ipv6_addr& oprefix = IPv6_ADDR_UNSPECIFIED,
                    bool onLink = true, bool autoConf = true,
                    size_t validTime = 0, size_t preferredTime = 0
#ifdef USE_MOBILITY
                    , bool rtr_addr = false
#endif
                    );

  ICMPv6NDOptPrefix(const PrefixEntry& pe);

  virtual ICMPv6NDOptPrefix* dup() const
    {
      ///Use compiler's generated copy ctor
      return new ICMPv6NDOptPrefix(*this);
    }

  unsigned char prefixLen;
  bool onLink;
  bool autoConf;
  unsigned int validLifetime;
  unsigned int preferredLifetime;
  ipv6_addr prefix;
#ifdef USE_MOBILITY
  bool rtrAddr;
#endif
};

/**
 * @class ICMPv6NDOptLLAddr
 * @brief Link Layer Address option
 */

class ICMPv6NDOptLLAddr: public IPv6NeighbourDiscovery::ICMPv6_NDOptionBase
{
public:

  ///Default len of one for Ethernet link layer (6 bytes)
  ICMPv6NDOptLLAddr(bool source, const string& link_layer_addr, int len = 1)
    :ICMPv6_NDOptionBase(source?NDO_SRC_LINK_LAYER_ADDR:NDO_DEST_LINK_LAYER_ADDR, len), addr(link_layer_addr)
    {
    }
  virtual ICMPv6NDOptLLAddr* dup() const
    {
      ///Use compiler's generated copy ctor
      return new ICMPv6NDOptLLAddr(*this);
    }
  const string& address() const
    { return addr; }
  void setAddress(const string& lladdr)
    {
      addr = lladdr;
    }

private:

  ///Use however many bytes are necessary.  For Ethernet only 6
  ///bytes of this 16 byte
  string addr;
};

/**
   @class ICMPv6NDOptRedirect
   @brief Redirect Option
 */
class ICMPv6NDOptRedirect: public IPv6NeighbourDiscovery::ICMPv6_NDOptionBase
{
public:
      //Length of contained pdu in 8 octet units
  ICMPv6NDOptRedirect(IPv6Datagram* header = 0, int len = 0)
    :ICMPv6_NDOptionBase(NDO_REDIRECTED_HEADER, len), _header(header)
    {}
  ~ICMPv6NDOptRedirect();
  virtual ICMPv6NDOptRedirect* dup() const
    {
      ///Use compiler's generated copy ctor
      return new ICMPv6NDOptRedirect(*this);
    }
  void setHeader(IPv6Datagram* header);

  const IPv6Datagram* header() const { return _header; }

  IPv6Datagram* removeHeader();

private:
  IPv6Datagram* _header;

};

/**
 * @class ICMPv6NDOptMTU
 * @brief MTU option
 *
 */

class ICMPv6NDOptMTU: public IPv6NeighbourDiscovery::ICMPv6_NDOptionBase
{
public:
  ICMPv6NDOptMTU(unsigned int link_mtu)
    :ICMPv6_NDOptionBase(NDO_MTU, 1)
    {}
  virtual ICMPv6NDOptMTU* dup() const
    {
      ///Use compiler's generated copy ctor
      return new ICMPv6NDOptMTU(*this);
    }
  unsigned int MTU() const
    { return mtu; }
  void setMTU(unsigned int link_mtu)
    {
      mtu = link_mtu;
    }

private:
  unsigned int mtu;

};

#ifdef USE_MOBILITY
// New Advertisement Interval Option

class ICMPv6NDOptAdvInt: public IPv6NeighbourDiscovery::ICMPv6_NDOptionBase
{
public:

  ICMPv6NDOptAdvInt(unsigned long advInt = 0);

  virtual ICMPv6NDOptAdvInt* dup() const
    {
      ///Use compiler's generated copy ctor
      return new ICMPv6NDOptAdvInt(*this);
    }

  unsigned long advInterval;
};
#endif
/** @} */

};

#endif /* ICMPV6NDOPTIONS_H */

