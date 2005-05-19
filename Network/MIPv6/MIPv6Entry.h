// -*- C++ -*-
//
// Copyright (C) 2001, 2005 CTIE, Monash University
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
    @file MIPv6Entry.h
	@brief binding cache entry (bc_entry) and MIPv6Destination Entry structures
    @see draft-ietf-mobileip-ipv6-16.txt, section 4.6
    @author Eric Wu
    @date 2/4/2002
*/

#ifndef MIPv6ENTRY_H
#define MIPv6ENTRY_H

#ifndef MAP
#define MAP
#include <map>
#endif //MAP

#ifndef LIST
#define LIST
#include <list>
#endif //LIST

#ifndef BOOST_SHARED_PTR_HPP
#include <boost/shared_ptr.hpp>
#endif //BOOST_SHARED_PTR_HPP

#ifndef IPv6_ADDR_H
#include "ipv6_addr.h"
#endif //IPv6_ADDR_H
#ifndef IPV6NDENTRY_H
#include "NDEntry.h"
#endif //IPV6NDENTRY_H


//using namespace std;

namespace MobileIPv6
{
  /**
   * @defgroup MIPv6CDS Mobile IPv6 Conceptual Data Structures
   *
   * Reference from MIPv6 Draft 16 Sec. 4.7
   *
   */
  //@{

  /**
   * @struct bc_entry
   * @brief Binding Cache entry
   *
   * Cache Replacement policy can remove any bc_entry except when is_home_reg is
   * set.  In such cases only when the entry expires can it be removed.
   *
   * @note Receipt of a Home Address Option cannot modify bc_entry.
   */

  struct bc_entry
  {
    ipv6_addr home_addr;
    ipv6_addr care_of_addr;

    /// remaining lifetime
    unsigned long expires;

    /// a flag to indicate if the entry is a home registration entry
    bool is_home_reg;

    ///maximum received Sequence Number in previous binding update
    unsigned int seq_no;

    ///Placeholder for the BSA for authenticating BU and calculating BA
    int bsa;

    //@name Valid only when is_home_reg is true
    //@{

    ///Mobile router is not supported yet
    bool isRouter;

    ///prefix length of home_addr
    unsigned char prefix_len;
    //@}

    //@name Cell Residency Signaling parameters
    //@{
    simtime_t prevBUTime; 
    simtime_t avgCellResidenceTime;
    simtime_t buArrivalTime;
    //@}
  };

  std::ostream& operator<< (std::ostream& os, const bc_entry& bce);

} //end namespace MobileIPv6
#endif // MIPv6ENTRY
