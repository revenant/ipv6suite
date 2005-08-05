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

#include "IPAddressResolver.h"
#include "InterfaceTable.h"

#include <assert.h>

BRSrcModel::~BRSrcModel()
{
    delete timer;
}

void BRSrcModel::sendPacket()
{
  //Section to create a packet, fragmentAndSend, and schedule next send
}

// Divide packet into fragment size and pass to lower layer.
void BRSrcModel::fragmentAndSend(unsigned long pktSize, int msgType)
{
    while(pktSize > fragmentLen)
    {
        sendFragment(fragmentLen, msgType);
        pktSize -= fragmentLen;
    }
    sendFragment(pktSize, msgType);
}

// Send fragment to lower layer
void BRSrcModel::sendFragment(unsigned long size, int msgType)
{
    // Create the packet along with its details
    BRMsg* pkt;
    pkt = new BRMsg("BRPacket");
    pkt->setType(msgType);
    pkt->setLength(size*8);
    pkt->setTimestamp();
    pkt->setSrcName(fullPath().c_str());
    pkt->setSequenceNo(sequenceNo);
    sequenceNo++;
    
    // Attach destination addr to control info, which
    // can be processed by link layer
    LL6ControlInfo *ctrlInfo = new LL6ControlInfo();
    const char* addr = resolveMACAddress(destAddr);
    assert(addr);
    ctrlInfo->setDestLLAddr(addr);
    pkt->setControlInfo(ctrlInfo);
    send(pkt, "brOut");  
}

const char* BRSrcModel::resolveMACAddress(std::string destName)
{
    cModule *mod = simulation.moduleByPath(destName.c_str());
    assert(mod != NULL);
    InterfaceTable *ift = IPAddressResolver().interfaceTableOf(mod);
    for (int i=0; i<ift->numInterfaces(); i++)
    {
        InterfaceEntry *ie = ift->interfaceAt(i);
        if(!ie->isLoopback())
        {
            return ie->llAddrStr();
        }
    }
    return 0;
}
