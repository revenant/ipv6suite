//  -*- C++ -*-
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
 *  @file    IPv6Address.h
 *
 *  @brief Represention of an IPv6 address
 *
 *  @author  Eric Wu
 *
 *  @date   22/8/2001
 *
 */

#ifndef IPv6ADDRESS_H
#define IPv6ADDRESS_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif // __OMNETPP_H

#ifndef IOSFWD
#include <iosfwd>
#define IOSFWD
#endif //IOSFWD

#ifndef CASSERT
#include <cassert>
#define CASSERT
#endif //CASSERT

#ifndef BOOST_OPERATORS_HPP
#include <boost/operators.hpp>
#endif //BOOST_OPERATORS_HPP

#if defined __CN_PAYLOAD_H
extern "C"{
#include <netinet/in.h>
};
#endif //__CN_PAYLOAD_H

#ifndef IPv6_ADDR_H
#include "ipv6_addr.h"
#endif //IPv6_ADDR_H

#ifndef STRING
#define STRING
#include <string>
#endif //STRING

/**
 * @class IPv6Address

 * @brief cObject derived class used for storing the IPv6 address including the
 * prefix length.
 *
 * Used where ever an IPv6 address is required in packets and also internally in
 * many modules.  Should move towards either ipv6_addr for non prefix related
 * tasks or ipv6_prefix.
 */

class IPv6Address : public cObject, boost::equality_comparable<IPv6Address>
{
public:

#if defined OPP_VERSION && OPP_VERSION >= 3
//Unnecessary as cEnvir is a std::ostream now
#else
  friend cEnvir& operator<<(cEnvir& ev, const IPv6Address& obj);
#endif

  ///@name Constructors, Destructors and operators
  //@{
  IPv6Address(unsigned int* addr = 0, int prefix_len = 0, const char *n = NULL);
  IPv6Address(const char* t, const char *n = NULL);
  IPv6Address(const IPv6Address& obj);
  explicit IPv6Address(const ipv6_addr& addr,
                       size_t prefix_len = IPv6_ADDR_LENGTH,
                       const char* obj_name = NULL);
  explicit IPv6Address(const ipv6_prefix& pref);

  virtual ~IPv6Address()
    {
#if defined UNITTESTOUTPUT
      cerr<<name()<<" addr deleted="<<this<<endl;
#endif //UNITTESTOUTPUT
    }

  IPv6Address& operator=(const IPv6Address& obj);
  IPv6Address& operator=(const ipv6_addr& src);

  std::istream& operator>>(std::istream& is);
  std::ostream& operator<<(std::ostream& os) const;

  bool operator<(const IPv6Address& rhs) const;
  bool operator==(const IPv6Address& rhs) const;
  bool operator==(const ipv6_addr& rhs) const;

  operator ipv6_addr() const
    {
      return m_addr;
    }

  ipv6_addr::SCOPE scope() const
    {
      //Scope is determined only when it hasn't been determined before
      //The setAddress function resets this flag back to Scope_None
      //Example of Lazy Evaluation
      if (m_scope == ipv6_addr::Scope_None)
      {
        m_scope = ipv6_addr_scope(m_addr);
      }
      return m_scope;
    }


  //@}

  ///@name cObject functions redefined
  //@{
  virtual const char* className() const { return "IPv6Address"; }
  virtual IPv6Address* dup() const { return new IPv6Address(*this); }
  virtual std::string info();
  virtual void writeContents(std::ostream& os);
  //@}

  ///@name Attribute set functions
  //@{
  void setPrefixLength(int prefix_len){m_prefix_length = prefix_len;}
  void setAddress(unsigned int* addr);
  void setAddress(const char* t);
  void setAddress(const ipv6_addr& addr);

  void setPreferredLifetime(unsigned int seconds)
    { m_preferredLifetime = seconds;}

  ///Sets the updated flag so RoutingTable6 will not elapse its lifetime
  void setStoredLifetimeAndUpdate(unsigned int validLifetime)
    {
      setStoredLifetime(validLifetime);
      _updated = true;
    }
  ///Sets the validLifetime only
  void setStoredLifetime(unsigned int validLifetime)
    {
      m_storedLifetime = validLifetime;
    }

  ///Resets updated flag to false so the lifetime on this address can elapse
  ///again
  void updatedLifetime()
    {
      assert(updated());
      _updated = false;
    }
  bool updated() { return _updated; }

  //@}

  ///@name Attribute get functions
  //@{
  /// returns full address/prefix
  std::string address(void) const;

  /// returns full address without prefix length in string
  std::string addressSansPrefix(void) const
    {
      return toTextFormat(m_full_addr);
    }

  /// returns prefix address in array of integers each integer element
  /// corresponds to one 32-bit IPv6 address segment
  const unsigned int* getFullInt(void) const
    {
      return m_full_addr;
    }

  size_t prefixLength(void) const { return m_prefix_length; }

  /// Conversion function from enum to string
  const char* scope_str() const;

  unsigned int preferredLifetime() { return m_preferredLifetime; }

  unsigned int storedLifetime() { return m_storedLifetime; }
  //@}

  /// According to Section 2.7 RFC 2373
  bool isMulticast() const
    {
      return m_addr.isMulticast();
    }

  /// Indicates if the address is from the same network
  bool isNetwork(const IPv6Address& toCmp) const;

  bool isNetwork(const ipv6_addr& prefix) const;

  /// Indicates how many bits from the to_cmp address, starting counting
  /// from the left, matches the address.
  size_t nbBitsMatching(const IPv6Address* to_cmp) const;

  size_t nbBitsMatching(const ipv6_addr& prefix) const;

  ///returns the lower bits of the address
  std::string lowerNbBits(int nbbits);
  ///returns the higher bits of the address
  ipv6_addr higherNbBits(size_t nbbits);

  ///Creates the solicited node address corresponding to addr
  static ipv6_addr solNodeAddr(const ipv6_addr& addr)
    {
      ipv6_addr retAddr =
      {
        (size_t)0xFF02<<16, 0x0, 0x1, addr.low | (size_t)0xFF<<24
      };

      return retAddr;
    }

  ///Returns the solicted node address for this address
  ipv6_addr solNodeAddr()
    {
      return solNodeAddr(m_addr);
    }

  ///Truncates the address to the prefix length
  void truncate()
    {
      if (m_prefix_length > 0 && m_prefix_length <IPv6_ADDR_LENGTH)
        m_addr = higherNbBits(m_prefix_length);
      //assert(m_prefix_length >= 0 && m_prefix_length<=IPv6_ADDR_LENGTH);
    }

private:

  std::string toTextFormat(const unsigned int* addr) const;

  ///@name Internal data members
  //@{
  union
  {
    struct ipv6_addr m_addr;
    unsigned int m_full_addr[4];
  };

  unsigned int m_prefix_length;

  /// IPv6 addressing scope
  mutable ipv6_addr::SCOPE m_scope;
  //@}

  ///@name AddrConf timer impl
  //@{
  /// Preferred and Valid lifetimes Used in Address Configuration
  unsigned int m_storedLifetime;
  unsigned int m_preferredLifetime;
  /// Indicates to Addr Conf whether this address has just been changed so don't
  /// elapse the lifetimes on it
  bool _updated;
  //@}
};

#if defined OPP_VERSION && OPP_VERSION >= 3
//Unnecessary as cEnvir is a std::ostream now
#else
cEnvir& operator<<(cEnvir& ev, const ipv6_addr& addr);
#endif  // defined OPP_VERSION && OPP_VERSION >= 3

bool operator==(const ipv6_addr& lhs, const IPv6Address& rhs);
IPv6Address operator+(const IPv6Address& lhs, const IPv6Address& rhs);
//Opp3 likes to overide all operator so it looks like we'll have to put into class
std::ostream& operator<<(std::ostream& os, const IPv6Address& obj);

#ifdef CXX
//using std::rel_ops::operator!=;
#endif //CXX

#endif // IPv6ADDRESS_H

