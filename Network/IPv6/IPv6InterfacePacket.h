// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/IPv6InterfacePacket.h,v 1.3 2005/02/11 12:23:46 andras Exp $
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
 * @file IPv6InterfacePacket.h
 * @brief Interface Packet between IPv6 layer and transport layer
 * @author Eric Wu
 * @test see InterfacePacketTest
 */

#ifndef IPv6INTERFACEPACKET_H
#define IPv6INTERFACEPACKET_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

#ifndef IPV6HEADERS_H
#include "IPv6Headers.h"
#endif //IPV6HEADERS_H
#ifndef IPv6_ADDR_H
#include "ipv6_addr.h"
#endif //IPv6_ADDR_H
#include "IPInterfacePacket.h"


/**
   @class IPv6InterfacePacket
   @brief Encapsulates upper layer PDUs for transfer between Transport and
   Network Layers.
 */
class IPv6InterfacePacket: public IPInterfacePacket
{
public:
  IPv6InterfacePacket(const char* src = 0, const char* dest = 0, cMessage *msg = 0);
  IPv6InterfacePacket(const ipv6_addr& src, const ipv6_addr& dest, cMessage *msg = 0);

  IPv6InterfacePacket(const IPv6InterfacePacket& );

  IPv6InterfacePacket& operator=(const IPv6InterfacePacket& ip);

  virtual IPv6InterfacePacket *dup() const
    { return new IPv6InterfacePacket(*this); }

  virtual const char* className() const { return "IPv6InterfacePacket"; }
  virtual std::string info();
  virtual void writeContents(ostream& os);

///@name Attributes
///@{
  virtual void setDestAddr(const char *destAddr){ _dest = c_ipv6_addr(destAddr); }
  virtual void setDestAddr(const ipv6_addr& dest) { _dest = dest; }

  virtual const char *destAddr() const
    {
    static string destAddrStorage;
    destAddrStorage = ipv6_addr_toString(_dest);
    return destAddrStorage.c_str();
    }

  ipv6_addr destAddress() const { return _dest; }

  virtual void setSrcAddr(const char *srcAddr){ _src = c_ipv6_addr(srcAddr); }
  virtual void setSrcAddr(const ipv6_addr& src) { _src = src; }

  virtual const char *srcAddr() const
    {
      static string srcAddrStorage;
      srcAddrStorage = ipv6_addr_toString(_src);
      return srcAddrStorage.c_str();
    }

  ipv6_addr srcAddress() const { return _src; }

///@}

private:

  ipv6_addr _src;
  ipv6_addr _dest;
};

#endif
