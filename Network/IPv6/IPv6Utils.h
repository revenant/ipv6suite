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
   @file opp_utils.h
   @brief Global utility functions
   @author Johnny Lai
 */

#ifndef __IPV6UTILS_H__
#define __IPV6UTILS_H__

/**
   @brief print packet header contents on stdout
   @arg routingInfoDisplay Display dgram's headers when true. Should pass
   routingInfoDisplay parameter from IPv6ForwardCore module.
   @arg dgram datagram to retrieve information from
   @arg name name of network node
   @arg directionOut Hint on whether the dgram is egressing the node
*/
namespace IPv6Utils
{
  void printRoutingInfo(bool routingInfoDisplay, IPv6Datagram* dgram, const char* name, bool directionOut);
};

#endif //__IPV6UTILS_H__

