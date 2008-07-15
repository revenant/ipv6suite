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
    @file EtherStateWaitBackoffJam.cc
    @brief Definition file for EtherStateWaitBackoffJam

    Defines simple FSM for Ethernet operation based on "Efficient and
    Accurate Ethernet Simulation" by Jia Wang and Srinivasan Keshav

    @author Eric Wu */

#include <sys.h> // Dout
#include "debug.h" // Dout

#include "EtherStateWaitBackoffJam.h"
#include "EtherSignal_m.h"
#include "EtherStateWaitBackoff.h"
#include "EtherStateSend.h"
#include "EtherModule.h"
#include "EtherStateWaitJam.h"


// Ethernet State WaitBackoffJam

EtherStateWaitBackoffJam* EtherStateWaitBackoffJam::_instance = 0;

EtherStateWaitBackoffJam* EtherStateWaitBackoffJam::instance()
{
  if (_instance == 0)
    _instance = new EtherStateWaitBackoffJam;

  return _instance;
}

EtherStateWaitBackoffJam::EtherStateWaitBackoffJam(void)
{}

void EtherStateWaitBackoffJam::endBackoff(EtherModule* mod)
{
  EtherStateWait::endBackoff(mod);

  mod->changeState(EtherStateWaitJam::instance());
}

std::auto_ptr<cMessage> EtherStateWaitBackoffJam::processSignal(EtherModule* mod, std::auto_ptr<cMessage> msg)
{
  return EtherState::processSignal(mod, msg);
}

std::auto_ptr<EtherSignalData> EtherStateWaitBackoffJam::processData(EtherModule* mod, std::auto_ptr<EtherSignalData> data)
{
  mod->incNumOfRxIdle(data->getSrcModPathName());

  return data;
}

std::auto_ptr<EtherSignalJam> EtherStateWaitBackoffJam::processJam(EtherModule* mod, std::auto_ptr<EtherSignalJam> jam)
{
  mod->incNumOfRxJam(jam->getSrcModPathName());
  return jam;
}

std::auto_ptr<EtherSignalJamEnd> EtherStateWaitBackoffJam::processJamEnd(EtherModule* mod, std::auto_ptr<EtherSignalJamEnd> jamEnd)
{
  mod->decNumOfRxJam(jamEnd->getSrcModPathName());
//  assert(!mod->interframeGap);

  if (!mod->isMediumBusy())
  {
    // check if the module is still sending the JAM message
//    cTimerMessage* tmrMessage = mod->getTmrMessage(TRANSMIT_JAM);
//    assert(tmrMessage);

    // if not, the module can start/resume backing off
//    if (!tmrMessage->isScheduled())
//    {
      mod->changeState(EtherStateWaitBackoff::instance());
      static_cast<EtherStateWaitBackoff*>(mod->currentState())->backoff(mod);
//    }
  }

  return jamEnd;
}

std::auto_ptr<EtherSignalIdle> EtherStateWaitBackoffJam::processIdle(EtherModule* mod, std::auto_ptr<EtherSignalIdle> idle)
{
  mod->decNumOfRxIdle(idle->getSrcModPathName());
  assert(!mod->interframeGap);

  if (!mod->isMediumBusy())
  {
    // check if the module is still sending the JAM message
//    cTimerMessage* tmrMessage = mod->getTmrMessage(TRANSMIT_JAM);
//    assert(tmrMessage);

    // if not, the module can start/resume backing off
//    if (!tmrMessage->isScheduled())
//    {
    mod->changeState(EtherStateWaitBackoff::instance());
    static_cast<EtherStateWaitBackoff*>(mod->currentState())->backoff(mod);
//    }
  }

  return idle;
}
