// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/NetworkInterfaces/Wireless/Attic/WirelessEtherSignal.h,v 1.1 2005/02/09 06:15:58 andras Exp $
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
    @file WirelessEtherSignal.h
    @brief WirelessEthernet control signal (also for simulation wise)
    @author Eric Wu
*/


#ifndef __WIRELESS_ETHER_SIGNAL_H
#define __WIRELESS_EHTER_SIGNAL_H

#include <cassert>
#include <string>

#include "EtherSignal.h"
#include "PHYWirelessSignal.h"

class WirelessEtherBasicFrame;

class WESignal : public EtherSignal, PHYWirelessSignal
{
 public:
  WESignal(EtherSignalType type = WIRELESS_EST_None, int c = -1, double p = -1);
  WESignal(const WESignal &p);
  ~WESignal() {};

  // assignment operator
  WESignal& operator=(const WESignal& p);
  virtual WESignal *dup() const { return new WESignal(*this); }
    
  // info functions
  virtual const char *className() const { return "WESignal"; }
  virtual void info(char *buf) {}
  virtual void writeContents(std::ostream& os) {}

  virtual int channel() const { return PHYWirelessSignal::channel(); }
  virtual double power() const { return PHYWirelessSignal::power(); }
  std::string sourceName() const { return srcName; }

  virtual void setChannel(int c) { PHYWirelessSignal::setChannel(c); }
  virtual void setPower(double p) { PHYWirelessSignal::setPower(p); }
  void setSourceName(std::string n) { srcName = n; }

 private:
  std::string srcName;
};

class WESignalIdle : public WESignal
{
 public:
  WESignalIdle(int c = -1, double p = -1);
  WESignalIdle(const WESignalIdle &p);
  ~WESignalIdle() {};

  // assignment operator
  WESignalIdle& operator=(const WESignalIdle& p);
  virtual WESignalIdle *dup() const { return new WESignalIdle(*this); }
    
  // info functions
  virtual const char *className() const { return "WESignalIdle"; }
  virtual void info(char *buf) {}
  virtual void writeContents(std::ostream& os) {}
};

class WESignalData : public WESignal
{
 public:
  WESignalData(WirelessEtherBasicFrame* frame = 0, int c = -1, double p = -1); 
  WESignalData(const WESignalData &p);
  ~WESignalData();

  // assignment operator
  WESignalData& operator=(const WESignalData& p);
  virtual WESignalData *dup() const { return new WESignalData(*this); }
    
  // info functions
  virtual const char *className() const { return "WESignalData"; }
  virtual void info(char *buf) {}
  virtual void writeContents(std::ostream& os) {}

  WirelessEtherBasicFrame* data();

  void encapsulate(WirelessEtherBasicFrame* frame);

 private:
  WirelessEtherBasicFrame* _frame;
};

#endif
