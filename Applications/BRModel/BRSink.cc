//
// Copyright (C) 2004 Monash University, Australia
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//


#include <omnetpp.h>
#include "BRSink.h"
#include "BRMsg_m.h"
#include <assert.h>

Define_Module(BRSink);

BRSink::~BRSink()
{
    // free space allocated for list
    for(BRStreamListIt it = streamList.begin(); it != streamList.end(); it++)
    {
        delete (*it)->delayStat;
        delete (*it)->numReceivedVec;
        delete (*it)->numLostVec;
        delete (*it)->avgDelayVec;
        delete (*it);    
    }
}

void BRSink::initialize()
{}

void BRSink::handleMessage(cMessage *msg)
{
    if(!msg->isSelfMessage())
        updateList(check_and_cast<BRMsg*>(msg));
    
    delete msg;
}

void BRSink::finish()
{
    std::string varName;
    // print statistics collected for each BR stream
    for(BRStreamListIt it = streamList.begin(); it != streamList.end(); it++)
    {
        recordScalar((*it)->delayStat->name(), (*it)->delayStat->mean());
        varName = "BRNumReceivedStat."+(*it)->srcName;
        recordScalar(varName.c_str(), (*it)->numReceived);
        varName = "BRNumLostStat."+(*it)->srcName;
        recordScalar(varName.c_str(), (*it)->numLost);
    }
}

//
// Update statistics for a particular BR source who sent the frame. If 
// it is a new source, a new entry will be added. 
//
void BRSink::updateList(BRMsg *msg)
{
    bool found = false;
    for(BRStreamListIt it = streamList.begin(); it != streamList.end(); it++)
    {
        // If BR source is in the list, update its statistics
        if((*it)->srcName == msg->getSrcName())
        {
            updateStreamListEntry(it, msg);
            found = true;
            break;
        }
    }
    // BR source is not in list, therefore create a new entry for it
    if(found == false)
    {
        newStreamListEntry(msg->getSrcName());
        updateStreamListEntry(--streamList.end(), msg);
    }
}

//
// Update statistics for a specified BR source entry
//
void BRSink::updateStreamListEntry(BRStreamListIt it, BRMsg* msg)
{
    assert(msg->getSequenceNo() >= (*it)->expectedSeq);
    (*it)->numReceived++;
    (*it)->numReceivedVec->record((*it)->numReceived); 
    (*it)->numLost += msg->getSequenceNo()-(*it)->expectedSeq;
    (*it)->numLostVec->record((*it)->numLost);
    (*it)->expectedSeq = msg->getSequenceNo()+1;
    (*it)->delayStat->collect(simTime()-msg->timestamp());
    (*it)->avgDelayVec->record((*it)->delayStat->mean());
}

//
// Create a new BR source entry in the list
//
void BRSink::newStreamListEntry(std::string srcName)
{
    std::string varName;
    BRStreamInfo* newStream = new BRStreamInfo;
    newStream->srcName = srcName;
    newStream->numReceived = 0;
    newStream->numLost = 0;
    newStream->expectedSeq = 1;
    varName = "BRDelayStat"+srcName;
    newStream->delayStat = new cStdDev(varName.c_str());
    varName = "BRNumReceivedVec."+srcName;
    newStream->numReceivedVec = new cOutVector(varName.c_str());
    varName = "BRNumLostVec."+srcName;
    newStream->numLostVec = new cOutVector(varName.c_str());
    varName = "BRAvgDelayVec."+srcName;
    newStream->avgDelayVec = new cOutVector(varName.c_str());
    
    streamList.push_back(newStream);
}
