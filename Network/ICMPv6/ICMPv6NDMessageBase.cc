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
   @file ICMPv6NDMessageBase.cc
    
   @brief Abstract base class definitions for ICMP Neighbour Discovery
   messages.    
   @author Johnny Lai
   @date 14.9.01

*/

#include "ICMPv6Message.h"
#include "ICMPv6NDMessageBase.h"
#include "IPv6Datagram.h"
#include "Constants.h"
#include "ICMPv6NDOptions.h"

#if !defined ICMPv6NDMESSAGEBASE_CC
#define ICMPv6NDMESSAGEBASE_CC

using namespace IPv6NeighbourDiscovery;

template<size_t n_addrs, size_t n_opts>
void ICMPv6NDMessageBase<n_addrs, n_opts>::removeOptions()
{
  for (size_t i = 0; i < n_opts; i++)
  {
    if (opts[i] != 0)
      setLength(length() - opts[i]->length()*ICMPv6_OCTETS_UNIT);
        
    delete opts[i];
    opts[i] = 0;        
  }
}

template<size_t n_addrs, size_t n_opts>
const ICMPv6NDMessageBase<n_addrs, n_opts>& ICMPv6NDMessageBase<n_addrs, n_opts>::operator=(const ICMPv6NDMessageBase<n_addrs, n_opts>& src)
{
  if (this != &src)
  {    
    removeOptions();
  
    ICMPv6Message::operator=(src);
  
    for (size_t i = 0; i < n_opts; i++)
      if (src.opts[i] != 0)
        opts[i] = src.opts[i]->dup();  

    std::copy(src.addrs, src.addrs + n_addrs, addrs);
  }
  
  return *this;  
}

template<size_t n_addrs, size_t n_opts>
bool ICMPv6NDMessageBase<n_addrs, n_opts>::operator==(const ICMPv6NDMessageBase<n_addrs, n_opts>& rhs) const
{
  if (this == &rhs)
    return true;
  
  if (!ICMPv6Message::operator==(rhs))
    return false;
  
  for (size_t i = 0; i < n_opts; i++)
    if (rhs.opts[i] == opts[i] && opts[i] == 0)
      continue;
    else if (rhs.opts[i] != opts[i] && (opts[i] == 0 || rhs.opts[i] == 0))
      return false;
    else //Neither are 0
    {
      if (*opts[i] != *rhs.opts[i])
      {
        return false;
        
//          switch(opts[i]->type())
//          {
//            case  NDO_SRC_LINK_LAYER_ADDR: case NDO_DEST_LINK_LAYER_ADDR:
//              if (*((ICMPv6NDOptLLAddr*)opts[i]) != *((ICMPv6NDOptLLAddr*)rhs.opts[i]))
//                return false;            
//              break;              
//            case NDO_PREFIX_INFO:
//              //if (*((ICMPv6NDOptPrefix*)opts[i]) != *((ICMPv6NDOptPrefix*)rhs.opts[i]))
//              //  return false;            
//              break;
//            case NDO_REDIRECTED_HEADER:
//              //if (*((ICMPv6NDOptRedirect)opts[i]) != *((ICMPv6NDOptRedirect)rhs.opts[i]))
//              //  return false; 
//              break;
//            case NDO_MTU:
//              //if (*((ICMPv6NDOptMTU)opts[i]) != *((ICMPv6NDOptMTU)rhs.opts[i]))
//              //  return false; 
//              break;            
//          }        
      }
      else
        return false;            
    }
  return true;  
}


   




/*export*/ template<size_t n_addrs, size_t n_opts>
ICMPv6NDMessageBase<n_addrs, n_opts>::ICMPv6NDMessageBase(const ICMPv6Type& otype, const ICMPv6Code& ocode)
  :ICMPv6Message(otype, ocode)
{
  init();      
}

template<size_t n_addrs, size_t n_opts>
inline ICMPv6NDMessageBase<n_addrs, n_opts>::ICMPv6NDMessageBase(const ICMPv6NDMessageBase& src)
  :ICMPv6Message(src)
{
  setName(src.name());
  operator=(src);  
}

template<size_t n_addrs, size_t n_opts>
inline ICMPv6NDMessageBase<n_addrs, n_opts>::~ICMPv6NDMessageBase()
{
      for (size_t i = 0; i < n_opts; i++)
        delete opts[i];      
}

template<size_t n_addrs, size_t n_opts>
inline void ICMPv6NDMessageBase<n_addrs, n_opts>::setLLAddress(bool source, const string& addr, int len, size_t index)  
{
  assert(index < n_addrs);
  assert(addr != "");
  
  if (opts[index] == 0)      
  {        
    opts[index] = new ICMPv6NDOptLLAddr(source, addr, len);
    addLength(opts[index]->length()*8);        
  }
  else        
    (static_cast<ICMPv6NDOptLLAddr*> (opts[index]))->setAddress(addr);      
}

template<size_t n_addrs, size_t n_opts>
inline std::string ICMPv6NDMessageBase<n_addrs, n_opts>::LLAddress(size_t index ) const
{
  assert(index < n_opts);      
  if (opts[index] == 0)
    return "";      
  return (static_cast<ICMPv6NDOptLLAddr*> (opts[index]))->address();
}

template<size_t n_addrs, size_t n_opts>
inline void ICMPv6NDMessageBase<n_addrs, n_opts>::init()
{
  for(size_t i = 0; i < n_opts; i++)
    opts[i] = 0;  
  for(size_t i = 0; i < n_addrs; i++)
    addrs[i] = IPv6_ADDR_UNSPECIFIED;
}

#endif //ICMPv6NDMESSAGEBASE_CC
