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
   @file opp_utils.cc
   @brief Global utility functions
   @author Johnny Lai
 */


#include "sys.h"
#include "debug.h"
#include <cstring>

#include <omnetpp.h>
#include "IPv6Datagram.h"

namespace IPv6Utils
{
  void printRoutingInfo(bool routingInfoDisplay, IPv6Datagram* datagram, const char* name, bool directionOut)
  {
    if (routingInfoDisplay)
    {
      cout<<name<<" "<<(directionOut?"-->":"<--")<<" "<<simulation.simTime()<<" src="<<datagram->srcAddress()<<" dest="
          <<datagram->destAddress()<<" len="<<datagram->length()<<endl;
      cout.flush();
    }
  }
}
