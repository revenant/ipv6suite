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

#ifndef __IPv6CBRSRCMODEL_H_
#define __IPv6CBRSRCMODEL_H_

#include <omnetpp.h>
#include "CBRSrcModel.h"

/**
 * Single-connection CBR application.
 */
class IPv6CBRSrcModel : public CBRSrcModel
{
 public:
    Module_Class_Members(IPv6CBRSrcModel, CBRSrcModel, 0);
    
    virtual void initialize();
    virtual void handleMessage(cMessage* msg);
 protected:

    virtual cMessage* createControlInfo(BRMsg*);
};

#endif


