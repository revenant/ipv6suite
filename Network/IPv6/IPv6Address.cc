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

/*
 *  @file    IPv6Address.cc
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

#include "IPv6Address.h"

#include <sstream>
#include <iostream>

//Use static qualifier if this does not compile
namespace 
{  
  const char IPv6_PREFIX_SEPARATOR = '/';

  const int NUMBER_OF_SEGMENT_BIT = 32;
  const int TOTAL_NUMBER_OF_32BIT_SEGMENT = 4;

  const int MAX_NUMBER_OF_ADDRESS_DIGIT = 32;
  const int MAX_NUMBER_OF_PREFIX_LEN_DIGIT = 3;
  const int MAX_NUMBER_OF_DELIMINATOR = 8;

  ipv6_addr IPv6ADDRESS_UNSPECIFIED_STRUCT = {0,0,0,0};
}

/**
 * @ingroup ipv6_addrOp 
 */

#if defined OPP_VERSION && OPP_VERSION >= 3
#else
cEnvir& operator<<(cEnvir& ev, const ipv6_addr& addr)
{
  ostringstream os;
  os<<addr;
  ev.printf("%s",os.str().c_str());
  return ev;
}
#endif

bool operator==(const ipv6_addr& lhs, const IPv6Address& rhs)
{
  return rhs.operator==(lhs);
}

IPv6Address operator+(const IPv6Address& lhs, const IPv6Address& rhs)
{
  IPv6Address combine;
#if !defined __INTEL_COMPILER
  Dout(dc::ipv6addrdealloc, __FUNCTION__<<" Called from " 
       <<location_ct((char*)__builtin_return_address(0) + 
                     libcwd::builtin_return_address_offset));
//  Dout(dc::ipv6addrdealloc, " Storage is "<< combine.storage());
#endif  //__INTEL_COMPILER  
  unsigned int combine_int_addr[TOTAL_NUMBER_OF_32BIT_SEGMENT];

  for(int i=0; i<TOTAL_NUMBER_OF_32BIT_SEGMENT; i++)
    combine_int_addr[i] = 0;

  const unsigned int* lhs_int_addr = lhs.getFullInt();
  const unsigned int* rhs_int_addr = rhs.getFullInt();

  for(int i=0; i<TOTAL_NUMBER_OF_32BIT_SEGMENT; i++)
    combine_int_addr[i] = lhs_int_addr[i] | rhs_int_addr[i];

  if(lhs.prefixLength()>0)
    combine.setPrefixLength(lhs.prefixLength());
  else if(rhs.prefixLength()>0)
    combine.setPrefixLength(rhs.prefixLength());
  
  combine.setAddress(combine_int_addr);

  return combine;  
}

Register_Class( IPv6Address );

IPv6Address::IPv6Address(unsigned int* addr_seg, int prefix_len, const char *n)
  :cObject(n), m_addr(IPv6ADDRESS_UNSPECIFIED_STRUCT), m_prefix_length(prefix_len),
   m_scope(ipv6_addr::Scope_None), m_storedLifetime(VALID_LIFETIME_INFINITY),
   m_preferredLifetime(VALID_LIFETIME_INFINITY), _updated(false)


{
#if !defined __INTEL_COMPILER
  Dout(dc::ipv6addrdealloc, "Called from " <<
       location_ct((char*)__builtin_return_address(0) + libcwd::builtin_return_address_offset));
  Dout(dc::ipv6addrdealloc, "ctor(int*, int, char*) "<<(void*)this);
#endif  //__INTEL_COMPILER
  if (addr_seg)
    setAddress(addr_seg); 
}

IPv6Address::IPv6Address(const ipv6_prefix& pref)
  :cObject("ipv6_prefix"), m_addr(IPv6ADDRESS_UNSPECIFIED_STRUCT), m_prefix_length(pref.length),
   m_scope(ipv6_addr::Scope_None), m_storedLifetime(VALID_LIFETIME_INFINITY),
   m_preferredLifetime(VALID_LIFETIME_INFINITY), _updated(false)
{
#if !defined __INTEL_COMPILER
  Dout(dc::ipv6addrdealloc, "ctor(ipv6_prefix) "<<(void*)this);
#endif  //__INTEL_COMPILER
  setAddress(pref.prefix);
  if (m_prefix_length == 0 && m_addr != IPv6_ADDR_UNSPECIFIED)
    m_prefix_length = IPv6_ADDR_LENGTH;
}

IPv6Address::IPv6Address(const char* t, const char *name)
  :cObject(name), m_addr(IPv6ADDRESS_UNSPECIFIED_STRUCT), m_prefix_length(0),
   m_scope(ipv6_addr::Scope_None), m_storedLifetime(VALID_LIFETIME_INFINITY),
   m_preferredLifetime(VALID_LIFETIME_INFINITY), _updated(false)
{
#if !defined __INTEL_COMPILER
  Dout(dc::ipv6addrdealloc, " ctor(char*, char*) "<<(void*)this);
#endif  //__INTEL_COMPILER
  setAddress(t);
  if (m_prefix_length == 0 && m_addr != IPv6_ADDR_UNSPECIFIED)
    m_prefix_length = IPv6_ADDR_LENGTH;
}

IPv6Address::IPv6Address(const ipv6_addr& addr, size_t prefix_len, const char* obj_name)
  :cObject(obj_name), m_addr(IPv6ADDRESS_UNSPECIFIED_STRUCT), m_prefix_length(prefix_len),
   m_scope(ipv6_addr::Scope_None), m_storedLifetime(VALID_LIFETIME_INFINITY),
   m_preferredLifetime(VALID_LIFETIME_INFINITY), _updated(false)
{
#if !defined __INTEL_COMPILER
  Dout(dc::ipv6addrdealloc, "ctor(ipv6_addr&, size_t, char*) "<<(void*)this);
#endif //!defined __INTEL_COMPILER

  setAddress(addr);
  if (m_prefix_length == 0 && m_addr != IPv6_ADDR_UNSPECIFIED)
    m_prefix_length = IPv6_ADDR_LENGTH;
}

IPv6Address::IPv6Address(const IPv6Address& obj):cObject()
{
#if !defined __INTEL_COMPILER
  Dout(dc::ipv6addrdealloc, "copy ctor "<<(void*)this);
#endif //!defined __INTEL_COMPILER

  setName(obj.name());
  IPv6Address::operator=(obj);
}

void IPv6Address::setAddress(unsigned int* addr_seg)
{
  for(int i=0; i<TOTAL_NUMBER_OF_32BIT_SEGMENT; i++)
    m_full_addr[i] = addr_seg[i];
  m_scope = ipv6_addr::Scope_None;  
}

/**
   Parse IPv6 address in fully specified x:x:x:x:x:x:x:x: notation    
   @param t can be 0 to initialise with Unspecified address 
*/
void IPv6Address::setAddress(const char* t)
{
  if (t == NULL)
  {
    m_addr = IPv6ADDRESS_UNSPECIFIED_STRUCT;
    m_prefix_length = 0;
    return;    
  }
  
  stringstream is(t); 
  this->operator>>(is);

  m_scope = ipv6_addr::Scope_None;
}

void IPv6Address::setAddress(const ipv6_addr& addr)
{
  m_addr = addr;
  m_scope = ipv6_addr::Scope_None;  
}

///Want order in maps to be longest prefix first followed by shorter
///prefixes so longest prefix is the least
bool IPv6Address::operator<(const IPv6Address& rhs) const
{
  if (m_prefix_length != rhs.m_prefix_length) 
    return (m_prefix_length < rhs.m_prefix_length)?false:true;
  //Compare prefixes 
  return m_addr < rhs.m_addr;  
}

bool IPv6Address::operator==(const IPv6Address& rhs) const
{
  return m_addr == rhs.m_addr && m_prefix_length == rhs.m_prefix_length &&
    scope() == rhs.scope();
}

bool IPv6Address::operator==(const ipv6_addr& rhs) const
{
  return m_full_addr[0] == rhs.extreme && m_full_addr[1] == rhs.high &&
    m_full_addr[2] == rhs.normal && m_full_addr[3] == rhs.low;
}

std::ostream& IPv6Address::operator<<(std::ostream& os) const
{
  return ::operator<<(os, *this);
}

std::ostream& operator<<(std::ostream& os, const IPv6Address& obj)
{
  (const_cast<IPv6Address&> (obj)).writeContents(os);
  return os;
}

#if defined OPP_VERSION && OPP_VERSION >= 3
#else
cEnvir& operator<<(cEnvir& ev, const IPv6Address& obj)
{
  ev.printf("%s/%d \n", obj.addressSansPrefix().c_str(), obj.m_prefix_length);
  return ev;
}
#endif

IPv6Address& IPv6Address::operator=(const IPv6Address& obj)
{
  cObject::operator=(obj);
  m_prefix_length = obj.m_prefix_length;
/*  m_prefix = obj.m_prefix;*/
  m_addr = obj.m_addr;  	
  m_scope = obj.m_scope;  
  return *this;
};

///When a container of this element is told to writeContents this function is
///invoked to fill the buf with info.  Its a static so be sure that buf is never
///reused like os <<buf<<*this
void IPv6Address::info(char* buf)
{
  ostringstream os;
  os << *this ;
  os << '\0';
  os.str().copy(buf, string::npos);
}


///Can set only IPv6 address
IPv6Address& IPv6Address::operator=(const ipv6_addr& addr)
{
  setAddress(addr);
  return *this;
}

std::istream& IPv6Address::operator>>(std::istream& is)
{
  unsigned int octals[8] = {0, 0, 0, 0, 0, 0, 0, 0} ;  
  char sep = '\0';
  
  //Assume no prefix is specified initially
  m_prefix_length = 0; 

/*
  //Allow uncaught exceptions to propagate and abort program
  try 
  {
*/
    for (int i = 0; i < 8; i ++)
    {
      
      is >> hex >> octals[i];
      if (is.eof() )
        break;      
      is >> sep;
      if (sep == IPv6_PREFIX_SEPARATOR)
        is >> dec >> m_prefix_length;
    }
/*    
  }
  catch (...) 
  {
    ev << "exception thrown while parsing IPv6Address";
  }
*/  
  
  for (int i = 0; i < TOTAL_NUMBER_OF_32BIT_SEGMENT; i++ )
    m_full_addr[i] = (octals[i*2]<<(NUMBER_OF_SEGMENT_BIT/2)) + octals[2*i + 1];

  return is;
  
}

void IPv6Address::writeContents(std::ostream& os)
{
  os << address() << "Scope: " << scope_str();
}

std::string IPv6Address::address(void) const
{
  ostringstream os;
  os << addressSansPrefix() << IPv6_PREFIX_SEPARATOR << dec << m_prefix_length;
  return os.str();
}

bool IPv6Address::isNetwork(const IPv6Address& toCmp) const
{
  if(nbBitsMatching(&toCmp)>=m_prefix_length)
    return true;
  return false;
}

bool IPv6Address::isNetwork(const ipv6_addr& prefix) const
{  
  if(nbBitsMatching(prefix)>=m_prefix_length)
    return true;
  return false;
}

///Unused so will be deprecated or even removed
std::string IPv6Address::lowerNbBits(int nbbits)
{
  unsigned int addr_int[4];

  for(int i = 0; i < 4; i++)
    addr_int[i] = m_full_addr[i];

  int nb_segs = nbbits / 32;
  int nth_bit = nbbits % 32;
  
  for(int i = 0; i < 3 - nb_segs; i++)
    addr_int[i] = 0;

  addr_int[3-nb_segs] = (addr_int[3-nb_segs] << 32 - nth_bit) >> 32 - nth_bit;
    
  return toTextFormat(addr_int);
}

///Truncates everything after nbbits in the address to give a prefix
ipv6_addr IPv6Address::higherNbBits(size_t nbbits)
{
  assert(nbbits <= IPv6_ADDR_LENGTH);
  
  if (nbbits == 0)
    return IPv6_ADDR_UNSPECIFIED;
  if (nbbits == IPv6_ADDR_LENGTH)
    return m_addr;
  
  unsigned int addr_int[TOTAL_NUMBER_OF_32BIT_SEGMENT];

  for(int i = 0; i < TOTAL_NUMBER_OF_32BIT_SEGMENT; i++)
    addr_int[i] = m_full_addr[i];
  
  int nb_segs = nbbits / NUMBER_OF_SEGMENT_BIT;
  int nth_bit = NUMBER_OF_SEGMENT_BIT - nbbits % NUMBER_OF_SEGMENT_BIT;
  
  for (int i = nb_segs; i < TOTAL_NUMBER_OF_32BIT_SEGMENT; i++)
  {
    if (i == nb_segs)
    {  
      addr_int[i] = addr_int[i] >> nth_bit;
      addr_int[i] = addr_int[i] << nth_bit;
      continue;
    }
    addr_int[i] = 0;
  }
  
  ipv6_addr prefix = {addr_int[0], addr_int[1], addr_int[2], addr_int[3]};

  return prefix;
}

size_t IPv6Address::nbBitsMatching(const IPv6Address* to_cmp) const
{
  return calcBitsMatch(m_full_addr, to_cmp->m_full_addr);
}

size_t IPv6Address::nbBitsMatching(const ipv6_addr& prefix) const
{
  const unsigned int addr2[4] = {prefix.extreme, prefix.high, prefix.normal, prefix.low};  
  return calcBitsMatch(m_full_addr, addr2);
}


std::string IPv6Address::toTextFormat(const unsigned int* addr) const
{
  ipv6_addr ipaddr = 
    {
      addr[0], addr[1], addr[2], addr[3]
    };
  ostringstream os;
  os << ipaddr;
  return os.str();
}

const char* IPv6Address::scope_str() const
{
  //Determine scope if necessary
  m_scope = scope();
  
  switch(m_scope)
  {
  case ipv6_addr::Scope_None:
    return "None";
    break;    
  case ipv6_addr::Scope_Node:
    return "Node";
    break;
  case ipv6_addr::Scope_Link:
    return "Link";
    break;
  case ipv6_addr::Scope_Site:
    return "Site";
    break;
  case ipv6_addr::Scope_Organization:
    return "Organization";
    break;
  case ipv6_addr::Scope_Global:
    return "Global";
  }
  return "";
}

/**
 * @defgroup TestCases CppUnit Test Cases
 * @brief Some rough unit test cases and a very rough regression test
 * Enable with -DUSE_CPPUNIT as compiler option
 */
#ifdef USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>

/**
 * @class AddressTest
 * @brief Unit test IPv6Address and ipv6_addr
 * @ingroup TestCases
 */

class AddressTest: public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( AddressTest );
  CPPUNIT_TEST( test );
  CPPUNIT_TEST_SUITE_END();
public:
  
  AddressTest();
  
                           
  void test();

  void setUp();
  void tearDown();
    
private:
  IPv6Address* ip_addr1;
  IPv6Address* ip_addr2;
  
};

CPPUNIT_TEST_SUITE_REGISTRATION( AddressTest );

/**
 * @addtogroup TestCases
 * @{ 
 */

static ipv6_addr addr1 = {0x12345678,0xabcdef00,0x1234,0};
static unsigned int int_addra1[] = {0x12345678,0xabcdef00,0x1234,0};
static unsigned int *int_addr1 = int_addra1;
///@}
  
AddressTest::AddressTest()
  :TestFixture(), ip_addr1(0), ip_addr2(0)
{}
  
                           
void AddressTest::test()
{

  ipv6_addr test_addr_string_initialise = 
    {
0xabcdabcd,0xabcdabcd,0xabcdabcd, 0xabcdabcd
    };

  //Test IPv6Address char* ctor and equals operator(ipv6_addr)
  CPPUNIT_ASSERT(!(ip_addr1->operator==(*ip_addr2)));
  CPPUNIT_ASSERT(*ip_addr1 == test_addr_string_initialise);
  
  //Test IPv6Address ipv6_addr assingment
  *ip_addr2 = test_addr_string_initialise;
  ip_addr2->setPrefixLength(100);  
  CPPUNIT_ASSERT((ip_addr2->operator==(*ip_addr1)));

  //test address struct == int* address
  CPPUNIT_ASSERT( addr1 == int_addr1);

  CPPUNIT_ASSERT(string("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd") == ip_addr1->addressSansPrefix());
  
  ip_addr1->setAddress("1234:5678:abcd:ef00:1234:abcd:efff:ffff/108");
  
  CPPUNIT_ASSERT(ip_addr1->prefixLength() == 108);
  IPv6Address* ip_addr3 = ip_addr1->dup();
  ip_addr1->truncate();
  CPPUNIT_ASSERT(string("1234:5678:abcd:ef00:1234:abcd:eff0:0") == ip_addr1->addressSansPrefix());
  CPPUNIT_ASSERT("1234:5678:abcd:ef00:1234:abcd:efff:ffff" == ip_addr3->addressSansPrefix());
  CPPUNIT_ASSERT(!strcmp("1234:5678:abcd:ef00:1234:abcd:efff:ffff/108", ip_addr3->address().c_str()));
  CPPUNIT_ASSERT("1234:5678:abcd:ef00:1234:abcd:efff:ffff/108" == ip_addr3->address());
  
  ipv6_addr bitwiseAnd = {0xfe800000,0,0x026097ff,0x2};   
  CPPUNIT_ASSERT((bitwiseAnd & IPv6_ADDR_LINK_LOCAL_PREFIX) == IPv6_ADDR_LINK_LOCAL_PREFIX);

  const char* equalString = "Equal Strings";
  string s1 = equalString;
  string s2 = equalString;
  CPPUNIT_ASSERT(s1 == s2);
  CPPUNIT_ASSERT(!strcmp(equalString, "Equal Strings"));

  const std::string addr1String("1234:5678:abcd:ef00:1234:abcd:eff0:0/108Scope: None");
  ostringstream os, os2;
  os<<*ip_addr1;  
  CPPUNIT_ASSERT(!strcmp(os.str().c_str(), addr1String.c_str()));
  CPPUNIT_ASSERT(os.str() == addr1String);

  const std::string addr2String("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd/100Scope: Global");
  os2<<*ip_addr2;
  CPPUNIT_ASSERT(os2.str() == addr2String);

  //Test char* initialisation
  ipv6_addr test = c_ipv6_addr("abcd:1234:fefe:abde:1234:0:7789:9999");
  ostringstream testos;
  testos<<test;
  CPPUNIT_ASSERT("abcd:1234:fefe:abde:1234:0:7789:9999" == testos.str()); 
  CPPUNIT_ASSERT(test == IPv6Address("abcd:1234:fefe:abde:1234:0:7789:9999"));
}
  
void AddressTest::setUp()
{
  ip_addr1 = new IPv6Address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd/100");
  ip_addr2 = new IPv6Address("ef00:abcd:ef00:ffff:0:0f0f:0:0/30");

}
void AddressTest::tearDown()
{
  delete ip_addr1;
  delete ip_addr2;  
}

#endif //USE_CPPUNIT
