// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/ICMPv6Message.h,v 1.3 2005/02/10 05:59:32 andras Exp $
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
    @file ICMPv6Message.h

    @brief ICMPv6 Error Messages definition in ICMPv6Message and ICMPv6
    Informational Messages definition in ICMPv6Echo.

    Refer to RFC2463.
    @author Johnny Lai
    @date 13.9.01
*/
#if !defined ICMPV6MESSAGE_H
#define ICMPV6MESSAGE_H

#ifndef __CPACKET_H
#include <cpacket.h>
#endif //__CPACKET_H

#ifndef CASSERT
#define CASSERT
#include <cassert>
#endif //CASSERT


typedef int ICMPv6Code;
static const int ICMPv6_MAX_CODE = 255;

/**
   ICMPv6 allowable message types
 */
enum ICMPv6Type
{
  ICMPv6_UNSPECIFIED = 0,
  ICMPv6_DESTINATION_UNREACHABLE = 1,
  ICMPv6_PACKET_TOO_BIG = 2,
  ICMPv6_TIME_EXCEEDED = 3,
  ICMPv6_PARAMETER_PROBLEM = 4,
  ICMPv6_ECHO_REQUEST = 128,
  ICMPv6_ECHO_REPLY = 129,
  ICMPv6_MLD_QUERY = 130,
  ICMPv6_MLD_REPORT = 131,
  ICMPv6_MLD_DONE = 132,
  ICMPv6_ROUTER_SOL = 133,
  ICMPv6_ROUTER_AD = 134,
  ICMPv6_NEIGHBOUR_SOL = 135,
  ICMPv6_NEIGHBOUR_AD = 136,
  ICMPv6_REDIRECT = 137,
  ICMPv6_MLDv2_REPORT = 143
};

enum // const int ICMPv6DEST_UN[] =
{
  NO_ROUTE_TO_DEST = 0,
  ADDRESS_UNREACHABLE = 3
/*
  1, //Communication with dest prohibited (firewall filter)
  2, //Unused
  4 //port unreachable
*/
};


enum //static const int ICMPv6_TIME_EX[] =
{
  ND_HOP_LIMIT_EXCEEDED = 0, //hop limit exceeded
  ND_FRAGMENT_REASSEMBLY_TIME //fragment reassembly time exceeded
};


static const int ICMPv6_PARARM_PROB[] =
{
  0, //erroneous header field
  1, //unrecognized Next Header type
  2 //unrecognized IPv6 option encountered
};


class IPv6InterfacePacket;
class IPv6Datagram;

/**
   @class ICMPv6Message

   @brief ICMPv6 message class for ICMP error messages

   Destination Unreachable, Packet too big, time exceeded and
   parameter problem error messages.  Base class for Informational
   Echo and Neighbour Disc ICMP Messages.

   kind() is synonymous with the type field of the ICMP mesage

   The offending PDU that caused this ICMP message to be issued will
   be stored and accessed using encapsulate/decapsulate respectively.
*/
class ICMPv6Message: public cPacket
{
public:

///@name constructors, destructors and operators
///@{
  ICMPv6Message(const ICMPv6Type otype, const ICMPv6Code ocode = 0,
                IPv6Datagram* errorPdu = 0, size_t optInfo = 0);
  ICMPv6Message(const ICMPv6Message& src);
  virtual ~ICMPv6Message();
  bool operator==(const ICMPv6Message& rhs) const;

  ICMPv6Message& operator=(const ICMPv6Message & );
///@}


/// @name Overridden cObject functions
///@{
  virtual ICMPv6Message* dup() const
    { return new ICMPv6Message(*this); }
  virtual const char* className() const
    { return "ICMPv6Message"; }

  virtual std::string info();
///@}

///@name Redefined cMessage functions
///@{
  ///ICMP Error messages contain the offending datagram
  ///Use cPacket::encapsulate() to encapsulate any cMessage
  void encapsulate(IPv6Datagram* errorPdu);
  ///Use cPacket::decapsulate() if you want a cPacket back instead
  IPv6Datagram *decapsulate();
  ///Use cPacket::encapsulatedMsg() if you want a cPacket back instead
  IPv6Datagram *encapsulatedMsg() const;

#if defined __CN_PAYLOAD_H
  virtual struct network_payload *networkOrder() const;
  unsigned short networkCheckSum(unsigned char*icmpmsg,
         struct in6_addr *src, struct in6_addr *dest, int icmplen) const;
#endif //defined __CN_PAYLOAD_H
///@}

      /**
         Unimplemented.  A better way would be for the lower classes
         to continually call upper classes passing in a string to
         which the contents are appended and checksum calculated in
         another func.
      */
  virtual int calculateChecksum()
    {
      return 0;
    }

  virtual size_t optionCount() const { return 0; }

  bool isErrorMessage() const;

///@name Attributes
///@{
  void setChecksum(int cs)
    {
      assert( cs <= 0xffff);
      _checksum = cs;
    }

  int checksum() const
    { return _checksum; }

  const ICMPv6Type& type() const
    { return _type; }

  const ICMPv6Code& code() const
    { return _code; }

  unsigned int optInfo() const
    { return _opt_info; }

  void setOptInfo(unsigned int info)
    { _opt_info = info; }
///@}
private:

  ICMPv6Type _type;
  ICMPv6Code _code;
  ///16 bit checksum
  int _checksum;

      ///Usually reserved but some ICMP messages delimit this field
      ///further for use (next 4 octets after checksum)
  unsigned int _opt_info;

};


/**
   @class ICMPv6Echo
   @brief Arbitrary data in ping packets can be specified by encapsulating it.
 */
class ICMPv6Echo: public ICMPv6Message
{
public:

///@name constructors, destructors and operators
///@{
  ICMPv6Echo(int id = 0, int seq = 0,  bool request = false);
  ICMPv6Echo(const ICMPv6Echo& src );
  ICMPv6Echo& operator=(const ICMPv6Echo& rhs);
///@}

/// @name Overridden cObject functions
///@{
  virtual std::string info();
  virtual ICMPv6Echo *dup() const
    { return new ICMPv6Echo(*this); }
  virtual const char* className() const
    { return "ICMPv6Echo"; }
///@}

  ///16 bit identifier for each ping session
  int seqNo() const
    { return optInfo()&0xffff; }
  int identifier() const
    { return optInfo()>>16; }

  ///sequence number incremented regardless of ping timeouts
  void setSeqNo(int seq)
    {
      assert(seq <= 0xffff);
      setOptInfo((optInfo()&0xffff0000)|seq);
    }

  void setIdentifier(int id)
    {
      assert(id <= 0xffff);
      setOptInfo((optInfo()&0xffff)|(id<<16));
    }

};

#endif //ICMPV6MESSAGE_H

