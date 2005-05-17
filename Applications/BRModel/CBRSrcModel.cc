//
// Copyright 2004 Monash University, Australia
//
// This library is free software, you can redistribute it and/or modify
// it under  the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation;
// either version 2 of the License, or any later version.
// The library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//

#include "CBRSrcModel.h"

Define_Module(CBRSrcModel);

void CBRSrcModel::initialize()
{
  //Get NED parameters
  destAddr.set(par("destAddr").stringValue());  
  tStart = par("tStart");
  fragmentLen = par("fragmentLen");
  bitRate = &par("bitRate");

  sequenceNo = 1;
  double packetsPerSec = (double)(*bitRate)/(8*fragmentLen);
  if(packetsPerSec < 1)
  {
    packetsPerSec = 1;
    packetSize = (unsigned long)bitRate/8;
  }
  else 
    packetSize = fragmentLen;
  
  sendPeriod = 1/packetsPerSec;
  
  //Schedule first active period
  timer = new cMessage("CBRtimer");
  scheduleAt(tStart, timer);
}

void CBRSrcModel::handleMessage(cMessage* msg)
{
  if(msg->isSelfMessage())
  {
    sendPacket();
  }
  else
  {
  }
}

void CBRSrcModel::sendPacket()
{
  //send the frame size in bytes
  fragmentAndSend(packetSize);

  //schedule next frame
  scheduleAt(simTime()+sendPeriod, timer);
}
