// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/NetworkInterfaces/Ethernet6/EtherStateSend.h,v 1.2 2005/02/10 05:59:32 andras Exp $
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
    @file EtherStateSend.h
    @brief Header file for EtherStateSend

    Defines simple FSM for Ethernet operation based on "Efficient and
    Accurate Ethernet Simulation" by Jia Wang and Srinivasan Keshav

    @author Eric Wu */

#ifndef __ETHER_STATE_SEND_H__
#define __ETHER_STATE_SEND_H__

#include <omnetpp.h>

#include "EtherState.h"
#include "cTimerMessage.h"

class EtherModule;

// Ethernet State Send

class EtherStateSend : public EtherState
{
  friend class EtherState;
  friend class EtherStateReceive;

public:
  static EtherStateSend* instance(void);

  virtual std::auto_ptr<cMessage> processSignal(EtherModule* mod, std::auto_ptr<cMessage> msg);

  void endSendingData(EtherModule* mod);
  void endSendingJam(EtherModule* mod, cTimerMessage* msg);

  void sendJam(EtherModule* mod);

protected:
  EtherStateSend(void);

  virtual std::auto_ptr<EtherSignalData> processData(EtherModule* mod, std::auto_ptr<EtherSignalData> data);
  virtual std::auto_ptr<EtherSignalJam> processJam(EtherModule* mod, std::auto_ptr<EtherSignalJam> jam);
  virtual std::auto_ptr<EtherSignalJamEnd> processJamEnd(EtherModule* mod, std::auto_ptr<EtherSignalJamEnd> jamEnd);
  virtual std::auto_ptr<EtherSignalIdle> processIdle(EtherModule* mod, std::auto_ptr<EtherSignalIdle> idle);

protected:
  static EtherStateSend* _instance;

  void removeFrameFromBuffer(EtherModule* mod);

  // implementation of exponential backoff; NOTE: not too sure if the
  // random number generator is of psudo type. Therefore it is best to
  // assign another simtime_t variable and copy it across FRIST.
  simtime_t getBackoffInterval(EtherModule* mod);

  void sendBackoff(EtherModule* mod);
};

#endif
