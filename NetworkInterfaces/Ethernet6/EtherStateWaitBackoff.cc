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
    @file EtherStateWaitBackoff.cc
    @brief Definition file for EtherStateWaitBackoff

    Defines simple FSM for Ethernet operation based on "Efficient and
    Accurate Ethernet Simulation" by Jia Wang and Srinivasan Keshav

    @author Eric Wu */

#include <sys.h> // Dout
#include "debug.h" // Dout

#include "EtherStateWaitBackoff.h"
#include "EtherSignal.h"
#include "EtherModule.h"
#include "EtherStateIdle.h"
#include "EtherStateReceiveWaitBackoff.h"
#include "EtherStateWaitBackoffJam.h"
#include "cTimerMessageCB.h"

// Ethernet State WaitBackoff

EtherStateWaitBackoff* EtherStateWaitBackoff::_instance = 0;

EtherStateWaitBackoff* EtherStateWaitBackoff::instance(void)
{
  if (_instance == 0)
    _instance = new EtherStateWaitBackoff;

  return _instance;
}

EtherStateWaitBackoff::EtherStateWaitBackoff(void)
{}

void EtherStateWaitBackoff::endBackoff(EtherModule* mod)
{
  EtherStateWait::endBackoff(mod);

  // backoff is finished and now resend the frame by restarting from
  // idle state
  mod->changeState(EtherStateIdle::instance());
  static_cast<EtherStateIdle*>(mod->currentState())->chkOutputBuffer(mod);
}

std::auto_ptr<cMessage> EtherStateWaitBackoff::processSignal(EtherModule* mod, std::auto_ptr<cMessage> msg)
{
  return EtherState::processSignal(mod, msg);
}

std::auto_ptr<EtherSignalData> EtherStateWaitBackoff::processData(EtherModule* mod, std::auto_ptr<EtherSignalData> data)
{
  mod->incNumOfRxIdle(data->getSrcModPathName());

  assert(!mod->inputFrame);
  mod->inputFrame = data.get()->dup();

  cTimerMessage* tmrBackoff = mod->getTmrMessage(SELF_BACKOFF);
  assert(tmrBackoff && tmrBackoff->isScheduled());

  mod->backoffRemainingTime = tmrBackoff->remainingTime();
  Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << " @receives a DATA message and ceases to back off, BackoffRemainingTime =" << mod->backoffRemainingTime);
  tmrBackoff->cancel();

  mod->changeState(EtherStateReceiveWaitBackoff::instance());

  return data;
}

std::auto_ptr<EtherSignalJam> EtherStateWaitBackoff::processJam(EtherModule* mod, std::auto_ptr<EtherSignalJam> jam)
{
  mod->incNumOfRxJam(jam->getSrcModPathName());

  cTimerMessage* tmrBackoff = mod->getTmrMessage(SELF_BACKOFF);
  assert(tmrBackoff && tmrBackoff->isScheduled());

  mod->backoffRemainingTime = tmrBackoff->remainingTime();
  Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << " @ receives a JAM message and ceases to back off, BackoffRemainingTime = " << mod->backoffRemainingTime);
  tmrBackoff->cancel();

  mod->changeState(EtherStateWaitBackoffJam::instance());

  return jam;
}

std::auto_ptr<EtherSignalJamEnd> EtherStateWaitBackoff::processJamEnd(EtherModule* mod, std::auto_ptr<EtherSignalJamEnd> jamEnd)
{
  assert(false);
  return jamEnd;
}

std::auto_ptr<EtherSignalIdle> EtherStateWaitBackoff::processIdle(EtherModule* mod, std::auto_ptr<EtherSignalIdle> idle)
{
  assert(false);

  return idle;
}

void EtherStateWaitBackoff::backoff(EtherModule* mod)
{
  Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << ": BackoffRemainingTime = " << mod->backoffRemainingTime);
  assert(mod->backoffRemainingTime);

  cTimerMessage* tmrMessage = mod->getTmrMessage(SELF_BACKOFF);
  if (!tmrMessage)
  {
    Loki::cTimerMessageCB<void,
      TYPELIST_1(EtherModule*)>* selfBackoff;

    selfBackoff  = new Loki::cTimerMessageCB<void,
      TYPELIST_1(EtherModule*)>
      (SELF_BACKOFF, mod, static_cast<EtherStateWaitBackoff*>
       (mod->currentState()), &EtherStateWaitBackoff::endBackoff,
         "endBackoff");

    Loki::Field<0> (selfBackoff->args) = mod;

    mod->addTmrMessage(selfBackoff);
    tmrMessage = selfBackoff;
  }
  assert(!tmrMessage->isScheduled());
  tmrMessage->reschedule(mod->simTime() + mod->backoffRemainingTime);
}
