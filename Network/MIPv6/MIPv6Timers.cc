// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/MIPv6/MIPv6Timers.cc,v 1.1 2005/02/09 06:15:58 andras Exp $
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

MIPv6PeriodicCB::MIPv6PeriodicCB(IPv6Mobility* mob,
                                 TFunctorBaseA<cTimerMessage>* cb,
                                 unsigned int interval)
  :cTTimerMessageCBA<cTimerMessage, void>(Tmr_MIPv6Lifetime, mob, cb,
                                          "MIPv6Lifetime"),
   interval(interval)
{
  rescheduleDelay(interval);
}
    
void MIPv6PeriodicCB::callFunc()
{ 
  cTTimerMessageCBA<cTimerMessage, void>::callFunc();
  ///chain another call for next period
  rescheduleDelay(interval);
}
 
}

