// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/PHY/Mobility/Attic/MobileBaseStation.cc,v 1.1 2005/02/09 06:15:58 andras Exp $
//
// Copyright (C) 2001, 2002 CTIE, Monash University
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
   @file MobileBaseStation.cc

   A class that handles BS operations such as assigning channels for
   mobile entities

   Author Eric Wu
 */

#include "MobileBaseStation.h"
#include "opp_utils.h"

using namespace::OPP_Global;

BaseStation::BaseStation(cSimpleModule* mod)
  : Entity(mod)
{
  _type = MobileBS;
}
