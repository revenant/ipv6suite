//  -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/ipv6_addr.h,v 1.1 2005/02/09 06:15:58 andras Exp $
// Monash University, Melbourne, Australia

/**
 *  @file    ipv6_addr.h
 *
 *  @brief Represention of an IPv6 address
 *
 *  @author Johnny Lai
 *
 *  @date   22/8/2001
 *
 */

#ifndef IPv6_ADDR_H
#define IPv6_ADDR_H

#ifndef IOSFWD
#include <iosfwd>
#define IOSFWD
#endif //IOSFWD

#ifndef STRING
#include <string> //ipv6_addr_toString
#define STRING
#endif //STRING

#ifndef BOOST_OPERATORS_HPP
#include <boost/operators.hpp>
#endif //BOOST_OPERATORS_HPP

#ifndef BOOST_ARRAY_HPP
#include <boost/array.hpp>
#endif //BOOST_ARRAY_HPP

#if defined __CN_PAYLOAD_H
extern "C"{
#include <netinet/in.h>
};
#endif //__CN_PAYLOAD_H


using namespace std;


extern const size_t IPv6_ADDR_LENGTH;
extern const unsigned int EUI64_LENGTH;

extern const char* LOOPBACK_ADDRESS;
extern const char* ALL_NODES_NODE_ADDRESS;
extern const char* ALL_NODES_LINK_ADDRESS;
extern const char* ALL_NODES_SITE_ADDRESS;

extern const char* ALL_ROUTERS_NODE_ADDRESS;
extern const char* ALL_ROUTERS_LINK_ADDRESS;
extern const char* ALL_ROUTERS_SITE_ADDRESS;

extern const char* LINK_LOCAL_PREFIX;
extern const char* SOLICITED_NODE_PREFIX;

struct ipv6_addr;
extern const ipv6_addr IPv6_ADDR_UNSPECIFIED;
extern const ipv6_addr IPv6_ADDR_MULTICAST_PREFIX;
extern const ipv6_addr IPv6_ADDR_MULT_NODE_PREFIX;
extern const ipv6_addr IPv6_ADDR_MULT_LINK_PREFIX;
extern const ipv6_addr IPv6_ADDR_MULT_SITE_PREFIX;
extern const ipv6_addr IPv6_ADDR_MULT_ORGN_PREFIX;
extern const ipv6_addr IPv6_ADDR_MULT_GLOB_PREFIX;
extern const ipv6_addr IPv6_ADDR_LINK_LOCAL_PREFIX;
extern const ipv6_addr IPv6_ADDR_SITE_LOCAL_PREFIX;
extern const ipv6_addr IPv6_ADDR_GLOBAL_PREFIX;

extern const unsigned int NLA_BITSHIFT;
extern const unsigned int SLA_BITSHIFT;
extern const unsigned int TLA_BITSHIFT;

extern const ipv6_addr IPv6_ADDR_TLA_MASK;
extern const ipv6_addr IPv6_ADDR_NLA_MASK;
extern const ipv6_addr IPv6_ADDR_SLA_MASK;

//defined in NDEntry.cc
extern const unsigned int VALID_LIFETIME_INFINITY ;


/**
 * @struct ipv6_addr
 * @brief Basic representation of IPv6 address
 */
struct ipv6_addr
{
  
  /**
   * @enum SCOPE
   * Only the 'well
   * known' scopes specified in RFC2373 are implemented.
   * Node exists only for multicast addresses
   */
  enum SCOPE
  {
    Scope_None = 0,
    Scope_Node = 1,         // 0x01
    Scope_Link = 2,         // 0x02
    Scope_Site = 5,         // 0x03
    Scope_Organization = 8, // 0x08
    Scope_Global = 14       // 0x0E
  };

  ///MSB order
  unsigned int extreme;
  unsigned int high;
  unsigned int normal;
  unsigned int low;

  bool isMulticast() const
    {
      return (extreme & IPv6_ADDR_MULTICAST_PREFIX.extreme) ==
        IPv6_ADDR_MULTICAST_PREFIX.extreme?true:false;      
    }  

#if defined __CN_PAYLOAD_H
  struct in6_addr *in6_nwaddr(struct in6_addr *sixaddress) const
  {
     if(!sixaddress){ 
        return NULL; 
     }
     sixaddress->s6_addr32[0]= htonl( extreme);
     sixaddress->s6_addr32[1]= htonl( high);
     sixaddress->s6_addr32[2]= htonl( normal);
     sixaddress->s6_addr32[3]= htonl( low);
     return sixaddress; 
  }
#endif //__CN_PAYLOAD_H

  ipv6_addr& operator+=(unsigned int number);
  ipv6_addr& operator+=(const ipv6_addr& rhs);
  ipv6_addr& operator>>=(unsigned int shift);
  ipv6_addr& operator<<=(unsigned int shift);
  ipv6_addr& operator|=( const ipv6_addr& rhs);
  ipv6_addr& operator&=( const ipv6_addr& rhs);  

};

ipv6_addr c_ipv6_addr(const char* addr);
typedef boost::array<unsigned int,4> ipv6_addr_array;
ipv6_addr_array ipv6_addr_toArray(const ipv6_addr& addr);

string ipv6_addr_toString(const ipv6_addr& addr);
string ipv6_addr_toBinary(const ipv6_addr& addr);
string longToBinary(unsigned long long no);

bool operator==(const ipv6_addr& lhs, const ipv6_addr& rhs);
bool operator!=(const ipv6_addr& lhs, const ipv6_addr& rhs);

bool operator<(const ipv6_addr& lhs, const ipv6_addr& rhs);
ipv6_addr operator&(const ipv6_addr& lhs, const ipv6_addr& rhs);
ipv6_addr operator|(const ipv6_addr& lhs, const ipv6_addr& rhs);
ipv6_addr operator>>(const ipv6_addr&, unsigned int shift);
ipv6_addr operator<<(const ipv6_addr&, unsigned int shift);
ipv6_addr operator+(const ipv6_addr& lhs, const ipv6_addr& rhs);


ostream& operator<<( ostream& os, const ipv6_addr& src_addr);

size_t calcBitsMatch(const unsigned int* addr1, const unsigned int* addr2);
size_t calcBitsMatch(const ipv6_addr_array& addr1, const ipv6_addr_array& addr2);

/** 
 * Determine the topological hierarchy that the address belongs to.
 * 
 * @param m_addr address to determine the scope for
 * 
 * @return SCOPE for m_addr
 *
 * @warning Does not test conformity to the non multicast site local format
 * i.e. the next 38 bits are indeed 0
 */
inline ipv6_addr::SCOPE ipv6_addr_scope(const  ipv6_addr& m_addr)
{
  if (!m_addr.isMulticast())
  {
    if ((m_addr & IPv6_ADDR_SITE_LOCAL_PREFIX) ==
        IPv6_ADDR_SITE_LOCAL_PREFIX)
      return ipv6_addr::Scope_Site;

    if ((m_addr & IPv6_ADDR_LINK_LOCAL_PREFIX) == 
        IPv6_ADDR_LINK_LOCAL_PREFIX)
      return ipv6_addr::Scope_Link;
        
    if ((m_addr & IPv6_ADDR_GLOBAL_PREFIX) ==
        IPv6_ADDR_GLOBAL_PREFIX)
      return ipv6_addr::Scope_Global;

    if (m_addr == c_ipv6_addr(LOOPBACK_ADDRESS))
      return ipv6_addr::Scope_Node;
  }
  else
  {
    if ((m_addr.extreme & IPv6_ADDR_MULT_LINK_PREFIX.extreme) ==
        IPv6_ADDR_MULT_LINK_PREFIX.extreme)
      return ipv6_addr::Scope_Link;
          
    if ((m_addr.extreme & IPv6_ADDR_MULT_SITE_PREFIX.extreme) ==
        IPv6_ADDR_MULT_SITE_PREFIX.extreme)
      return ipv6_addr::Scope_Site;
         
    if ((m_addr.extreme & IPv6_ADDR_MULT_NODE_PREFIX.extreme) == 
        IPv6_ADDR_MULT_NODE_PREFIX.extreme)
      return ipv6_addr::Scope_Node;

    if ((m_addr.extreme & IPv6_ADDR_MULT_ORGN_PREFIX.extreme) ==
        IPv6_ADDR_MULT_ORGN_PREFIX.extreme)
      return ipv6_addr::Scope_Organization; 

    if ((m_addr.extreme & IPv6_ADDR_MULT_GLOB_PREFIX.extreme) ==
        IPv6_ADDR_MULT_GLOB_PREFIX.extreme)
      return ipv6_addr::Scope_Global;
  }

  //We need a proper address validater.  Currently all unit tests use lots of
  //bogus addresses which don't fit in any of the scopes.
  //assert(false);
  return ipv6_addr::Scope_None;
}

/**
 * @class ipv6_prefix
 *
 * @brief Replacement for most of IPv6Address functionality.  

 * This should be used internally for efficiency reasons and also to clarify
 * intent i.e. whether an address itself is needed or prefix length too.
 * 
 */
class ipv6_prefix: boost::equality_comparable<ipv6_prefix>
{
public:
  
  ///construct a prefix from addr with prefixLen.  No attempt is made to 
  ///truncate addr to the specified prefixLen
  ipv6_prefix(const ipv6_addr& addr, unsigned int prefixLen)
    :prefix(addr), length(prefixLen)
    {}
  
  ipv6_prefix()
    :prefix(IPv6_ADDR_UNSPECIFIED), length(IPv6_ADDR_LENGTH)
    {}
  
  ///Returns true when length bits in this prefix matches addr
  bool matchPrefix(const ipv6_addr& addr) const;

  bool operator==(const ipv6_prefix& rhs) const
  {
    return length == rhs.length && prefix == rhs.prefix;
  }
  
  //ipv6_addr prefix;
  union
  {
    struct ipv6_addr prefix;
    unsigned int m_full_addr[4];	  
  };

  unsigned int length;  
};

ostream& operator<<( ostream& os, const ipv6_prefix& src_addr);

#endif //IPv6_ADDR_H

