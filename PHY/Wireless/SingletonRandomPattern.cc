// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/PHY/Wireless/Attic/SingletonRandomPattern.cc,v 1.1 2005/02/09 06:15:58 andras Exp $
//
// Copyright (C) 2001, 2003 CTIE, Monash University
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
	@file SingletonRandomPattern.cc
	@brief Header file for SingletonRandomPattern

	Responsibilities:
        - mobility handling

  Initially implemented in PHYWirelessModule (now obsolete and 
  evolved into PHYSimple), but taken out since it makes more sense 
  as a seperate entity rather than part of the physical layer

  @author Eric Wu
*/

#include <cassert>

#include "SingletonRandomPattern.h"

RandomPattern* RandomPattern::_instance = 0;
int RandomPattern::refCount = 0;
int RandomPattern::currentRefCounter = 0;

RandomPattern* RandomPattern:: initializePattern()
{
  if (_instance == 0)
    _instance = new RandomPattern;
  
  _instance->refCount++;
  _instance->currentRefCounter = refCount - 1;
  return _instance;
}

double RandomPattern::wayPoint(int& x, int& y)
{
  if ( currentRefCounter == refCount - 1)
  {
    // calculate point
    _nextInterval = randomWaypoint(x, y);
    
    _x = x;
    _y = y;

    currentRefCounter = 0;
  }
  else
  {
    currentRefCounter++;

    x = _x;
    y = _y;
  }
  return _nextInterval;
}

RandomPattern::RandomPattern()
  : _x(0), _y(0), _nextInterval(0)
{}
