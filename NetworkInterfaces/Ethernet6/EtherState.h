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

/*
    @file EtherState.h
    @brief Header file for EtherState

    Defines simple FSM for Ethernet operation based on "Efficient and
    Accurate Ethernet Simulation" by Jia Wang and Srinivasan Keshav

    @author Eric Wu */

#ifndef __ETHER_STATE_H__
#define __ETHER_STATE_H__

#include <memory> //auto_ptr

#ifndef BOOST_CAST_HPP
#include <boost/cast.hpp>
#endif //BOOST_CAST_HPP

#include <omnetpp.h>

#include "ethernet.h"

class EtherModule;

class EtherSignalData;
class EtherSignalJam;
class EtherSignalJamEnd;
class EtherSignalIdle;

// Super class of Ethernet state

class EtherState
{
public:
  EtherState(void) {}

  virtual std::auto_ptr<cMessage> processSignal(EtherModule* mod, std::auto_ptr<cMessage> msg);

protected:
  virtual std::auto_ptr<EtherSignalData> processData(EtherModule* mod, std::auto_ptr<EtherSignalData> data);
  virtual std::auto_ptr<EtherSignalJam> processJam(EtherModule* mod, std::auto_ptr<EtherSignalJam> jam);
  virtual std::auto_ptr<EtherSignalJamEnd> processJamEnd(EtherModule* mod, std::auto_ptr<EtherSignalJamEnd> jamEnd);
  virtual std::auto_ptr<EtherSignalIdle> processIdle(EtherModule* mod, std::auto_ptr<EtherSignalIdle> idle);

  // converting backoff message to different state
//  void convertSelfBackoff(EtherModule* mod, const int fromMsgID, const int toMsgID);

private:
  // debug message
  void printMsg(EtherModule* mod, const EtherSignalType type);
};

template<class Target, class  Source> std::auto_ptr<Target>
auto_downcast(std::auto_ptr<Source> & r)
{
  boost::polymorphic_downcast<Target*> (r.get());
  return std::auto_ptr<Target>(static_cast<Target*>(r.release()));
}

#endif
