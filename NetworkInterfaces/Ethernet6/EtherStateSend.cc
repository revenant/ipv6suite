// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/NetworkInterfaces/Ethernet6/EtherStateSend.cc,v 1.1 2005/02/09 06:15:58 andras Exp $
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
	@file EtherStateSend.cc
	@brief Definition file for EtherStateSend

	Defines simple FSM for Ethernet operation based on "Efficient and
	Accurate Ethernet Simulation" by Jia Wang and Srinivasan Keshav

	@author Eric Wu */

#include "sys.h" // Dout
#include "debug.h" // Dout

#include <cmath>
#include <memory>

#include "EtherStateSend.h"
#include "EtherSignal.h"
#include "EtherModule.h"
#include "EtherStateIdle.h"
#include "cTimerMessageCB.h"
#include "EtherStateWaitBackoffJam.h"
#include "EtherStateWaitBackoff.h"
#include "EtherStateWaitJam.h"
#include "opp_akaroa.h"

// Ethernet State Send

EtherStateSend* EtherStateSend::_instance = 0;

EtherStateSend* EtherStateSend::instance(void)
{
  if (_instance == 0)
    _instance = new EtherStateSend;
  
  return _instance;
}

EtherStateSend::EtherStateSend(void)
{}

std::auto_ptr<cMessage> EtherStateSend::processSignal(EtherModule* mod, std::auto_ptr<cMessage> msg)
{
  return EtherState::processSignal(mod, msg);
}

void EtherStateSend::endSendingData(EtherModule* mod)
{
  assert(mod->outputBuffer.size());

  EtherSignalIdle* idle = new EtherSignalIdle;
  idle->setSrcModPathName(mod->fullPath());
  mod->sendFrame(idle, mod->outPHYGate());

  if(!mod->isMediumBusy())
  {
    if ( mod->frameCollided )
    {
      mod->frameCollided = false;

      // abort the transmission of the frame
      if ( mod->getRetry() > MAX_RETRY)
      {
        Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << ": " << " MAXIMUM RETRY! abort sending! ");
      
        removeFrameFromBuffer(mod);
        
        mod->resetRetry();
        mod->cancelAllTmrMessages();

        if (mod->isMediumBusy())
          mod->changeState(EtherStateWaitJam::instance());
        else
        {
          mod->changeState(EtherStateIdle::instance());
          static_cast<EtherStateIdle*>(mod->currentState())->chkOutputBuffer(mod);
        }
        return;
      }

      // compute backoff period
      mod->backoffRemainingTime = ( mod->backoffRemainingTime ?
                                    mod->backoffRemainingTime : getBackoffInterval(mod));

      mod->changeState(EtherStateWaitBackoff::instance());
      static_cast<EtherStateWaitBackoff*>(mod->currentState())->backoff(mod);
      return;
    }

    assert(!mod->interframeGap);

    removeFrameFromBuffer(mod);

    mod->resetRetry();
    
    mod->changeState(EtherStateIdle::instance());

    double d = (double)INTER_FRAME_GAP*8;
    simtime_t interFrameGapTime = d / BANDWIDTH;
    
    cTimerMessage* tmrMessage = mod->getTmrMessage(SELF_INTERFRAMEGAP);
    if (!tmrMessage)
    {
      Loki::cTimerMessageCB<void, 
        TYPELIST_1(EtherModule*)>* selfInterframeGap;    

      selfInterframeGap  = new Loki::cTimerMessageCB<void, 
        TYPELIST_1(EtherModule*)>
        (SELF_INTERFRAMEGAP, mod, static_cast<EtherStateIdle*>
         (EtherStateIdle::instance()), &EtherStateIdle::chkOutputBuffer, 
         "selfInterframeGap");

      Loki::Field<0> (selfInterframeGap->args) = mod;
          
      mod->addTmrMessage(selfInterframeGap);
      tmrMessage = selfInterframeGap;
    }
    tmrMessage->reschedule(mod->simTime() + interFrameGapTime);
    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << ": " << "scheduling INTERFRAME message");
  }
}

void EtherStateSend::endSendingJam(EtherModule* mod, cTimerMessage* msg)
{
  Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << ": " << " finishes sending JAM message ");

  assert(msg);

  EtherSignalJamEnd* jamEnd = new EtherSignalJamEnd;
  jamEnd->setSrcModPathName(mod->fullPath());  
  mod->sendFrame(jamEnd, mod->outPHYGate());

  if(mod->isMediumBusy())
    return;

  // abort the transmission of the frame
  if ( mod->getRetry() > MAX_RETRY)
  {
    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << ": " << " MAXIMUM RETRY! abort sending! ");

    removeFrameFromBuffer(mod);

    mod->resetRetry();
    mod->cancelAllTmrMessages();

    if (mod->isMediumBusy())
      mod->changeState(EtherStateWaitJam::instance());
    else
    {
      mod->changeState(EtherStateIdle::instance());
      static_cast<EtherStateIdle*>(mod->currentState())->chkOutputBuffer(mod);
    }
    return;
  }

//  if (mod->isMediumBusy())
//    mod->changeState(EtherStateWaitBackoffJam::instance());
//  else
  if (!mod->isMediumBusy() && !mod->getTmrMessage(TRANSMIT_SENDDATA)->isScheduled())
  {
    // compute backoff period
    mod->backoffRemainingTime = ( mod->backoffRemainingTime ?
                                  mod->backoffRemainingTime : getBackoffInterval(mod));

    mod->changeState(EtherStateWaitBackoff::instance());
    static_cast<EtherStateWaitBackoff*>(mod->currentState())->backoff(mod);
  }
}

void EtherStateSend::sendJam(EtherModule* mod)
{
  double d = (double)JAM_LENGTH*8;
  simtime_t transmTime = d / BANDWIDTH;

  EtherSignalJam* jam = new EtherSignalJam;
  jam->setSrcModPathName(mod->fullPath());  
  mod->sendFrame(jam, mod->outPHYGate());

  cTimerMessage* tmrMessage = mod->getTmrMessage(TRANSMIT_JAM);
  if (!tmrMessage)
  {
    Loki::cTimerMessageCB<void, 
      TYPELIST_2(EtherModule*, cTimerMessage*)>* selfJamMsg;    

    selfJamMsg  = new Loki::cTimerMessageCB<void, 
      TYPELIST_2(EtherModule*, cTimerMessage*)>
      (TRANSMIT_JAM, mod, static_cast<EtherStateSend*>
       (EtherStateSend::instance()), &EtherStateSend::endSendingJam, 
       "endSendingJam");
        
    Loki::Field<0> (selfJamMsg->args) = mod;
    Loki::Field<1> (selfJamMsg->args) = selfJamMsg;
          
    mod->addTmrMessage(selfJamMsg);
    tmrMessage = selfJamMsg;
  }

  // we keep rescheduling the end-of-jam message when the module keep
  // receiving the start-of-frame message in the SEND state
  if(tmrMessage->isScheduled())
    tmrMessage->cancel();
  
  tmrMessage->reschedule(mod->simTime() + transmTime);
}

std::auto_ptr<EtherSignalData> EtherStateSend::processData(EtherModule* mod, std::auto_ptr<EtherSignalData> data)
{
  mod->frameCollided = true;

  if (!mod->isMediumBusy())
  {
    mod->incrementRetry();
    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << "RETRY " << mod->getRetry());
  }

  mod->incNumOfRxIdle(data->getSrcModPathName());

  sendJam(mod);
  Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << ": " << " Collision detected, sending JAM signal.");

  return data;
}

std::auto_ptr<EtherSignalJam> EtherStateSend::processJam(EtherModule* mod, std::auto_ptr<EtherSignalJam> jam)
{
  mod->frameCollided = true;

  if (!mod->isMediumBusy())
  {
    mod->incrementRetry();
    Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << "RETRY " << mod->getRetry());
  }

  mod->incNumOfRxJam(jam->getSrcModPathName());

  // there are times when the data from the network gets received
  // first and consequently cancalled the self sending tmr message.
  // cancel the sending of the frame
//  cTimerMessage* tmrSend = mod->getTmrMessage(TRANSMIT_SENDDATA);
//  assert(tmrSend);
//  if (tmrSend->isScheduled())
//  {
//    mod->incrementRetry();
    
    // compute backoff period
//    mod->backoffRemainingTime = getBackoffInterval(mod);

//    tmrSend->cancel();
//  }

  // When the STA receives a JAM message, it is most likely unware if
  // the medium is busy or not. That is why it can receive a JAM
  // message while it is sending the data. Therefore, it should be the
  // first attemp of the trasnmission.
//  assert( mod->getRetry() <= MAX_RETRY );
  
//  cTimerMessage* tmrMessage = mod->getTmrMessage(TRANSMIT_JAM);
//  if (tmrMessage && !tmrMessage->isScheduled())
//    mod->changeState(EtherStateWaitBackoffJam::instance());

//  Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << ": " << " Collision detected, receiving JAM signal. At this time backoffRemainingTime = " << mod->backoffRemainingTime);

  return jam;
}

std::auto_ptr<EtherSignalJamEnd> EtherStateSend::processJamEnd(EtherModule* mod, std::auto_ptr<EtherSignalJamEnd> jamEnd)
{
  mod->decNumOfRxJam(jamEnd->getSrcModPathName());
  assert(!mod->interframeGap);

  if (!mod->isMediumBusy() && !mod->getTmrMessage(TRANSMIT_SENDDATA)->isScheduled() && !mod->getTmrMessage(TRANSMIT_JAM)->isScheduled())
  {
    mod->frameCollided = false;

    // compute backoff period
    mod->backoffRemainingTime = ( mod->backoffRemainingTime ?
                                  mod->backoffRemainingTime : getBackoffInterval(mod));

    mod->changeState(EtherStateWaitBackoff::instance());
    static_cast<EtherStateWaitBackoff*>(mod->currentState())->backoff(mod);
  }

  return jamEnd;
}

std::auto_ptr<EtherSignalIdle> EtherStateSend::processIdle(EtherModule* mod, std::auto_ptr<EtherSignalIdle> idle)
{
  mod->decNumOfRxIdle(idle->getSrcModPathName());
  
  if (!mod->isMediumBusy() && !mod->getTmrMessage(TRANSMIT_SENDDATA)->isScheduled() && !mod->getTmrMessage(TRANSMIT_JAM)->isScheduled())
  {
    mod->frameCollided = false;

    // compute backoff period
    mod->backoffRemainingTime = ( mod->backoffRemainingTime ?
                                  mod->backoffRemainingTime : getBackoffInterval(mod));

    mod->changeState(EtherStateWaitBackoff::instance());
    static_cast<EtherStateWaitBackoff*>(mod->currentState())->backoff(mod);
  }

  return idle;
}

simtime_t EtherStateSend::getBackoffInterval(EtherModule* mod)
{
  //Spec allows max retry of 15 only
//  assert( mod->getRetry() <= MAX_RETRY );

  size_t k = (mod->getRetry()>10?10:mod->getRetry());
  size_t u = static_cast<size_t>(pow(2, k));
  double r = OPP_UNIFORM(0,u);
  simtime_t backoff =  r * SLOT_TIME;

  return backoff;
}

void EtherStateSend::removeFrameFromBuffer(EtherModule* mod)
{
  EtherSignalData* data = *(mod->outputBuffer.begin());
  delete data;
  mod->outputBuffer.pop_front();
  
  Dout(dc::ethernet|flush_cf, "MAC LAYER: " << mod->fullPath() << ": Output packet removed from the buffer, the size of output buffer is: " << mod->outputBuffer.size());
}
