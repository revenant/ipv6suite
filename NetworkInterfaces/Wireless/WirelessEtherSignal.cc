/*
// -*- C++ -*-
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

/ *
    @file WirelessEtherSignal.cc
    @brief WirelessEthernet control signal (also for simulation wise)
    @author Eric Wu
* /

#include "WirelessEtherSignal.h"
#include "WirelessEtherFrame_m.h"

WESignal::WESignal(WirelessEtherSignalType type, int c, double p)
  : cMessage(), PHYWirelessSignal(c, p) // XXX!!!!!!!!!!!!!!!!!!!!!!!!!, _type(type)
{
  setName("WIRELESS_NONE");
  setKind(static_cast<int>(type));
}

WESignal::WESignal(const WESignal& p)
{
  operator=(p);
}

WESignal& WESignal::operator=(const WESignal& p)
{
  cMessage::operator=(p);
  PHYWirelessSignal::operator=(p);

  setSourceName(p.sourceName());

  return *this;
}

// Note that a copy of frame is made. Otherwise, multiple modules may delete
// the same frame and cause memory access problems. However, the programmer needs to
// be extra careful when writing code to prevent memory leaks.
WESignalData::WESignalData(WirelessEtherBasicFrame* frame, int c, double p)
  : WESignal(WIRELESS_EST_Data, c, p), _frame(static_cast<WirelessEtherBasicFrame*>(frame->dup()))
{
  if(frame)
  {
    if(frame->getFrameControl().subtype==ST_ACK)
      setName("ACK");
    else
    {
      if(frame->encapsulatedMsg())
        setName(frame->encapsulatedMsg()->className());
    }
  }

  setKind(static_cast<int>(WIRELESS_EST_Data));
  setLength(frame->length()*8); // convert into bits
}

WESignalData::WESignalData(const WESignalData& p)
{
  setName(p.name());
  operator=(p);
}

WESignalData::~WESignalData()
{
  assert(_frame);
  delete _frame;
}

WESignalData& WESignalData::operator=(const WESignalData& p)
{
  assert(p._frame);
  WESignal::operator=(p);

  _frame = static_cast<WirelessEtherBasicFrame*>(p._frame->dup());
  return *this;
}

WirelessEtherBasicFrame* WESignalData::data()
{
  return _frame;
}

void WESignalData::encapsulate(WirelessEtherBasicFrame* frame)
{
  cMessage::encapsulate(frame);
  if(frame->getFrameControl().subtype==ST_ACK)
    setName("ACK");
  else
    setName(frame->encapsulatedMsg()->className());
}

WESignalIdle::WESignalIdle(int c, double p)
  : WESignal(WIRELESS_EST_Idle, c, p)
{
  setName("WIRELESS_IDLE");
  setKind(static_cast<int>(WIRELESS_EST_Idle));
}

WESignalIdle::WESignalIdle(const WESignalIdle& p)
{
  setName(p.name());
  operator=(p);
}

WESignalIdle& WESignalIdle::operator=(const WESignalIdle& p)
{
  WESignal::operator=(p);

  return *this;
}
*/
