// -*- C++ -*-
//
// Copyright (C) 2002, 2004, 2005 CTIE, Monash University
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

/**
   @file IPv6Encapsulation.h
   @brief Manage Encapsulation of packets

   Responsibilities:
     - set Tunnel destination address depending on the tunnel src address
     - Encapsulate in IPv6 datagram extension header
     - send new IPInterfacePacket to IPSend to be newly encapsulated
     - Configure and remove encapsulating targets
   @author Johnny Lai
   @date 04.03.02
*/

#ifndef IPv6ENCAPSULATION_H
#define IPv6ENCAPSULATION_H

//hash_map doesn't exist on TRU64 will have to change to std::map or find
//a substitute hash_map.
#if (defined __GNUC__ && __GNUC__ >= 3) || __INTEL_COMPILER >= 810
#include <ext/hash_map>
#else
#if defined CXX
#ifndef MAP
#define MAP
#include <map>
#endif //MAP
#else
#include <hash_map>
#endif //CXX
#endif // __GNUC__ && __GNUC__ >= 3 || __INTEL_COMPILER >= 810

#ifndef STD_FUNCTIONAL
#define STD_FUNCTIONAL
#include <functional>
#endif //STD_FUNCTIONAL

#ifndef BOOST_SHARED_PTR_HPP
#include <boost/shared_ptr.hpp>
#endif //BOOST_SHARED_PTR_HPP

#ifndef IPv6_ADDR_H
#include "ipv6_addr.h"
#endif //IPv6_ADDR_H
#ifndef IPV6HEADERS_H
#include "IPv6Headers.h" //IPv6_MIN_MTU
#endif //IPV6HEADERS_H
#ifndef IPV6ADDRESS_H
#include "IPv6Address.h"
#endif //IPV6ADDRESS_H

#ifndef BOOST_FUNCTION_HPP
#include <boost/function.hpp>
#endif

class RoutingTable6;
class InterfaceTable;

namespace IPv6NeighbourDiscovery
{
  class NeighbourEntry;
}

class IPv6Datagram;

/**
   @class IPv6Encapsulation
   @brief Handles the encapsulation of IPv6 Datagrams within IPv6Datagrams

   Allows multiple encapsulations (nested)

   //XXX this is in fact tunneling, not encapsulation as in IPv6Send... --AV

 */
class IPv6Encapsulation : public cSimpleModule
{
  struct Tunnel;

/**
   @struct Tunnel
   @brief Definition of a tunnel used as a virtual interface

   The virtual interface is identified by an ifIndex, stored as the key in
   IPv6Encapsulation::Tunnels. It is also available inside the
   IPv6NeighbourDiscovery::NeighbourEntry ne member itself. To use ne only to
   store an ifIndex may be a excessive but it matches well with the NC/DC
   paradigm.
 */
  struct Tunnel
  {
    Tunnel(const ipv6_addr& src = IPv6_ADDR_UNSPECIFIED,
           const ipv6_addr& dest = IPv6_ADDR_UNSPECIFIED,
           size_t oifIndex = UINT_MAX, bool forOnePkt = false);

    ~Tunnel();

    bool operator==(const Tunnel& rhs)
      {
        return entry == rhs.entry && exit == rhs.exit;
      }

    ///entry point of tunnel
    ipv6_addr entry;
    ///exit point of tunnel
    ipv6_addr exit;

    ///outgoing interface for entry point address
    size_t ifIndex;
    /// 6.7 usually the PATH MTU of tunnel - size of headers to be created
    //perhaps should be a fn that queries the rt destination entry PMTU
    size_t tunnelMTU;

    ///IPv6NeighbourDiscovery::NeighbourEntry for this tunnel. The vifIndex is
    ///in here. When destination A is linked with this tunnel the DE for A will
    ///point to ne.
    boost::shared_ptr<IPv6NeighbourDiscovery::NeighbourEntry> ne;

    ///Flag to indicate destruction of tunnel after a packet is sent. Needed as
    ///RR procedure requires the same packet to go via tunnel and direct and
    ///after binding established all data packets are direct.
    bool forOnePkt;
  };

  friend std::ostream& operator<<(std::ostream& os,
                                  const IPv6Encapsulation::Tunnel& tun);

  friend std::ostream& operator<<(std::ostream& os,
                                  const pair<const size_t, Tunnel> & p);
  friend std::ostream& operator<<(std::ostream & os,
                                  const IPv6Encapsulation& tunMod);

public:
  Module_Class_Members(IPv6Encapsulation, cSimpleModule, 0);

  ///Create tunnel and create lookup entries by inserting into Dest Cache
  size_t createTunnel(const ipv6_addr& src, const ipv6_addr& dest,
                    size_t ifIndex,
                      const ipv6_addr& destTrigger = IPv6_ADDR_UNSPECIFIED,
                      bool forOnePkt = false);
  ///Remove tunnel and the associated entries in Dest Cache
  bool destroyTunnel(const ipv6_addr& src, const ipv6_addr& dest);
  ///Remove tunnel and the associated entries in Dest Cache
  bool destroyTunnel(size_t vIfIndex);

  ///Returns the vIfIndex of tunnel if found, 0 otherwise
  size_t findTunnel(const ipv6_addr& src, const ipv6_addr& dest);
  ///Forwards all datagrams destined for dest to virtual tunnel
  bool tunnelDestination(const ipv6_addr& dest, size_t vIfIndex);
  /// called from modified XML parser, trying to support prefixes
  bool tunnelDestination(const IPv6Address& dest, size_t vIfIndex);

  typedef boost::function<void (IPv6Datagram*)> MIPv6TunnelCallback;
  /**
   * @brief Register a callback required by MIPv6 to test if packets are been
   * encapsulated from HA or CN.
   *
   * Required so MIPv6 can take corresponding action (usually sending a BU to
   * that node)
   *
   * @param cb a callback func that accepts IPv6Datagram as argument. This
   * object can be modified but not deleted. Ownership of cb is taken by this
   * class.
   */
  void registerCB(MIPv6TunnelCallback cb);

  ///@name Overidden cSimpleModule functions
  //@{
  int numInitStages() const;

  virtual void initialize(int stageNo);

  virtual void handleMessage(cMessage* msg);

  virtual void finish();
  //@}

private:
  InterfaceTable *ift;
  RoutingTable6 *rt;

  simtime_t delay;
  ///traffic class (-1 for copied, 0 for default and others preconfigured)
  int trafficClass;
  ///hoplimit (0 for defaults)
  size_t tunHopLimit;
  ///not implemented 6.6
  int encapLimit;
  typedef
/*
#if defined __GNUC__
#if __GNUC_PREREQ(3,1)
  __gnu_cxx::hash_map<size_t, struct Tunnel, __gnu_cxx::hash<size_t> >
#else
  std::hash_map<size_t, struct Tunnel, std::hash<size_t> >
#endif //__GNUC_PREREQ(3,1)
#else
#if defined CXX || __INTEL_COMPILER >= 810 || defined _MSC_VER
*/
  //must be bug in icc 8.1 that prevents compilation of hash_map when it used to work
  std::map<std::size_t, struct Tunnel>
/*
#else
  //Intel icc < 8.1 and anything besides gcc/Compaq cxx
  std::hash_map<std::size_t, struct Tunnel>
#endif //CXX || __INTEL_COMPILER > 810
#endif // __GNUC__
*/
  Tunnels;

  typedef Tunnels::iterator TI;
  ///Actual tunnels are stored here indexed by ifIndex
  Tunnels tunnels;
  ///The lowest vifIndex assigned so far. Virtual ifIndexes are assigned
  ///downwards.
  size_t vIfIndexTop;

  struct equalTunnel:public binary_function<Tunnels::value_type,
                     Tunnels::value_type, bool>
  {
    bool operator()(const Tunnels::value_type& lhs,
                    const Tunnels::value_type& rhs) const
      {
        return lhs.second.entry == rhs.second.entry &&
          lhs.second.exit == rhs.second.exit;
      }
  };

  ///handle for callback function @see registerCB for more
  ///information
  MIPv6TunnelCallback mipv6CheckTunnelCB;
};

std::ostream& operator<<(std::ostream & os, const IPv6Encapsulation::Tunnel& tun);
std::ostream& operator<<(std::ostream & os, const IPv6Encapsulation& tunMod);

#endif // IPv6ENCAPSULATION_H

