// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Util/Attic/TimerConstants.cc,v 1.1 2005/02/09 06:15:58 andras Exp $
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
 * @file   TimerConstants.cc
 * @author Johnny Lai
 * @date   07 Jun 2004
 * 
 * @brief Stores a constant used to overcome the restriction of message sending
 * from different module
 *
 * 
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"    
#include "debug.h"

#include "TimerConstants.h"


const double SELF_SCHEDULE_DELAY = 0.000001;

/* Self schedule delays of 0 never yield to other modules.  1ns is a close
 * approximation to zero. If other events have similar "magnitude" then this
 * needs to be reduced by at least 10e3 to simulate "zero" delay.
 */
const double ZERO_WAIT_DELAY =  0.000000001;


