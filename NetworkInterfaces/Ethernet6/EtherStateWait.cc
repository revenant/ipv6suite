// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/NetworkInterfaces/Ethernet6/EtherStateWait.cc,v 1.1 2005/02/09 06:15:58 andras Exp $
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
	@file EtherStateWait.cc
	@brief Definition file for EtherStateWait

	Defines simple FSM for Ethernet operation based on "Efficient and
	Accurate Ethernet Simulation" by Jia Wang and Srinivasan Keshav

	@author Eric Wu */

#include <sys.h> // Dout
#include "debug.h" // Dout

#include "EtherStateWait.h"
#include "EtherSignal.h"
#include "EtherStateWaitBackoffJam.h"
#include "EtherStateReceiveWaitBackoff.h"
#include "EtherStateWaitBackoff.h"
#include "EtherModule.h"

// Ethernet State Wait

EtherStateWait* EtherStateWait::_instance = 0;

EtherStateWait* EtherStateWait::instance()
{
  if (_instance == 0)
    _instance = new EtherStateWait;
  
  return _instance;
}

EtherStateWait::EtherStateWait(void)
{}

void EtherStateWait::endBackoff(EtherModule* mod)
{
  // debug message
  if ( mod->currentState() == EtherStateWaitBackoff::instance())
    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << ": " << " END OF BACKOFF in WAITBACKOFF state");
  else if ( mod->currentState() == EtherStateReceiveWaitBackoff::instance())
    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << ": " << " END OF BACKOFF in RECEIVEWAITBACKOFF state");
  else if ( mod->currentState() == EtherStateWaitBackoffJam::instance())
    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << ": " << " END OF BACKOFF in WAITBACKOFFJAM state");

  assert(mod->backoffRemainingTime);
  mod->backoffRemainingTime = 0;
}

std::auto_ptr<cMessage> EtherStateWait::processSignal(EtherModule* mod, std::auto_ptr<cMessage> msg)
{
  return EtherState::processSignal(mod, msg);
}

std::auto_ptr<EtherSignalData> EtherStateWait::processData(EtherModule* mod, std::auto_ptr<EtherSignalData> data)
{
  return data;
}

std::auto_ptr<EtherSignalJam> EtherStateWait::processJam(EtherModule* mod, std::auto_ptr<EtherSignalJam> jam)
{
  return jam;
}

std::auto_ptr<EtherSignalJamEnd> EtherStateWait::processJamEnd(EtherModule* mod, std::auto_ptr<EtherSignalJamEnd> jamEnd)
{
  return jamEnd;
}

std::auto_ptr<EtherSignalIdle> EtherStateWait::processIdle(EtherModule* mod, std::auto_ptr<EtherSignalIdle> idle)
{
  return idle;
}
