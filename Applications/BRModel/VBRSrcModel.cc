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

#include "VBRSrcModel.h"

Define_Module(VBRSrcModel);

void VBRSrcModel::initialize()
{
  //Get NED parameters
  destAddr.set(par("destAddr").stringValue());  
  tStart = par("tStart");
  fragmentLen = par("fragmentLen");
  pixPerFrame = par("pixPerFrame");
  frameRate = par("frameRate");
  
  sequenceNo = 1;
  a=0.8781;
  b=0.1108;
  normalMean=0.572;
  previousBitRate=0;

  //Schedule first active period
  timer = new cMessage("VBRtimer");
  scheduleAt(tStart, timer);
}

void VBRSrcModel::handleMessage(cMessage* msg)
{
  if(msg->isSelfMessage())
  {
    sendPacket();
  }
  else
  {
  }
}

void VBRSrcModel::sendPacket()
{
  //calculate new bit rate (bits/pixel)
  double bitRate = a*previousBitRate + b*normal(normalMean,1);
  if(bitRate < 0)
    bitRate = 0;
  previousBitRate = bitRate;

  //send the frame size in bytes
  fragmentAndSend((unsigned long)((double)pixPerFrame*bitRate/8));

  //schedule next frame
  scheduleAt(simTime()+(1/(double)frameRate), timer);
}
