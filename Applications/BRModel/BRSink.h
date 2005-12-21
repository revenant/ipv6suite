//
// Copyright (C) 2004 Monash University, Australia
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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


#ifndef __BRSINK_H__
#define __BRSINK_H__

#include <omnetpp.h>
#include <list>
#include "BRMsg_m.h"

/**
 * Consumes packets received from the BR modules and record statistics.
 */
class BRSink : public cSimpleModule
{
 public:
    Module_Class_Members(BRSink, cSimpleModule, 0);
    //
    // Stores info about each stream for a particular BR source
    //
    struct BRStreamInfo
    {
        std::string srcName;
        unsigned int numReceived;
        unsigned int numLost;
        unsigned int numPrevious;
        unsigned int expectedSeq;
        cStdDev* delayStat;
        cOutVector* numReceivedVec;
        cOutVector* numLostVec;
        cOutVector* avgDelayVec;
    };    
    
    ~BRSink();
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
 protected:
    typedef std::list<BRStreamInfo*> BRStreamList;
    BRStreamList streamList;  //list of BR stream
    typedef BRStreamList::iterator BRStreamListIt;
    
    void updateList(BRMsg *msg);
    void updateStreamListEntry(BRStreamListIt it, BRMsg* msg);
    void newStreamListEntry(std::string srcName);

    bool recordStats;
};


#endif


