//
// Copyright (C) 2001  Eric Wu (eric.wu@eng.monash.edu.au)
// Monash University, Melbourne, Australia

/*
 *  @file    ipv6_addr.cc
 *
 *  @brief Represention of an IPv6 address
 *
 *  @author  Eric Wu
 *
 *  @date    22/8/2001
 *
 */

#include "sys.h"
#include "debug.h"

#include "ipv6_addr.h"
#include "InterfaceEntry.h" // for InterfaceToken
#include <cassert>
#include <sstream>
#include <iostream>
#include <algorithm>

const size_t IPv6_ADDR_BITLENGTH = 128;
const unsigned int EUI64_LENGTH = 64;

const char* LOOPBACK_ADDRESS = "0:0:0:0:0:0:0:1/0";
const char* ALL_NODES_NODE_ADDRESS = "FF01:0:0:0:0:0:0:1/0";
const char* ALL_NODES_LINK_ADDRESS = "FF02:0:0:0:0:0:0:1/0";
const char* ALL_NODES_SITE_ADDRESS = "FF05:0:0:0:0:0:0:1/0";

const char* ALL_ROUTERS_NODE_ADDRESS = "FF01:0:0:0:0:0:0:2/0";
const char* ALL_ROUTERS_LINK_ADDRESS = "FF02:0:0:0:0:0:0:2/0";
const char* ALL_ROUTERS_SITE_ADDRESS = "FF05:0:0:0:0:0:0:2/0";

const char* LINK_LOCAL_PREFIX = "FE80:0:0:0:0:0:0:0/64";
const char* SOLICITED_NODE_PREFIX = "FF02:0:0:0:0:1:FF00:0000/108";

namespace
{
  const unsigned int NUMBER_OF_SEGMENT_BIT = 32;
  const unsigned int TOTAL_NUMBER_OF_32BIT_SEGMENT = 4;

  const int LWMASK = 0xFFFF;
  const char IPv6_ADDR_SEP = ':';
  const char IPv6_PREFIX_SEPARATOR = '/';

};

const ipv6_addr IPv6_ADDR_UNSPECIFIED = {0,0,0,0};
const ipv6_addr IPv6_ADDR_MULTICAST_PREFIX =  {0xFF000000,0,0,0};
const ipv6_addr IPv6_ADDR_MULT_NODE_PREFIX =  {0xFF010000,0,0,0};
const ipv6_addr IPv6_ADDR_MULT_LINK_PREFIX =  {0xFF020000,0,0,0};
const ipv6_addr IPv6_ADDR_MULT_SITE_PREFIX =  {0xFF050000,0,0,0};
const ipv6_addr IPv6_ADDR_MULT_ORGN_PREFIX =  {0xFF080000,0,0,0};
const ipv6_addr IPv6_ADDR_MULT_GLOB_PREFIX =  {0xFF0E0000,0,0,0};
const ipv6_addr IPv6_ADDR_LINK_LOCAL_PREFIX = {0xFE800000,0,0,0};
const ipv6_addr IPv6_ADDR_SITE_LOCAL_PREFIX = {0xFEC00000,0,0,0};
const ipv6_addr IPv6_ADDR_GLOBAL_PREFIX =     {0x20000000,0,0,0};

const unsigned int TLA_BITSHIFT = 112;
const unsigned int NLA_BITSHIFT = 80;
const unsigned int SLA_BITSHIFT = 64;

const ipv6_addr IPv6_ADDR_TLA_MASK = {0x1FFF0000, 0, 0, 0};
const ipv6_addr IPv6_ADDR_RES_MASK = {0xFF00, 0, 0, 0};
const ipv6_addr IPv6_ADDR_NLA_MASK = {0xFF,0xFFFF0000,0,0};
const ipv6_addr IPv6_ADDR_SLA_MASK = {0, 0x0000FFFF, 0, 0};


ipv6_addr& ipv6_addr::operator+=(unsigned int number)
{
  low += number;
  return *this;
}

ipv6_addr& ipv6_addr::operator+=(const ipv6_addr& rhs)
{
  low += rhs.low;
  normal += rhs.normal;
  high += rhs.high;
  extreme += rhs.extreme;
  return *this;

}

namespace
{
  typedef boost::array<unsigned long long,2> ipv6_addr_llarray;
  ipv6_addr_llarray ipv6_addr_toLongArray(const ipv6_addr& src)
  {
    ipv6_addr_llarray addr;
    addr[0] = ((unsigned long long)src.normal<<NUMBER_OF_SEGMENT_BIT) + src.low;
    addr[1] = ((unsigned long long)src.extreme<<NUMBER_OF_SEGMENT_BIT) + src.high;
    return addr;
  }

  ipv6_addr c_ipv6_addr(const ipv6_addr_llarray& addr)
  {
    ipv6_addr val;
    val.extreme = addr[1]>>NUMBER_OF_SEGMENT_BIT;
    val.high = addr[1]&(((long long)1<<NUMBER_OF_SEGMENT_BIT)-1);
    val.normal = addr[0]>>NUMBER_OF_SEGMENT_BIT;
    val.low = addr[0]&(((long long)1<<NUMBER_OF_SEGMENT_BIT)-1);
    return val;
  }
}

ipv6_addr& ipv6_addr::operator>>=(unsigned int shift)
{
  if (shift == 0)
    return *this;

  ipv6_addr_llarray addr = ipv6_addr_toLongArray(*this);

  if (shift < sizeof(addr[0])*8)
  {
    addr[0] >>= shift ;
    unsigned long long carry = addr[1] & ((1<<shift)-1);
    addr[1] >>= shift;
    addr[0]|=(carry<<(sizeof(addr[0])*8 - shift));
  }
  else if (shift == sizeof(addr[0])*8)
  {
    addr[0] = addr[1];
    addr[1] = 0;
  }
  else
  {
    unsigned int mod = shift % (sizeof(addr[0])*8);
    addr[0] = addr[1]>>mod;
    addr[1] = 0;
  }

  return *this = c_ipv6_addr(addr);

}

ipv6_addr& ipv6_addr::operator<<=(unsigned int shift)
{
  if (shift == 0)
    return *this;

  ipv6_addr_llarray addr = ipv6_addr_toLongArray(*this);

  if (shift < sizeof(addr[0])*8)
  {
    addr[1] <<= shift;
    unsigned long long carry = (addr[0] >> (sizeof(addr[0])*8-shift));
    addr[1] |= carry;
    addr[0] <<= shift;
  }
  else if(shift == sizeof(addr[0])*8)
  {
    addr[1] = addr[0];
    addr[0] = 0;
  }
  else
  {
    unsigned int mod = shift % (sizeof(addr[0])*8);
    addr[1] = addr[0]<<mod;
    addr[0] = 0;
  }

  return *this = c_ipv6_addr(addr);
}

ipv6_addr& ipv6_addr::operator|=( const ipv6_addr& rhs)
{
  extreme|= rhs.extreme;
  high|= rhs.high;
  normal |= rhs.normal;
  low |= rhs.low;
  return *this;
}

ipv6_addr& ipv6_addr::operator&=( const ipv6_addr& rhs)
{
  extreme &= rhs.extreme;
  high &= rhs.high;
  normal &= rhs.normal;
  low &= rhs.low;
  return *this;

}

/**
 * @defgroup ipv6_addrOp free ipv6_addr operators
 *
 */
//@{
bool operator==(const ipv6_addr& lhs, const ipv6_addr& rhs)
{
  return lhs.extreme == rhs.extreme && lhs.high == rhs.high &&
    lhs.normal == rhs.normal && lhs.low == rhs.low;
}

bool operator!=(const ipv6_addr& lhs, const ipv6_addr& rhs)
{
      return !(lhs==rhs);
}

// reverse sorting
bool operator<(const ipv6_addr& lhs, const ipv6_addr& rhs)
{
  if(lhs.extreme != rhs.extreme)
    return (lhs.extreme < rhs.extreme) ? false : true;
  if(lhs.high != rhs.high)
    return (lhs.high < rhs.high)       ? false : true;
  if(lhs.normal != rhs.normal)
    return (lhs.normal < rhs.normal)   ? false : true;
  if(lhs.low != rhs.low)
    return (lhs.low < rhs.low)         ? false : true;
  return false;
}

ipv6_addr operator&(const ipv6_addr& lhs, const ipv6_addr& rhs)
{
  ipv6_addr ret = lhs;
  return ret &= rhs;

}

ipv6_addr operator|(const ipv6_addr& lhs, const ipv6_addr& rhs)
{
  ipv6_addr ret = lhs;
  return ret |= rhs;

}

ipv6_addr operator>>(const ipv6_addr& src, unsigned int shift)
{
  ipv6_addr ret = src;
  return ret>>=shift;
}

ipv6_addr operator<<(const ipv6_addr& src, unsigned int shift)
{
  ipv6_addr ret = src;
  return ret<<=shift;
}


std::ostream& operator<<( std::ostream& os, const ipv6_addr& src_addr)
{
  //May be due to os been deleted somewhere
  //Dout(dc::custom|flush_cf, FILE_LINE<<" Weird ostream null object here"<<os);

  os<<hex<< (src_addr.extreme>>16) <<IPv6_ADDR_SEP<< (src_addr.extreme&LWMASK)
    <<IPv6_ADDR_SEP<<(src_addr.high>>16)<<IPv6_ADDR_SEP<<(src_addr.high&LWMASK)
    <<IPv6_ADDR_SEP<< (src_addr.normal>>16)<<IPv6_ADDR_SEP<<(src_addr.normal&LWMASK)
    <<IPv6_ADDR_SEP<< (src_addr.low>>16)<<IPv6_ADDR_SEP<< (src_addr.low&LWMASK)
    <<dec;
  return os;
}

ipv6_addr operator+(const ipv6_addr& lhs, const ipv6_addr& rhs)
{
  ipv6_addr res = lhs;
  return res+=rhs;
}

ipv6_addr c_ipv6_addr(const char* addr)
{
  if (addr == 0)
    addr = "";

  stringstream is(addr);

  unsigned int octals[8] = {0, 0, 0, 0, 0, 0, 0, 0} ;
  char sep = '\0';

  try
  {
    for (int i = 0; i < 8; i ++)
    {

      is >> hex >> octals[i];
      if (is.eof() )
        break;
      is >> sep;
      if (sep == IPv6_PREFIX_SEPARATOR)
        break;
    }
  }
  catch (...)
  {
    std::cerr << "exception thrown while parsing ipv6 char* address";
    return IPv6_ADDR_UNSPECIFIED;
  }

  unsigned int int_addr[TOTAL_NUMBER_OF_32BIT_SEGMENT];
  for (unsigned int i = 0; i < TOTAL_NUMBER_OF_32BIT_SEGMENT; i++ )
    int_addr[i] = (octals[i*2]<<(NUMBER_OF_SEGMENT_BIT/2)) + octals[2*i + 1];

  ipv6_addr ret;

  ret.extreme = int_addr[0];
  ret.high = int_addr[1];
  ret.normal = int_addr[2];
  ret.low = int_addr[3];

  return ret;
}

ipv6_addr_array ipv6_addr_toArray(const ipv6_addr& addr)
{
  ipv6_addr_array buf = {{addr.extreme, addr.high, addr.normal, addr.low}};
  return buf;
}

string ipv6_addr_toString(const ipv6_addr& addr)
{
  stringstream ss;
  ss<<addr;
  return ss.str();
}

string ipv6_addr_toBinary(const ipv6_addr& src)
{
  stringstream ss;
  ipv6_addr_array addr = ipv6_addr_toArray(src);
  for (unsigned int h = 0; h < TOTAL_NUMBER_OF_32BIT_SEGMENT; h++)
    for (unsigned int i = NUMBER_OF_SEGMENT_BIT; i > 0; i--)
    {
      if (((h > 0) || h == 0 && i < NUMBER_OF_SEGMENT_BIT) && i % 16 == 0)
        ss<<":";
      ss<<(addr[h]&(1<<i-1)?"1":"0");
    }

  assert(ss.str().size() == IPv6_ADDR_BITLENGTH+7);
  return ss.str();
}

string longToBinary(unsigned long long no)
  {
    stringstream ss;
    for (unsigned int i = sizeof(no)*8; i > 0; i--)
    {
      if (i < sizeof(no)*8 && i % 16 == 0)
        ss<<":";
      ss<<(no&(1<<i-1)?"1":"0");
    }
    return ss.str();
    assert(ss.str().size() == IPv6_ADDR_BITLENGTH/2+3);
  }
//@}

ipv6_addr ipv6_addr_fromInterfaceToken(const ipv6_addr& prefix, const InterfaceToken& token)
{
  assert(token.length() == 64);
  ipv6_addr addr = prefix;
  addr.normal = token.normal();
  addr.low = token.low();
  return addr;
}

/**
 * Return the number of bits that match in addr1 and addr2
 *
 * @note Can use std::bitset for clarity
 */

size_t calcBitsMatch(const unsigned int* addr1, const unsigned int* addr2)
{
  unsigned int res[TOTAL_NUMBER_OF_32BIT_SEGMENT];
  for(unsigned int i=0; i<TOTAL_NUMBER_OF_32BIT_SEGMENT; i++)
  {
    res[i] = addr1[i] ^ addr2[i];
    // If the bits are equal, there is a 0, so counting
    // the zeros from the left
    for (int j = 31; j > -1; j--)
      if (res[i] & (1 << j))
        return (31 - j) + ( NUMBER_OF_SEGMENT_BIT * i );
  }
  return (TOTAL_NUMBER_OF_32BIT_SEGMENT * NUMBER_OF_SEGMENT_BIT);
}

/**
 * Return the number of bits that match in addr1 and addr2
 *
 */
size_t calcBitsMatch(const ipv6_addr_array& addr1, const ipv6_addr_array& addr2)
{
  unsigned int a1[TOTAL_NUMBER_OF_32BIT_SEGMENT] = {addr1[0], addr1[1], addr1[2], addr1[3]};
  unsigned int a2[TOTAL_NUMBER_OF_32BIT_SEGMENT] = {addr2[0], addr2[1], addr2[2], addr2[3]};
  return calcBitsMatch(a1,a2);
}


bool ipv6_prefix::matchPrefix(const ipv6_addr& addr) const
{
    //return IPv6Address(*this).isNetwork(rhs.prefix);
  return calcBitsMatch(ipv6_addr_toArray(prefix),
                       ipv6_addr_toArray(addr))>=length;
}


ostream& operator<<( ostream& os, const ipv6_prefix& src_addr)
{
  os<<src_addr.prefix<<"/"<<src_addr.length;
  return os;
}


#if defined USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>
#include <string>

class ipv6_addrTest: public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( ipv6_addrTest );
  CPPUNIT_TEST( testConversions );
  CPPUNIT_TEST( testShift );
  CPPUNIT_TEST( prefixEquality );
  CPPUNIT_TEST_SUITE_END();

  void testConversions()
    {
      //cout<<" lltoBin "<<longToBinary(0x8000800000010001)<<endl;
      const std::string _64sep("8001:8001:8001:8001:8001:8001:8001:8001");
      CPPUNIT_ASSERT(ipv6_addr_toBinary(c_ipv6_addr(_64sep.c_str())) == "1000000000000001:1000000000000001:1000000000000001:1000000000000001:1000000000000001:1000000000000001:1000000000000001:1000000000000001");
    }

  void testShift()
    {
      const std::string saddr("8000:0:0:8002:0:0:0:0");
      ipv6_addr addr = c_ipv6_addr(saddr.c_str());

      addr >>=  2;
      cout << ipv6_addr_toBinary(addr)<<endl;

      addr <<= 2;
      cout << ipv6_addr_toBinary(addr)<<endl;
      CPPUNIT_ASSERT(ipv6_addr_toString(addr) == saddr);

      addr >>= 65;
      addr <<= 65;
      CPPUNIT_ASSERT(ipv6_addr_toString(addr) == saddr);

      //const std::string saddr("abcd:1234:5678:8888:1111:2222:3333:4444");
    }

  void prefixEquality()
    {
      ipv6_addr a = c_ipv6_addr("3018:ffff:0:0:127b:c0ff:fe2e:7212");
      ipv6_addr b = c_ipv6_addr("3092:eeee:2344:3333:a64b:65ff:fec6:a7fc");
      ipv6_prefix pa(a, 64);
      ipv6_prefix pb(b, 64);
      CPPUNIT_ASSERT(pa != pb);

      ipv6_addr_array c = {{0x3232, 0xabcd, 0xef00,0xfe }};
      ipv6_addr_array d = {{0x3232, 0xabcd, 0x2, 0x0}};
      CPPUNIT_ASSERT(calcBitsMatch(c, d) == 80);

      ipv6_addr_array e = {{0x3232, 0xabecd, 0xef00,0xfe }};
      ipv6_addr_array f = {{0x3232, 0xabcd, 0x2, 0x0}};
      CPPUNIT_ASSERT(calcBitsMatch(e, f) == 44);
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(ipv6_addrTest);
#endif //USE_CPPUNIT
