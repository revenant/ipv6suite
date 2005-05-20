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


#include "BRSrcModel.h"
#include "BRMsg_m.h"
#include "LL6ControlInfo_m.h"

BRSrcModel::~BRSrcModel()
{
    delete timer;
}

void BRSrcModel::sendPacket()
{
  //Section to create a packet, fragmentAndSend, and schedule next send
}

// Divide packet into fragment size and pass to lower layer.
void BRSrcModel::fragmentAndSend(unsigned long pktSize)
{
    while(pktSize > fragmentLen)
    {
        sendFragment(fragmentLen);
        pktSize -= fragmentLen;
    }
    sendFragment(pktSize);
}

// Send fragment to lower layer
void BRSrcModel::sendFragment(unsigned long size)
{
    // Create the packet along with its details
    BRMsg* pkt;
    pkt = new BRMsg("BRPacket");
    pkt->setLength(size*8);
    pkt->setTimestamp();
    pkt->setSrcName(fullPath().c_str());
    pkt->setSequenceNo(sequenceNo);
    sequenceNo++;
    
    // Attach destination addr to control info, which
    // can be processed by link layer
    LL6ControlInfo *ctrlInfo = new LL6ControlInfo();
    ctrlInfo->setDestLLAddr(destAddr.stringValue());
    pkt->setControlInfo(ctrlInfo);
    send(pkt, "brOut");  
}
