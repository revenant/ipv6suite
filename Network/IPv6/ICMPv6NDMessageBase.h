// -*- C++ -*-
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
    @file ICMPv6NDMessageBase.h

    @brief Abstract base class definitions for ICMP Neighbour Discovery
    messages.
    @author Johnny Lai
    @date 14.9.01


*/

#if !defined ICMPV6NDMESSAGEBASE_H
#define ICMPV6NDMESSAGEBASE_H

#include <omnetpp.h>
#include <string>

#include "ICMPv6Message.h"
#include "ipv6_addr.h"
#include "ICMPv6NDOptionBase.h"

class IPv6Datagram;

namespace IPv6NeighbourDiscovery
{

/**
   @class ICMPv6NDMessageBase

   @brief This is a template base class which holds common functionality for all
   Neigbour Discovery (ND) messages.

   @ingroup ICMPv6NDMsgs

   The template parameter n_addr specifies how many addresses are part of the ND
   message and the n_opts specify the maximum number of possible options.  The
   better way (if I knew how) to do handle options would be to specify exactly
   which options are allowed as a template parameter instead of holding pointers
   to the Option base class and just specifying the number.

 */

template<size_t n_addrs, size_t n_opts>
class ICMPv6NDMessageBase:public ICMPv6Message
{
public:

  /**
     Ensure that removal of options always zeroes out
     the pointers in the opts array.
  */
  virtual ~ICMPv6NDMessageBase();
  void removeOptions();

  bool hasOptions() const
    {
      return opts[0] != 0;
    }

/**
   @name Overridden ICMPv6Message functions
 */
//@{
  ///Force decendents to provide function
  virtual ICMPv6NDMessageBase* dup() const = 0;
//@}

  ///Returns the number of possible options but not the number of options
  ///in use i.e. a RtrAd message has a pointer to srcLLAddr but it is not in use
  virtual size_t optionCount() const
    {
      return n_opts;
    }

protected:

  bool hasLLAddr(size_t i) const { return !LLAddress(i).empty(); }

  void setAddress(const ipv6_addr& addr, size_t i)
    {
      assert( i < n_addrs);
      addrs[i] = addr;
    }

  ipv6_addr address(size_t i) const
    {
      assert(i < n_addrs);
      return addrs[i];
    }

  /// return link layer address
  std::string LLAddress(size_t index = 0) const;

  ///Length is set at 1 (8 octet units) as assuming Link Layer address is
  ///Ethernet
  void setLLAddress(bool source, const string& addr, int len = 1, size_t index = 0);

protected:

  ICMPv6NDMessageBase(const ICMPv6Type& otype, const ICMPv6Code& ocode = 0);
  ICMPv6NDMessageBase(const ICMPv6NDMessageBase& src);

  const ICMPv6NDMessageBase& operator=(const ICMPv6NDMessageBase& src);
  bool operator==(const ICMPv6NDMessageBase& rhs) const;

  //TODO make private and provide get/set func so len is updated automatically
  //instaed of having to do it manually
  ICMPv6_NDOptionBase* opts[n_opts];
private:

  void init();

  ipv6_addr addrs[n_addrs];

};


#ifndef USE_MOBILITY
typedef ICMPv6NDMessageBase<1,2> ICMPv6NDMsgBaseRtrAd;
#else
// with new advertisement interval for mobility support
typedef ICMPv6NDMessageBase<1,3> ICMPv6NDMsgBaseRtrAd;
#endif
typedef ICMPv6NDMessageBase<1,1> ICMPv6NDMsgBseRtrSol;
typedef ICMPv6NDMessageBase<1,1> ICMPv6NDMsgBaseNgbrSol;
typedef ICMPv6NDMessageBase<1,1> ICMPv6NDMsgBaseNgbrAd;
typedef ICMPv6NDMessageBase<2,2> ICMPv6NDMsgBaseRedirect;


};
#if !defined EXPORT_KEYWORD_DEFINED
#include "ICMPv6NDMessageBase.cc"
#endif
#endif //ICMPV6NDMESSAGEBASE_H
