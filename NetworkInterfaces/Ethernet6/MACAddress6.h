//  -*- C++ -*-
//
// Monash University, Melbourne, Australia

/**
 *  File:    MACAddress6.h
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
 * raw data type used bye MACAddress6

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
 * @class MACAddress6
 * @brief IEEE802.3 MAC address object
 * @sa MAC_address
 */

class MACAddress6 : public /*XXX cObject,*/ boost::equality_comparable<MACAddress6>
{
 public:
  // Constructor
  MACAddress6(void):mac_addr(MAC_ADDRESS_UNSPECIFIED_STRUCT){}
  MACAddress6(const char* addr);
  MACAddress6(const MAC_address& addr);
  MACAddress6(const MACAddress6& obj);

  ///@name cObject functions redefined
  //@{
  virtual const char* className() const { return "MACAdress"; }
  virtual MACAddress6 *dup() const { return new MACAddress6(*this); }
  virtual std::string info();
  virtual void writeContents(std::ostream& os);
  //@}

  friend std::ostream& operator<<(std::ostream& os, const MACAddress6& obj);

  // operator functions
  MACAddress6& operator=(const MACAddress6& obj);
  operator MAC_address() const;
  operator const char*();

  bool operator==(const MACAddress6& rhs)
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
