// -*- C++ -*-
// Copyright (C) 2008 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file RTPVoip.h
 * @author Johnny Lai
 * @date 15 Apr 2008
 *
 * @brief Definition of class RTPVoip
 *
 * @test see RTPVoipTest
 *
 * @todo Remove template text
 */

#ifndef RTPVOIP_H
#define RTPVOIP_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

#ifndef RTP_H
#include "rtp.h"
#endif

/**
 * @class RTPVoip
 *
 * @brief 
 *
 * detailed description
 */

class RTPVoip: public RTP
{
 public:

  Module_Class_Members(RTPVoip, cSimpleModule, 0);

  ///@name Overidden cSimpleModule functions
  //@{
  int numInitStages() const;

  virtual void initialize(int stageNo);

  virtual void finish();

  virtual void handleMessage(cMessage* msg);

  virtual void receiveChangeNotification(int category, cPolymorphic *details);
  //@}

  //@name constructors, destructors and operators
  //@{
  RTPVoip();

  ~RTPVoip();
  //@}


  virtual void processReceivedPacket(cMessage* msg);
  virtual void leaveSession();
  virtual void establishSession();
  virtual simtime_t calculateTxInterval();
  virtual bool sendRTPPacket();
  virtual bool isMobileNode();

  void sendBye();

  //ned params storage
  //@{
  // no need for this in RTP either as we should initiate at initiate or just don't
  //simtime_t startTime;
  double frameLength;
  unsigned int bitrate;
  unsigned int framesPerPacket;
  
  //@}

 protected:

 private:

};


#endif /* RTPVOIP_H */

