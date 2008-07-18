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

#ifndef __CBRSRCMODEL_H_
#define __CBRSRCMODEL_H_

#include <omnetpp.h>
#include "BRSrcModel.h"

/**
 * Single-connection CBR application.
 */
class CBRSrcModel : public BRSrcModel
{
 public:
    Module_Class_Members(CBRSrcModel, BRSrcModel, 0);
    
    virtual void initialize();
    virtual void handleMessage(cMessage* msg);
 protected:
    virtual void sendPacket();
    
    int msgType;           //msg type    
    cPar *bitRate;
    unsigned long packetSize;
    double sendPeriod;
    
 private:
};

#endif


