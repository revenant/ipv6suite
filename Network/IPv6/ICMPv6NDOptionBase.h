// -*- C++ -*-
// Copyright (C) 2004 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file ICMPv6NDOptionBase.h
 * @author Johnny Lai
 * @date 18 Jul 2004
 *
 * @brief Definitition of abstract base class ICMPv6NDOptionBase for ICMP
 * Neighbour Discovery Options
 *
 * Extracted from ICMPv6NDMessageBase.h to prevent cyclic dependencies detected by
 * gcc34.
 */

#ifndef ICMPV6NDOPTIONBASE_H
#define ICMPV6NDOPTIONBASE_H

#ifndef CASSERT
#define CASSERT
#include <cassert>
#endif //CASSERT

#ifndef BOOST_OPERATORS_HPP
#include <boost/operators.hpp>
#endif //BOOST_OPERATORS_HPP

/**
 *@namespace IPv6NeighbourDiscovery
 *@brief Logical grouping of classes that implement NeigbhourDiscovery protocol.
 */

namespace IPv6NeighbourDiscovery
{
  
///Possible ND options
enum ICMPv6_NDOptType
{
  NDO_SRC_LINK_LAYER_ADDR = 1,
  NDO_DEST_LINK_LAYER_ADDR = 2,
  NDO_PREFIX_INFO = 3,
  NDO_REDIRECTED_HEADER = 4,
  NDO_MTU = 5,

  // Mobile IPv6 options
  NDO_ADV_INTERVAL = 7,
  NDO_HOME_AGENT_INFO = 8,

  // Hierarchical MIPv6 options
  NDO_MAP = 10 // extact value TBA
};


/**
 * @defgroup ICMPv6NDOptions ICMPv6 Neighbour Discovery Options
 * Options used in ICMPv6 Neighbour Discovery messages
 * @{ 
 */

/**
   @class ICMPv6_NDOptionBase
   @brief Base class from which all ND option types descend from
 */
class ICMPv6_NDOptionBase: boost::equality_comparable<ICMPv6_NDOptionBase>
{
public:
  
  virtual ~ICMPv6_NDOptionBase()
    {}
  
  ///Pure virtual functions which subclasses will have to override
  virtual ICMPv6_NDOptionBase* dup() const = 0;
  
  bool operator==(const ICMPv6_NDOptionBase& rhs) const
    {
      //Forego the equality of length for now
      return _type == rhs._type;
    }
  
///@name Attributes
//@{
  
  ICMPv6_NDOptType type(){ return _type; }
  
  /**
     This length is different from classes derived from cMessage as 
     is in units of 8 octets.  Refer to RFC 2461 Sec. 4.6
  */
  int length()
    { return _len; }
  /**
     This length is different from classes derived from cMessage as 
     is in units of 8 octets
  */
  void setLength(int len)
    {
      assert(len <=255);
      _len = len;
    }

//@}

protected:
  ICMPv6_NDOptionBase(ICMPv6_NDOptType type, int len)
    :_type(type), _len(len)
    { assert(len <=255); }

private:
  
  ICMPv6_NDOptType _type;
  ///Units of 8 octets
  unsigned char _len;
  
};
/** @} */


};

#endif /* ICMPV6NDOPTIONBASE_H */

