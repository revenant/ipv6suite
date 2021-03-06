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

#ifndef __BRSRCMODEL_H_
#define __BRSRCMODEL_H_

#include <omnetpp.h>
#include "MACAddress6.h"

class BRMsg;

/**
 * Base class for BR applications.
 * Contains functionality common to all Bit Rate models, namely sendFragment and fragmentAndSend
 */
class BRSrcModel : public cSimpleModule
{
 public:
    Module_Class_Members(BRSrcModel, cSimpleModule, 0);
    ~BRSrcModel();
    
 protected:
    virtual void sendPacket();
    void sendFragment(unsigned long, int msgType);
    void fragmentAndSend(unsigned long, int msgType);
    const char* resolveMACAddress(std::string);

    // create control packet for MAC layer - can be redefined suitably
    // for various other layer protocol
    virtual cMessage* createControlInfo(BRMsg*);
    
    cMessage* timer;
    unsigned long sequenceNo;
    
    //NED file parameters
    std::string destAddr;      //mac address of destination
    unsigned long fragmentLen; //bytes
    double tStart;             //seconds
    
 private:
};

#endif
