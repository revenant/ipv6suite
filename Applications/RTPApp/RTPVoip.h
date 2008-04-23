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
 */

#ifndef RTPVOIP_H
#define RTPVOIP_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

#ifndef RTP_H
#include "RTP.h"
#endif

//boost 1.35 req.
#ifndef BOOST_CIRCULAR_BUFFER_FWD_HPP
#include <boost/circular_buffer_fwd.hpp>
#endif

#ifndef BOOST_TUPLE_HPP
#include <boost/tuple/tuple.hpp>
#endif

/**
 * @class RTPVoip
 *
 * @brief models a simple 2 party voip application
 *
 * ITU-T P.59 conversation model and various bitrate settings.
 * Contains ITU-T G.107 E-model and also a constant jitter buffer
 * playback algorithm.
 * RTCP handling code comes from baseclass RTP
 */

class RTPVoip: public RTP
{
 public:

  ///@name Overidden cSimpleModule functions
  //@{
  int numInitStages() const;

  virtual void initialize(int stageNo);

  virtual void finish();  

  //virtual void receiveChangeNotification(int category, cPolymorphic *details);
  //@}

  //@name constructors, destructors and operators
  //@{
  RTPVoip();

  ~RTPVoip();
  //@}

  ///@name Overidden RTP functions
  //@{
  virtual void leaveSession();
  virtual void establishSession();
  virtual void processGoodBye(RTCPGoodBye* rtcp);
  virtual void processSDES(RTCPSDES* sdes);
  virtual void processRTPData(RTPPacket* rtpData, RTPMemberEntry &rme);
  virtual void handleMisorderedPackets(RTPMemberEntry *s, RTPPacket* rtpData);
  virtual void handleDroppedPackets(RTPMemberEntry *s, u_int16 udelta);
  virtual void attachData(RTPPacket* rtpData);
  //virtual bool sendRTPPacket();
  //@}

  ///represents one or more frames in a packet with
  ///first arg as timestamp and second arg is the seqNo
  typedef boost::tuple<double, unsigned int>  VoipFrame;

  //@name ned params storage
  //@{
  // no need for this in RTP either as we should initiate at initiate or just don't
  //simtime_t startTime;
  double frameLength;
  unsigned int bitrate;
  unsigned int framesPerPacket;  
  //@}

  //@name voip conversation model
  //@{
  void P59TS(RTP* callee);
  void P59ST(RTP* callee);
  void P59MS(RTP* callee);
  void P59DT(RTP* callee);
 
  cCallbackMessage* p59cb;
  cOutVector* talkStatesVector;
  cStdDev* stStat;
  cStdDev* dtStat;
  cStdDev* msStat;
  cStdDev* tsStat;
  simtime_t callerPause;
  simtime_t calleePause;
  cStdDev* callerPauseStat;
  cStdDev* calleePauseStat;
  //@}
 
 protected:
  //@name playout buffer params
  //@{
  virtual void playoutBufferedPacket();
  virtual simtime_t playoutTime(simtime_t timestamp);
  //work out dropped and discarded packets
  //void compareToPreviousVoipFrame(const VoipFrame& thisFrame);
  //void recordLosses(RTPMemberEntry& rme, u_int16 udelta);
  cCallbackMessage* playoutTimer;
  simtime_t networkDelay;
  simtime_t jitterDelay;
  typedef boost::circular_buffer<VoipFrame> JitterBuffer;
  JitterBuffer* cb;
  ///records seq of packets discarded by jitter buffer
  typedef std::list<unsigned int> DiscardBuffer;
  typedef DiscardBuffer::iterator DBI;
  DiscardBuffer db;
  ///records the range of seq where packets are dropped
  typedef std::vector<DiscardBuffer> SeqRangeRecord;
  typedef SeqRangeRecord::iterator SRRI;
  SeqRangeRecord probableDropped;
  ///records list of misordered packets. should be cleaned out periodically?
  JitterBuffer* misordered;
  VoipFrame lastPlayedFrame;
  ///records if packet has been discarded since last playtime or packet reception
  bool discarded;
  //@}
 private:

  //@name ITU T G.107 E-model R factor calc
  //@{
  double ecalculateRFactor();
  void ecalculateMeanTotalDelay(simtime_t timestamp);
  ///running meanDelay calculated at every packet reception
  double emeanDelay;
  DiscardBuffer elossEvents;
  ///count of playedback frames
  unsigned int elastReceived;
  ///value of extended = rme.cycles + rme.maxSeq at last sample
  unsigned int elastExpected;
  unsigned int elossEventsCount;
  unsigned int elostPackets;
  ///call ecalculateRFactor with freq in ned param emodelInterval
  cCallbackMessage* emodelTimer;
  cOutVector* erfactorVector;
  cOutVector* totalDelayVector;
  cStdDev*  totalDelayStat;
  //ned params 
  double Ie;
  double Bpl;
  double lookahead;
  //@}


};


#endif /* RTPVOIP_H */

