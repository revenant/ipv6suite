// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/PHY/Mobility/Attic/SingletonRandomPattern.h,v 1.1 2005/02/15 02:25:40 andras Exp $
//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2001 CTIE, Monash University
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
    @file SingletonRandomPattern.h
    @brief Header file for MobilityRandomPattern

    Responsibilities:
             - mobility handling

    @author Eric Wu, Steve Woon
*/

#ifndef __SINGLETON_RANDOMPATTERN_H__
#define __SINGLETON_RANDOMPATTERN_H__

#include "randomWP.h"

class RandomPattern : public RandomWP
{
  friend class MobilityRandomPattern;
public:
  // call this function at the initialiation of a cModule instance
  static RandomPattern* initializePattern();

  // use this function when obtaining the pointer to the RandomPatter
  static RandomPattern* instance() { return _instance; }

  double wayPoint(int& x, int& y);

private:
  RandomPattern();

  static int refCount;
  static int currentRefCounter;

  int _x, _y;
  double _nextInterval;

  static RandomPattern* _instance;
};

#endif

