// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/NetworkInterfaces/Ethernet6/EtherStateReceive.cc,v 1.4 2005/02/16 00:48:30 andras Exp $
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
    @file EtherStateReceive.cc
    @brief Definition file for EtherStateReceive

    Defines simple FSM for Ethernet operation based on "Efficient and
    Accurate Ethernet Simulation" by Jia Wang and Srinivasan Keshav

    @author Eric Wu */

#include <sys.h> // Dout
#include "debug.h" // Dout

#include "EtherStateReceive.h"
#include "ethernet.h"
#include "EtherModule.h"
#include "EtherSignal.h"
#include "EtherStateSend.h"
#include "EtherStateIdle.h"
#include "EtherFrame6.h"
#include "MACAddress6.h"
#include "cTTimerMessageCB.h"
#include "EtherStateWaitJam.h"


using std::string;


// Ethernet State Receive

EtherStateReceive* EtherStateReceive::_instance = 0;

EtherStateReceive* EtherStateReceive::instance(void)
{
  if (_instance == 0)
    _instance = new EtherStateReceive;

  return _instance;
}

EtherStateReceive::EtherStateReceive(void)
{}

std::auto_ptr<cMessage> EtherStateReceive::processSignal(EtherModule* mod, std::auto_ptr<cMessage> msg)
{
  return EtherState::processSignal(mod, msg);
}

std::auto_ptr<EtherSignalData> EtherStateReceive::processData(EtherModule* mod, std::auto_ptr<EtherSignalData> data)
{
  mod->incNumOfRxIdle(data->getSrcModPathName());

  if(mod->inputFrame)
  {
    delete mod->inputFrame;
    mod->inputFrame = 0;
  }

  mod->changeState(EtherStateWaitJam::instance());

  return data;
}

std::auto_ptr<EtherSignalJam> EtherStateReceive::processJam(EtherModule* mod, std::auto_ptr<EtherSignalJam> jam)
{
  if (mod->inputFrame)
  {
    delete mod->inputFrame;
    mod->inputFrame = 0;
  }

  mod->incNumOfRxJam(jam->getSrcModPathName());
  mod->changeState(EtherStateWaitJam::instance());

  return jam;
}

std::auto_ptr<EtherSignalJamEnd> EtherStateReceive::processJamEnd(EtherModule* mod, std::auto_ptr<EtherSignalJamEnd> jamEnd)
{
  assert(false);
  return jamEnd;
}

std::auto_ptr<EtherSignalIdle> EtherStateReceive::processIdle(EtherModule* mod, std::auto_ptr<EtherSignalIdle> idle)
{
  mod->decNumOfRxIdle(idle->getSrcModPathName());

  if ( mod->isMediumBusy() )
    return idle;

  if ( !mod->inputFrame )
  {
    mod->changeState(EtherStateIdle::instance());
    static_cast<EtherStateIdle*>(mod->currentState())->chkOutputBuffer(mod);
    return idle;
  }

  EtherFrame6* recFrame = mod->inputFrame->data();

  // send to upper layer
  mod->sendData(recFrame);

  delete mod->inputFrame;
  mod->inputFrame = 0;

  mod->changeState(EtherStateIdle::instance());
  static_cast<EtherStateIdle*>(mod->currentState())->chkOutputBuffer(mod);

  return idle;
}
