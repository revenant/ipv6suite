// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/HMIPv6/HMIPv6MStateMAP.h,v 1.1 2005/02/09 06:15:58 andras Exp $
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
 * @file HMIPv6MStateMAP.h
 * @author Johnny Lai
 * @date 05 Sep 2002
 * @brief Processes MAP registration Binding Updates from MN
 */

#ifndef HMIPV6MSTATEMAP_H
#define HMIPV6MSTATEMAP_H 1

#ifndef MIPV6MSTATEHOMEAGENT_H
#include "MIPv6MStateHomeAgent.h"
#endif //MIPV6MSTATEHOMEAGENT_H

class IPv6Mobility;
class IPv6Datagram;

namespace MobileIPv6
{  
  class MIPv6MHBindingUpdate;
}

using MobileIPv6::MIPv6MHBindingUpdate;

namespace HierarchicalMIPv6
{

/**
 * @class HMIPv6MStateMAP
 * @brief Implementation of MAP registration.  For basic mode it is the same as
 * a Home Agent?
 */

class HMIPv6MStateMAP: public MobileIPv6::MIPv6MStateHomeAgent
{
 public:

  static HMIPv6MStateMAP* instance();
  
  virtual ~HMIPv6MStateMAP();
  
protected:

  virtual bool processBU(IPv6Datagram* dgram, MIPv6MHBindingUpdate* bu,
                         IPv6Mobility* mod);

  static HMIPv6MStateMAP* _instance;

  //@name constructors, destructors and operators
  //@{
   HMIPv6MStateMAP();
  
  //@}

 private:
  
};

} //namespace HierarchicalMIPv6

#endif /* HMIPV6MSTATEMAP_H */

