// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/PHY/Wireless/Attic/PHYWirelessSignal.h,v 1.1 2005/02/09 06:15:58 andras Exp $
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
    @file PHYWirelessSignal.h
    @brief Wireless control signal ("Non-cMessage" class)
    @author Eric Wu
*/


#ifndef __PHY_WIRELESS_SIGNAL_H
#define __PHY_WIRELESS_SIGNAL_H

#include <cassert>
#include "wirelessethernet.h"

class PHYWirelessSignal
{
public:
  // c = channel 
  // p = power level (in mW)
  PHYWirelessSignal(int c = INVALID_CHANNEL, double p = INVALID_POWER);
  PHYWirelessSignal(const PHYWirelessSignal& p);

  // assignment operator
  PHYWirelessSignal& operator=(const PHYWirelessSignal& p);    

  virtual int channel() const { return ch; }
  virtual double power() const { return pwr; }
  virtual void setChannel(int c) { assert(c > 0); ch = c; }
  virtual void setPower(double p) { pwr = p; }

 protected:
  int ch;
  double pwr;
};

#endif
