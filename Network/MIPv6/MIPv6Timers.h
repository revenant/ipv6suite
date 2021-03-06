// -*- C++ -*-
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
 * @file MIPv6Timers.h
 * @author Johnny Lai
 * @date 06 Jun 2002
 *
 * @brief Timer classes that require declarations in more than one
 * implementation file
 *
 */

#ifndef MIPV6TIMERS_H
#define MIPV6TIMERS_H 1



#if !defined CSIGNALMESSAGE_H
#include "cSignalMessage.h"
#endif

class IPv6Mobility;

namespace MobileIPv6
{
  /**
   * @class MIPv6PeriodicCB
   * @brief Simple periodic timer to invoke callback repeatedly
   *
   */

  class MIPv6PeriodicCB: public cSignalMessage
  {
  public:
    MIPv6PeriodicCB(IPv6Mobility* mob, unsigned int interval);

    virtual void callFunc();

    unsigned int interval;
    IPv6Mobility* mob;
  };

}

#endif /* MIPV6TIMERS_H */
