// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/NetworkInterfaces/Ethernet6/EtherStateWaitJam.cc,v 1.2 2005/02/10 05:59:32 andras Exp $
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
    @file EtherStateWaitJam.cc
    @brief Definition file for EtherStateWaitJam

    Defines simple FSM for Ethernet operation based on "Efficient and
    Accurate Ethernet Simulation" by Jia Wang and Srinivasan Keshav

    @author Eric Wu */

#include "sys.h"
#include "debug.h"

#include "EtherModule.h"
#include "EtherStateWaitJam.h"
#include "EtherSignal.h"
#include "EtherStateIdle.h"

// Ethernet State WaitJam

EtherStateWaitJam* EtherStateWaitJam::_instance = 0;

EtherStateWaitJam* EtherStateWaitJam::instance()
{
  if (_instance == 0)
    _instance = new EtherStateWaitJam;

  return _instance;
}

EtherStateWaitJam::EtherStateWaitJam(void)
{}

std::auto_ptr<cMessage> EtherStateWaitJam::processSignal(EtherModule* mod, std::auto_ptr<cMessage> msg)
{
  return EtherState::processSignal(mod, msg);
}

std::auto_ptr<EtherSignalData> EtherStateWaitJam::processData(EtherModule* mod, std::auto_ptr<EtherSignalData> data)
{
  mod->incNumOfRxIdle(data->getSrcModPathName());
  return data;
}

std::auto_ptr<EtherSignalJam> EtherStateWaitJam::processJam(EtherModule* mod, std::auto_ptr<EtherSignalJam> jam)
{
  mod->incNumOfRxJam(jam->getSrcModPathName());
  return jam;
}

std::auto_ptr<EtherSignalJamEnd> EtherStateWaitJam::processJamEnd(EtherModule* mod, std::auto_ptr<EtherSignalJamEnd> jamEnd)
{
  mod->decNumOfRxJam(jamEnd->getSrcModPathName());

  if ( !mod->isMediumBusy() )
  {
    mod->changeState(EtherStateIdle::instance());

    cTimerMessage* tmrMessage = mod->getTmrMessage(SELF_INTERFRAMEGAP);
    if (tmrMessage && mod->interframeGap != 0)
      tmrMessage->reschedule(mod->simTime() + mod->interframeGap);
  }

  return jamEnd;
}

std::auto_ptr<EtherSignalIdle> EtherStateWaitJam::processIdle(EtherModule* mod, std::auto_ptr<EtherSignalIdle> idle)
{
  mod->decNumOfRxIdle(idle->getSrcModPathName());

  if ( !mod->isMediumBusy() )
  {
    mod->changeState(EtherStateIdle::instance());

    cTimerMessage* tmrMessage = mod->getTmrMessage(SELF_INTERFRAMEGAP);
    if (tmrMessage && mod->interframeGap != 0)
      tmrMessage->reschedule(mod->simTime() + mod->interframeGap);
  }

  return idle;
}
