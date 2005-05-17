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

#include "ABRSrcModel.h"

Define_Module(ABRSrcModel);

void ABRSrcModel::initialize()
{
  //Get NED parameters
  destAddr.set(par("destAddr").stringValue());  
  tStart = par("tStart");
  tIdle = &par("tIdle");
  tPause = &par("tPause");
  activePackets = &par("activePackets");
  packetLen = &par("packetLen");
  fragmentLen = par("fragmentLen");
  
  //Initialise variables
  state = IDLE;
  packetsLeft = 0;
  packetSize = 0;
  sequenceNo = 1;
  
  //Schedule first active period
  timer = new cMessage("ABRtimer");
  scheduleAt(tStart, timer);
}

void ABRSrcModel::handleMessage(cMessage* msg)
{
  if(msg->isSelfMessage())
  {
    //End of idle state
    if(state == IDLE)
    {
      //determine number of packets to send during ACTIVE state
      packetsLeft = (*activePackets);
      state = ACTIVE;
    }
    //start sending packet
    sendPacket();
  }
  else
  {
  }
}

void ABRSrcModel::sendPacket()
{
  fragmentAndSend(*packetLen);
  packetsLeft--;
  //pause if more packets left to send
  if(packetsLeft > 0)
  {
    scheduleAt(simTime()+(double)(*tPause), timer);
  }
  //idle if packet limit in active state is reached
  else
  {
    scheduleAt(simTime()+(double)(*tIdle), timer);
    state = IDLE;
  }
}
