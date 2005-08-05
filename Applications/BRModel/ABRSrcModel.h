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

#ifndef __ABRSRCMODEL_H_
#define __ABRSRCMODEL_H_

#include <omnetpp.h>
#include "BRSrcModel.h"

enum State 
{
    IDLE = 0,
    ACTIVE = 1
};

/**
 * Single-connection ABR application.
 */
class ABRSrcModel : public BRSrcModel
{
 public:
    Module_Class_Members(ABRSrcModel, BRSrcModel, 0);
    
    virtual void initialize();
    virtual void handleMessage(cMessage* msg);
 protected:
    virtual void sendPacket();
    
    int state;
    unsigned long packetsLeft; //packets left to tx in an active period
    unsigned long packetSize; //size of packet to be transmitted (bytes)
  
    //NED file parameters
    int msgType;           //msg type
    cPar *packetLen;       //bytes
    cPar *tIdle;           //seconds
    cPar *activePackets;   //packets
    cPar *tPause;          //seconds
    
 private:
};

#endif


