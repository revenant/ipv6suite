//
// Copyright (C) 2002 CTIE, Monash University
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
 * @file   MIPv6Timers.cc
 * @author Johnny Lai
 * @date   06 Jun 2002
 *
 * @brief  Implementation of timers used in MIPv6Mobility module
 *
 *
 */


#include "MIPv6Timers.h"
#include "IPv6Mobility.h"

namespace MobileIPv6
{

MIPv6PeriodicCB::MIPv6PeriodicCB(IPv6Mobility* mob, unsigned int interval)
  :cSignalMessage("MIPv6Lifetime", Tmr_MIPv6Lifetime), interval(interval)
{
  rescheduleDelay(interval);
}

void MIPv6PeriodicCB::callFunc()
{
  cSignalMessage::callFunc();
  ///chain another call for next period
  rescheduleDelay(interval);
}

}

