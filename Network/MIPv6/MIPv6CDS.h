// -*- C++ -*-
// Copyright (C) 2002, 2005 CTIE, Monash University
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
 * @file MIPv6CDS.h
 * @author Johnny Lai
 * @date 19 Apr 2002
 * @brief Ownership of MIPv6 Conceptual data structures and interfaces to them
 */

#ifndef MIPV6CDS_H
#define MIPV6CDS_H 1

#ifndef MAP
#define MAP
#include <map>
#endif //MAP

#ifndef BOOST_UTILITY_HPP
#include <boost/utility.hpp>
#endif //BOOST_UTILITY_HPP
#ifndef BOOST_WEAK_PTR_HPP
#include <boost/weak_ptr.hpp>
#endif //BOOST_WEAK_PTR_HPP

#ifndef MIPv6MOBILITYHEADERS_H
#include "MIPv6MobilityHeaders.h"
#endif // MIPv6MOBILITYHEADERS_H

#include <iosfwd>

namespace IPv6NeighbourDiscovery
{
  class NeighbourEntry;
}

template <class Arg> class TFunctorBaseA;
class cTimerMessage;
struct ipv6_addr;
class IPv6Encapsulation;

namespace MobileIPv6
{
  struct bc_entry;

  /**
   * @class MIPv6CDS
   * @brief Mobile IPv6 Conceptual Data Structures
   *
   * Defined in Sec 4.7 of MIPv6 Draft 16
   */

  class MIPv6CDS: public boost::noncopyable
  {
  public:

    //@name constructors, destructors and operators
    //@{
    MIPv6CDS();
    virtual ~MIPv6CDS();

    //@}

    ///Returns the preferred lifetime management function to be called at every
    ///period
    virtual TFunctorBaseA<cTimerMessage>* setupLifetimeManagement();

    ///Remove the binding associated with homeAddr
    bool removeBinding(const ipv6_addr& homeAddr);

    ///Locate binding cache entry by it's home addres
    boost::weak_ptr<bc_entry> findBinding(const ipv6_addr& homeAddr);

    ///Locate binding cache entry by it's care-of addres
    boost::weak_ptr<bc_entry> findBindingByCoA(const ipv6_addr& careofAddr);

    ///Replaces the existing binding entry or creates a new association
    boost::weak_ptr<bc_entry> insertBinding(bc_entry* entry);

    std::ostream& operator<<(std::ostream& os) const;

    ///Used to create/destroy tunnels for mobility bindings
    IPv6Encapsulation* tunMod;

    const bit_64& token(MIPv6MobilityHeaderType& ht)
      {
        if (ht==MIPv6MHT_HoT)
          return home_token;
        else if (ht==MIPv6MHT_CoT)
          return careof_token;
        assert(false);
      }

    typedef std::map<ipv6_addr, boost::shared_ptr<bc_entry> > BindingCache;
    typedef BindingCache::iterator  BCI;

  protected:
    ///Decrement by elapsed time in tmr for all entities with managed lifetimes
    void expireLifetimes(cTimerMessage* tmr);

    ///Returns true if found and stores the found object in it false otherwise
    bool findBinding(const ipv6_addr& homeAddr, BCI& it);

    ///Returns true if found and stores the found object in it false otherwise
    bool findBindingByCoA(const ipv6_addr& careofAddr, BCI& it);

    BindingCache bc;

    bit_64 home_token;
    bit_64 careof_token;
  private:

  };
  std::ostream& operator<<(std::ostream& os, const MobileIPv6::MIPv6CDS& mipv6cds);
} //namespace MobileIPv6

#endif /* MIPV6CDS_H */
