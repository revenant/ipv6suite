// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/NetworkInterfaces/Ethernet6/EtherState.cc,v 1.2 2005/02/10 05:59:32 andras Exp $
//
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
    @file EtherState.cc
    @brief Definition file for EtherState

    Defines simple FSM for Ethernet operation based on "Efficient and
    Accurate Ethernet Simulation" by Jia Wang and Srinivasan Keshav

    @author Eric Wu */

#include <sys.h> // Dout
#include "debug.h" // Dout

#include <string>

#include "EtherState.h"

#include "cTTimerMessageCB.h"

#include "EtherModule.h"
#include "EtherSignal.h"
#include "EtherFrame.h"
#include "EtherStateIdle.h"
#include "EtherStateSend.h"
#include "EtherStateReceive.h"
#include "EtherStateWaitBackoff.h"
#include "EtherStateWaitBackoffJam.h"
#include "EtherStateWaitJam.h"
#include "EtherStateReceiveWaitBackoff.h"

using std::string;

std::auto_ptr<cMessage> EtherState::processSignal(EtherModule* mod, std::auto_ptr<cMessage> msg)
{
  // a packet from upper layer
  if (msg.get()->arrivedOn(mod->outputQueueInGate()))
  {
    mod->receiveData(msg);
    return msg;
  }

  EtherSignal* signal = boost::polymorphic_downcast<EtherSignal*>(msg.get());

  // debug
  printMsg(mod, signal->type());
  ///////

  switch(signal->type())

  {
    case EST_Data:
      msg.reset((processData(mod, auto_downcast<EtherSignalData> (msg))).release());
      break;
    case EST_Idle:
      msg.reset((processIdle(mod, auto_downcast<EtherSignalIdle> (msg))).release());
      break;

    case EST_Jam:
      msg.reset((processJam(mod, auto_downcast<EtherSignalJam> (msg))).release());
      break;

    case EST_JamEnd:
      msg.reset((processJamEnd(mod, auto_downcast<EtherSignalJamEnd> (msg))).release());
      break;
    default:
      assert(false);
      break;
  }
  return msg;
}

std::auto_ptr<EtherSignalData> EtherState::processData(EtherModule* mod, std::auto_ptr<EtherSignalData> data)
{
  return data;
}

std::auto_ptr<EtherSignalJam> EtherState::processJam(EtherModule* mod, std::auto_ptr<EtherSignalJam> jam)
{
  return jam;
}

std::auto_ptr<EtherSignalJamEnd> EtherState::processJamEnd(EtherModule* mod, std::auto_ptr<EtherSignalJamEnd> jamEnd)
{
  return jamEnd;
}

std::auto_ptr<EtherSignalIdle> EtherState::processIdle(EtherModule* mod, std::auto_ptr<EtherSignalIdle> idle)
{

  return idle;
}

/*
void EtherState::convertSelfBackoff(EtherModule* mod, const int fromMsgID, const int toMsgID)
{
  cTimerMessage* from = mod->getTmrMessage(fromMsgID);
  assert(from);

  // BACKOFF timer message hasnt arrived yet. We have to reschedule it again
  cTimerMessage* to = mod->getTmrMessage(toMsgID);
  assert(to);
}
*/

void EtherState::printMsg(EtherModule* mod, const EtherSignalType type)
{
  string s;

  switch(type)
  {
    case EST_Data:
      s = "DATA";
      break;
    case EST_Idle:
      s = "IDLE";
      break;

    case EST_Jam:
      s = "JAM";
      break;

    case EST_JamEnd:
      s = "JAMEND";
      break;
    default:
      assert(false);
      break;
  }

  if ( mod->currentState() == EtherStateIdle::instance())
    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->simTime() << " sec, " << mod->fullPath() << ": " << s.c_str() << " in IDLE state");
  else if ( mod->currentState() == EtherStateSend::instance())
    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->simTime() << " sec, " << mod->fullPath() << ": " << s.c_str() << " in SEND state");
  else if ( mod->currentState() == EtherStateReceive::instance())
    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->simTime() << " sec, " << mod->fullPath() << ": " << s.c_str() << " in RECEIVE state");
  else if ( mod->currentState() == EtherStateWaitBackoff::instance())
    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->simTime() << " sec, " << mod->fullPath() << ": " << s.c_str() << " in WAITBACKOFF state");
  else if ( mod->currentState() == EtherStateWaitBackoffJam::instance())
    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->simTime() << " sec, " << mod->fullPath() << ": " << s.c_str() << " in WAITBACKOFFJAM state");
  else if ( mod->currentState() == EtherStateWaitJam::instance())
    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->simTime() << " sec, " << mod->fullPath() << ": " << s.c_str() << " in WAITJAM state");
  else if ( mod->currentState() == EtherStateReceiveWaitBackoff::instance())
    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->simTime() << " sec, " << mod->fullPath() << ": " << s.c_str() << " in RECEIVEWAITBACKOFF state");
  else
    assert(false);
}
