// -*- C++ -*-
//
// Copyright (C) 2004 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file TimerConstants.h
 * @author Johnny Lai
 * @date 07 Jun 2004
 *
 * @brief Declaration of constants used in timers
 *
 */

#ifndef TIMERCONSTANTS_H
#define TIMERCONSTANTS_H



/**
 * Delay to be used when scheduling a self message for another module. This is
 * to overcome the message owner problem i.e. you cannot get another module to
 * send a message when that module is not active (in its activity or
 * handleMessage loop)
 *
 */
// XXX ?? --AV
extern const double SELF_SCHEDULE_DELAY;

/**
 * Delay used for handleMessage to simulate activity's version of wait(0).
 * Required otherwise simulation time will not advance.
 */
// XXX ?? --AV
extern const double ZERO_WAIT_DELAY;



#endif /* TIMERCONSTANTS_H */

