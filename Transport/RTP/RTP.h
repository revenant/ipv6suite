// -*- C++ -*-
// Copyright (C) 2006 Johnny Lai
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
 * @file RTP.h
 * @author 
 * @date 29 Jul 2006
 *
 * @brief Definition of class RTP
 *
 */

#ifndef RTP_H
#define RTP_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

#include "UDPAppBase.h"

/**
 * @class RTP
 *
 * @brief 
 *
 * detailed description
 */

class RTP: public UDPAppBase
{
 public:
#ifdef USE_CPPUNIT
  friend class RTPTest;
#endif //USE_CPPUNIT

  Module_Class_Members(RTP, UDPAppBase, 0);

  ///@name Overidden cSimpleModule functions
  //@{
  int numInitStages() const;

  virtual void initialize(int stageNo);

  virtual void finish();

  virtual void handleMessage(cMessage* msg);

  //@}

  //@name constructors, destructors and operators
  //@{
  /*
  RTP();

  ~RTP();
  */
  //@}

  virtual void processReceivedPacket(cMessage* msg);
  virtual void leaveSession();
  virtual void establishSession();
  virtual simtime_t calculateTxInterval();

 protected:

 private:

  unsigned short port;
  std::vector<IPvXAddress> destAddrs;
  cMessage* rtcpTimeout;

  //last time rtcp tx
  simtime_t tp;
  //next schedule tx of rtcp
  simtime_t tn; //prob. in rtcp timer
  //estimate no. of participants at tn computation
  unsigned short pmembers; 
  //current no. of members
  unsigned short members;
  unsigned short senders;
  //fraction of sessionBandwidth used by all
  float rtcpBw;
  //true when data sent after 2nd last rtcp report tx
  bool weSent;
  //average of all rtcp packets sent & received (i.e. datagram size)
  float meanRtcpSize; // avg_rtcp_size
  //true if no rtcp sent
  bool initial;

};

#endif /* RTP_H */

