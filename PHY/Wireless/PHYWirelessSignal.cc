// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/PHY/Wireless/Attic/PHYWirelessSignal.cc,v 1.1 2005/02/09 06:15:58 andras Exp $
//
// Eric Wu
// Copyright (C) 2001 Monash University, Melbourne, Australia
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

/*
    @file PHYWirelessSignal.cc
    @brief Wireless control signal ("Non-cMessage" class)
    @author Eric Wu
*/

#include "PHYWirelessSignal.h"

PHYWirelessSignal::PHYWirelessSignal(int c, double p)
  : ch(c), pwr(p)
{}

PHYWirelessSignal::PHYWirelessSignal(const PHYWirelessSignal& p)
{
  operator=(p);
}

PHYWirelessSignal& PHYWirelessSignal::operator=(const PHYWirelessSignal& p)
{
    ch = p.ch;
    pwr = p.pwr;

	return *this;
}
