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
#include "EtherSignal_m.h"
#include "EtherFrame6.h"
#include "EtherStateIdle.h"
#include "EtherStateSend.h"
#include "EtherStateReceive.h"
#include "EtherStateWaitBackoff.h"
#include "EtherStateWaitBackoffJam.h"
#include "EtherStateWaitJam.h"
#include "EtherStateReceiveWaitBackoff.h"

using std::string;

EtherState::~EtherState(){}

std::auto_ptr<cMessage> EtherState::processSignal(EtherModule* mod, std::auto_ptr<cMessage> msg)
{
  // a packet from upper layer
  if (msg.get()->arrivedOn(mod->outputQueueInGate()))
  {
    mod->receiveData(msg);
    return msg;
  }

  EtherSignal* signal = check_and_cast<EtherSignal*>(msg.get());

  // debug
  printMsg(mod, signal);
  ///////

  if (dynamic_cast<EtherSignalData*>(signal))
     msg = processData(mod, auto_downcast<EtherSignalData>(msg));
  else if (dynamic_cast<EtherSignalIdle*>(signal))
     msg = processIdle(mod, auto_downcast<EtherSignalIdle> (msg));
  else if (dynamic_cast<EtherSignalJam*>(signal))
     msg = processJam(mod, auto_downcast<EtherSignalJam> (msg));
  else if (dynamic_cast<EtherSignalJamEnd*>(signal))
     msg = processJamEnd(mod, auto_downcast<EtherSignalJamEnd> (msg));
  else
     opp_error("unknown EtherSignal (%s)%s", signal->className(), signal->name());

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

void EtherState::printMsg(EtherModule* mod, EtherSignal *ethSignal)
{
  string s = ethSignal->className();

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
