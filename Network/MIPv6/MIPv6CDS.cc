// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/MIPv6/MIPv6CDS.cc,v 1.1 2005/02/09 06:15:58 andras Exp $
// Copyright (C) 2002 CTIE, Monash University 
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
 * @file   MIPv6CDS.cc
 * @author Johnny Lai
 * @date   19 Apr 2002
 * 
 * @brief 
 * 
 * @todo Implement mainly Binding Cache interface which is common to both 
 * HomeAgents and Mobile Nodes however there may be differences so that reuse is
 * inappropriate
 * 
 */


#include "MIPv6CDS.h"
#include "MIPv6Entry.h"
#include "cTTimerMessageCB.h"
#include "MIPv6Timers.h"

using std::rand;

namespace MobileIPv6
{

  MIPv6CDS::MIPv6CDS():tunMod(0)
  {
    home_token.high = rand();
    home_token.low = rand();
        
    careof_token.high = rand();
    careof_token.low = rand();
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

  TFunctorBaseA<cTimerMessage>* MIPv6CDS::setupLifetimeManagement()
  {
    return makeCallback(this, &MIPv6CDS::expireLifetimes);
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

