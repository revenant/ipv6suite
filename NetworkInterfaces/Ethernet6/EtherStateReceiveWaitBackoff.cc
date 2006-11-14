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
    @file EtherStateReceiveWaitBackoff.cc
    @brief Definition file for EtherStateReceiveWaitBackoff

    Defines simple FSM for Ethernet operation based on "Efficient and
    Accurate Ethernet Simulation" by Jia Wang and Srinivasan Keshav

    @author Eric Wu */

#include <sys.h> // Dout
#include "debug.h" // Dout

#include "EtherStateReceiveWaitBackoff.h"
#include "ethernet6.h"
#include "EtherSignal_m.h"
#include "EtherModule.h"
#include "EtherStateSend.h"
#include "EtherStateWaitBackoff.h"
#include "EtherStateWaitBackoffJam.h"
#include "EtherStateReceive.h"
#include "EtherFrame6.h"
#include "MACAddress6.h"

using std::string;

// Ethernet State ReceiveWaitBackoff

EtherStateReceiveWaitBackoff* EtherStateReceiveWaitBackoff::_instance = 0;

EtherStateReceiveWaitBackoff* EtherStateReceiveWaitBackoff::instance()
{
  if (_instance == 0)
    _instance = new EtherStateReceiveWaitBackoff;

  return _instance;
}

EtherStateReceiveWaitBackoff::EtherStateReceiveWaitBackoff(void)
{}

void EtherStateReceiveWaitBackoff::endBackoff(EtherModule* mod)
{
  EtherStateWait::endBackoff(mod);

  mod->changeState(EtherStateReceive::instance());
}

std::auto_ptr<cMessage> EtherStateReceiveWaitBackoff::processSignal(EtherModule* mod, std::auto_ptr<cMessage> msg)
{
  return EtherState::processSignal(mod, msg);
}

std::auto_ptr<EtherSignalData> EtherStateReceiveWaitBackoff::processData(EtherModule* mod, std::auto_ptr<EtherSignalData> data)
{
  mod->incNumOfRxIdle(data->getSrcModPathName());

  assert(mod->inputFrame);
  delete mod->inputFrame;
  mod->inputFrame = 0;

  mod->changeState(EtherStateWaitBackoffJam::instance());

  return data;
}

std::auto_ptr<EtherSignalJam> EtherStateReceiveWaitBackoff::processJam(EtherModule* mod, std::auto_ptr<EtherSignalJam> jam)
{
  mod->incNumOfRxJam(jam->getSrcModPathName());

  assert(mod->inputFrame);
  delete mod->inputFrame;
  mod->inputFrame = 0;

  mod->incNumOfRxJam(jam->getSrcModPathName());
  mod->changeState(EtherStateWaitBackoffJam::instance());

  return jam;
}

std::auto_ptr<EtherSignalJamEnd> EtherStateReceiveWaitBackoff::processJamEnd(EtherModule* mod, std::auto_ptr<EtherSignalJamEnd> jamEnd)
{
  assert(false);
  return jamEnd;
}

std::auto_ptr<EtherSignalIdle> EtherStateReceiveWaitBackoff::processIdle(EtherModule* mod, std::auto_ptr<EtherSignalIdle> idle)
{
  assert(mod->backoffRemainingTime);
  Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << " @ EtherStateReceiveWaitBackoff, BackoffRemainingTime = " << mod->backoffRemainingTime);

  assert(mod->inputFrame);
  EtherFrame6* recFrame = check_and_cast<EtherFrame6*>(mod->inputFrame->decapsulate());

  //collect statistics
  mod->RxDataBWStat += (double)recFrame->encapsulatedMsg()->length()/1000000;
  mod->noOfRxStat++;
  if(mod->statsVec)
    mod->InstRxFrameSizeVec->record(recFrame->encapsulatedMsg()->length()/8);

  mod->decNumOfRxIdle(idle->getSrcModPathName());

  // send to upper layer
  mod->sendData(recFrame);

  delete mod->inputFrame;
  mod->inputFrame = 0;

  mod->changeState(EtherStateWaitBackoff::instance());
  static_cast<EtherStateWaitBackoff*>(mod->currentState())->backoff(mod);

  return idle;
}
