//  -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/NetworkInterfaces/Ethernet6/Attic/MACAddress.h,v 1.2 2005/02/09 08:04:27 andras Exp $
// Monash University, Melbourne, Australia

/**
 *  File:    MACAddress.h
 *
 *  Purpose: Represention of an Ethernet MAC address
 *
 *  Author:  Eric Wu
 *
 *  Date:    08/11/2001
 */

#ifndef __MAC_ADDRESS_H__
#define __MAC_ADDRESS_H__

#ifndef __COBJECT_H
#include <cobject.h>
#endif //__COBJECT_H

#include <iosfwd> //istream/ostream
#ifndef BOOST_OPERATORS_HPP
#include <boost/operators.hpp>
#endif //BOOST_OPERATORS_HPP

/**
 * @struct MAC_address
 * @brief 48 bit MAC address
 *
 * raw data type used bye MACAddress

 * @warning encoding of the 48 bits is as follows the first 24 bits in high and
 * the next 24 bits in low. Not very intuitive.
 */

struct MAC_address
{
  unsigned int high;
  unsigned int low;
};

std::ostream& operator<<( std::ostream& os, const MAC_address& src_addr);
bool operator==(const MAC_address& lhs, const MAC_address& rhs);
bool operator!=(const MAC_address& lhs, const MAC_address& rhs);

extern const MAC_address MAC_ADDRESS_UNSPECIFIED_STRUCT;

/**
 * @class MACAddress
 * @brief IEEE802.3 MAC address object
 * @sa MAC_address
 */

class MACAddress : public cObject, boost::equality_comparable<MACAddress>
{
 public:
  // Constructor
  MACAddress(void):mac_addr(MAC_ADDRESS_UNSPECIFIED_STRUCT){}
  MACAddress(const char* addr);
  MACAddress(const MAC_address& addr);
  MACAddress(const MACAddress& obj);

  ///@name cObject functions redefined
  //@{
  virtual const char* className() const { return "MACAdress"; }
  virtual cObject *dup() const { return new MACAddress(*this); }
  virtual void info(char *buf);
  virtual void writeContents(std::ostream& os);
  //@}

  friend std::ostream& operator<<(std::ostream& os, const MACAddress& obj);

  // operator functions
  MACAddress& operator=(const MACAddress& obj);
  operator MAC_address() const;
  operator const char*();

  bool operator==(const MACAddress& rhs)
    {
      return mac_addr == rhs.mac_addr;
    }


  std::istream& operator>>(std::istream& is);


  // set Value
  void set(const char* addr);
  void set(const MAC_address& addr);

  // get Value
  const char* stringValue(void) const;
  const unsigned int* intValue(void)
  {
    m_int_addr[0] = mac_addr.high;
    m_int_addr[1] = mac_addr.low;
    return m_int_addr;
  }

private:
  union
  {
    MAC_address mac_addr;
    unsigned int m_int_addr[2];
  };
};

#endif
