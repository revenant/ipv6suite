// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/NetworkInterfaces/Ethernet6/Attic/EtherSignal.cc,v 1.3 2005/02/16 00:48:30 andras Exp $
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
    @file EtherSignal.cc
    @brief Ethernet control signal (also for simulation wise)
    @author Eric Wu
*/

#include <cassert>
#include "EtherSignal.h"
#include "EtherFrame6.h"

EtherSignal::EtherSignal(EtherSignalType type)
  : _type(type)
{}

EtherSignal::EtherSignal(const EtherSignal& p)
{
  setName(p.name());
  operator=(p);
}

EtherSignal& EtherSignal::operator=(const EtherSignal& p)
{
    cMessage::operator=(p);
    _type = p._type;
    srcModPathName = p.srcModPathName;
    setName(p.name());
    return *this;
}

EtherSignalData::EtherSignalData(EtherFrame6* frame)
  : EtherSignal(EST_Data), _frame(frame->dup())
{
  setName("FRAME");
  setKind(static_cast<int>(EST_Data));
  setLength(frame->length() * 8); // convert into bits
}

EtherSignalData::EtherSignalData(const EtherSignalData& p)
{
  operator=(p);
}

EtherSignalData::~EtherSignalData()
{
  assert(_frame);
  delete _frame;
}

EtherSignalData& EtherSignalData::operator=(const EtherSignalData& p)
{
  assert(p._frame);
  EtherSignal::operator=(p);
  setName(p.name());

  _frame = static_cast<EtherFrame6*>(p._frame->dup());
  return *this;
}

EtherFrame6* EtherSignalData::data()
{
  return _frame;
}

EtherSignalJam::EtherSignalJam(void)
  : EtherSignal(EST_Jam)
{
  setName("JAM");
  setKind(static_cast<int>(EST_Jam));
}

EtherSignalJam::EtherSignalJam(const EtherSignalJam& p)
{
  operator=(p);
}

EtherSignalJam& EtherSignalJam::operator=(const EtherSignalJam& p)
{
  EtherSignal::operator=(p);
  setName(p.name());
  return *this;
}

EtherSignalJamEnd::EtherSignalJamEnd(void)
  : EtherSignal(EST_JamEnd)
{
  setName("JAMEND");
  setKind(static_cast<int>(EST_JamEnd));
}

EtherSignalJamEnd::EtherSignalJamEnd(const EtherSignalJamEnd& p)
{
  operator=(p);
}

EtherSignalJamEnd& EtherSignalJamEnd::operator=(const EtherSignalJamEnd& p)
{
  EtherSignal::operator=(p);
  setName(p.name());

  return *this;
}

EtherSignalIdle::EtherSignalIdle(void)
  : EtherSignal(EST_Idle)
{
  setName("IDLE");
  setKind(static_cast<int>(EST_Idle));
}

EtherSignalIdle::EtherSignalIdle(const EtherSignalIdle& p)
{
  operator=(p);
}

EtherSignalIdle& EtherSignalIdle::operator=(const EtherSignalIdle& p)
{
    EtherSignal::operator=(p);
    setName(p.name());
    return *this;
}
