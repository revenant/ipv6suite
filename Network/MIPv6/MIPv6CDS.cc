//
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
 * @file   MIPv6CDS.cc
 * @author Johnny Lai
 * @date   19 Apr 2002
 *
 * @brief Binding Cache implementation
 */


#include "MIPv6CDS.h"
#include "MIPv6Entry.h"
#include "MIPv6Timers.h"
#include "stlwatch.h"
#include <iostream>

using std::rand;

std::ostream& operator<<(std::ostream& os, const MobileIPv6::MIPv6CDS::BindingCache::value_type& p)
{
  os<<"key="<<p.first<<" entry: "<<*(p.second);
  return os;
}

std::ostream& MobileIPv6::MIPv6CDS::operator<<(std::ostream& os) const
{
  copy(bc.begin(), bc.end(), ostream_iterator<MIPv6CDS::BindingCache::value_type >(os, "\n"));
  return os;
}

namespace MobileIPv6
{

std::ostream& operator<<(std::ostream& os, const MobileIPv6::MIPv6CDS& mipv6cds)
{
  return mipv6cds.operator<<(os);
}

  MIPv6CDS::MIPv6CDS():tunMod(0), mipv6cdsMN(0), ha(0)
  {
    home_token.high = rand();
    home_token.low = rand();

    careof_token.high = rand();
    careof_token.low = rand();

    WATCH_PTRMAP(bc);
  }

  MIPv6CDS::~MIPv6CDS()
  {}

  boost::weak_ptr<bc_entry> MIPv6CDS::findBinding(const ipv6_addr& homeAddr)
  {
    BCI it;
    if (findBinding(homeAddr, it))
      return it->second;
    return  boost::weak_ptr<bc_entry>();
  }

  boost::weak_ptr<bc_entry> MIPv6CDS::findBindingByCoA(const ipv6_addr& careofAddr)
  {
    BCI it;
    if (findBindingByCoA(careofAddr, it))
      return it->second;
    return  boost::weak_ptr<bc_entry>();
  }

  boost::weak_ptr<bc_entry> MIPv6CDS::insertBinding(bc_entry* entry)
  {
    boost::shared_ptr<bc_entry>& bce = bc[entry->home_addr];
    bce.reset(entry);
    return bce;
  }

  bool MIPv6CDS::removeBinding(const ipv6_addr& homeAddr)
  {
    BCI it;
    if (findBinding(homeAddr, it))
    {
      bc.erase(it);
      return true;
    }

    return false;
  }

  void MIPv6CDS::expireLifetimes(cTimerMessage* tmr)
  {
    //Decrement every BC entry and remove entry if decrement > remaining lifetime
    unsigned int dec = static_cast<MIPv6PeriodicCB*> (tmr)->interval;
    BCI temp;
    for (BCI it = bc.begin(); it != bc.end(); )
    {
      if (it->second->expires > dec)
      {
        it->second->expires -= dec;
        it++;
      }
      else
      {
        temp = it;
        it++;
        bc.erase(temp);
      }

    }
  }

  bool MIPv6CDS::findBinding(const ipv6_addr& homeAddr, BCI& it)
  {
    it = bc.find(homeAddr);
    return it != bc.end();
  }

  bool MIPv6CDS::findBindingByCoA(const ipv6_addr& careofAddr, BCI& it)
  {
    for (it = bc.begin(); it != bc.end(); it++)
    {
      boost::weak_ptr<bc_entry> bce = it->second;
      if (bce.lock().get()->care_of_addr == careofAddr)
        break;
    }
    return it != bc.end();
  }

} //namespace MobileIPv6

