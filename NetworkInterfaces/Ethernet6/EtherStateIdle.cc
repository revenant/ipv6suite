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
    @file EtherStateIdle.cc
    @brief Definition file for EtherStateIdle

    Defines simple FSM for Ethernet operation based on "Efficient and
    Accurate Ethernet Simulation" by Jia Wang and Srinivasan Keshav

    @author Eric Wu */

#include <sys.h> // Dout
#include "debug.h" // Dout

#include <boost/bind.hpp>
#include "EtherStateIdle.h"
#include "cCallbackMessage.h"
#include "EtherFrame6.h"
#include "ethernet6.h"
#include "EtherModule.h"
#include "EtherSignal_m.h"
#include "EtherStateSend.h"
#include "EtherStateReceive.h"
#include "EtherStateWaitJam.h"

// Ethernet State Idle

EtherStateIdle* EtherStateIdle::_instance = 0;

EtherStateIdle* EtherStateIdle::instance()
{
  if (_instance == 0)
    _instance = new EtherStateIdle;

  return _instance;
}

EtherStateIdle::EtherStateIdle(void)
{}

void EtherStateIdle::chkOutputBuffer(EtherModule* mod)
{
  mod->interframeGap = 0;

  // Network Interface Idling...
  // ready to accept data from upper layer
  mod->idleNetworkInterface();

  if(mod->isMediumBusy())
    return;

  if (mod->outputBuffer.size())
  {
    EtherSignalData* data = *(mod->outputBuffer.begin());

    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << ": start sending DATA... ");

    mod->sendFrame((cMessage *)data->dup(), mod->outPHYGate());

    double d = (double)data->encapsulatedMsg()->length()*8;
    d = (d > MIN_FRAMESIZE ? d : MIN_FRAMESIZE );

    simtime_t transmTime = d / BANDWIDTH;

    cTimerMessage* tmrMessage = mod->getTmrMessage(TRANSMIT_SENDDATA);

    if (!tmrMessage)
    {
      cCallbackMessage* sendSig = new cCallbackMessage(
        "endSendingData", TRANSMIT_SENDDATA);
      *sendSig = boost::bind(
        &EtherStateSend::endSendingData,
        static_cast<EtherStateSend*> (mod->currentState()), mod);

      mod->addTmrMessage(sendSig);
      tmrMessage = sendSig;
    }
    assert(!tmrMessage->isScheduled());
    tmrMessage->reschedule(mod->simTime() +  transmTime);

    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << ": the transmit timer "<< (tmrMessage->isScheduled()?"is scheduled":"is not scheduled"));

    mod->changeState(EtherStateSend::instance());
  }
}

std::auto_ptr<cMessage> EtherStateIdle::processSignal(EtherModule* mod, std::auto_ptr<cMessage> msg)
{
  return EtherState::processSignal(mod, msg);
}

std::auto_ptr<EtherSignalData> EtherStateIdle::processData(EtherModule* mod, std::auto_ptr<EtherSignalData> data)
{
    mod->incNumOfRxIdle(data->getSrcModPathName());

  // We can cancel the InterfameGap if it is previously
  // scheduled. This is because the minimum frame size is 512 bits and
  // is a much higer value than InterframeGap, which is 96 bits.
  cTimerMessage* tmrMessage = mod->getTmrMessage(SELF_INTERFRAMEGAP);
  if (tmrMessage && tmrMessage->isScheduled())
    tmrMessage->cancel();

  // make sure the inputFrame is null
  assert(!mod->inputFrame);

    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << ": receiving DATA... ");

  // has to duplicate the data because the auto_ptr will delete the
  // instance afterwards
  mod->inputFrame = (EtherSignalData*)data.get()->dup();

  // entering receive state and waiting to finish receiving the frame
  mod->changeState(EtherStateReceive::instance());

  return data;
}

std::auto_ptr<EtherSignalJam> EtherStateIdle::processJam(EtherModule* mod, std::auto_ptr<EtherSignalJam> jam)
{
  cTimerMessage* tmrMessage = mod->getTmrMessage(SELF_INTERFRAMEGAP);
  if (tmrMessage && tmrMessage->isScheduled())
  {
    mod->interframeGap = tmrMessage->remainingTime();
    tmrMessage->cancel();
  }

  assert(!mod->inputFrame);

  mod->changeState(EtherStateWaitJam::instance());
  mod->incNumOfRxJam(jam->getSrcModPathName());

  return jam;
}

std::auto_ptr<EtherSignalJamEnd> EtherStateIdle::processJamEnd(EtherModule* mod, std::auto_ptr<EtherSignalJamEnd> jamEnd)
{
  assert(false);
  return jamEnd;
}

std::auto_ptr<EtherSignalIdle> EtherStateIdle::processIdle(EtherModule* mod, std::auto_ptr<EtherSignalIdle> idle)
{
  assert(false);
  return idle;
}
