// -*- C++ -*-
//
// Copyright (C) 2002, 2003 CTIE, Monash University
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
 * @file MIPv6CDSHomeAgent.h
 * @author Johnny Lai
 * @date 05 May 2002
 * @brief Conceptual Data Structures for HA
 */

#ifndef MIPV6CDSHOMEAGENT_H
#define MIPV6CDSHOMEAGENT_H 1

#ifndef MIPV6CDS_H
#include "MIPv6CDS.h"
#endif //MIPV6CDS_H

#ifndef MAP
#define MAP
#include <map>
#endif //MAP

#ifndef LIST
#define LIST
#include <list>
#endif //LIST

#ifndef VECTOR
#define VECTOR
#include <vector>
#endif //VECTOR

#ifndef IPV6_ADDR_H
#include "ipv6_addr.h"
#endif //IPV6_ADDR_H


namespace MobileIPv6
{
  struct ha_entry;

  /**
   * @struct ha_entry
   * @brief Home Agent List entry
   *
   */

  struct ha_entry
  {
    ha_entry(int oifIndex,const ipv6_addr& ll_addr, const ipv6_addr& g_addr,
             unsigned long lifetime, int pref = 0)
      :_ifIndex(oifIndex), link_local_addr(ll_addr), _global_addr(g_addr),
       expire(lifetime), _preference(pref)
      {}

    size_t ifIndex() const { return _ifIndex; }

    const ipv6_addr& local_addr() const { return link_local_addr; }

    ///Suppose to keep whole list of them but we'll just keep the first one for
    ///now
    const ipv6_addr& global_addr(size_t i = 0) const
      {
        return _global_addr;
      }

    int global_addr_count() const
      {
        return 1;
      }

    ///Unimplemented
    void addAddress(const ipv6_addr& addr)
      {
        assert(false);
      }


    const int& preference() const { return _preference; }

    void setPreference(int pref) { _preference = pref; }

    void setLifetime(unsigned long lifetime) { expire = lifetime; }

  private:

    size_t _ifIndex;

    /**
     * @brief link local address of router with home agent bit set
     *
     * Obtained from source address of Router Advertisement
     */

    ipv6_addr link_local_addr;

    /**
     * @brief one or more global addresses derived from Prefix Information with
     * Router Prefix bit set set
     *
     * @note These global addresses must be removed once the associated prefix
     * expires
     */
    ipv6_addr _global_addr;

    /// remaining lifetime, decremented until it reaches zero when this home
    /// agent entry is removed from HA List.
    unsigned long expire;

    // preference fo this home agent; higher values indicate a more
    // preferable home agent
    int _preference;
  };


/**
 * @class MIPv6CDSHomeAgent
 * @brief CDS for a HomeAgent
 *
 * detailed description
 */

  class MIPv6CDSHomeAgent: public MIPv6CDS
  {
  public:

    //@name constructors, destructors and operators
    //@{
    MIPv6CDSHomeAgent(size_t interfaceCount);

    ~MIPv6CDSHomeAgent();

    //@}

    boost::weak_ptr<ha_entry> findHomeAgent(const ipv6_addr& addr);
    void insertHomeAgent(ha_entry* ha);
    void removeHomeAgent(boost::weak_ptr<ha_entry> ha);

    typedef std::list< boost::weak_ptr<ha_entry> > HomeAgents;
    ///Return a list of home agents belonging to a particular interface
    HomeAgents& getHomeAgents(size_t ifIndex);

    ///Find a homeAgent with matching prefix
    boost::weak_ptr<ha_entry> lookupHomeAgents(size_t ifIndex, ipv6_addr prefix, int prefix_len) const;

  protected:

    MIPv6CDSHomeAgent(const MIPv6CDSHomeAgent& src);

    MIPv6CDSHomeAgent& operator=(MIPv6CDSHomeAgent& src);

    //bool operator==(const MIPv6CDSHomeAgent& rhs);

    typedef std::map<ipv6_addr, boost::shared_ptr<ha_entry> > HomeAgentList;
    typedef HomeAgentList::iterator HALI;
    ///Map of all available home agents (convenient to update their properties)
    HomeAgentList hal;

    typedef vector<HomeAgents> HomeAgentLists;

    ///Need a list to maintain ordering of new homeagents as we move from subnet
    ///to subnet to enable sending of new care of addr to all of them
    HomeAgentLists halists;

  };

} //namespace MobileIPv6

#endif /* MIPV6CDSHOMEAGENT_H */
