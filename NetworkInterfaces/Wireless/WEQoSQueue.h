// -*- C++ -*-
// Copyright (C) 2005 Monash University
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

/*
 * Single queue for regular 802.11 implementations
 */

#ifndef __WEQOSQUEUE__
#define __WEQOSQUEUE__

#include <list>
#include <omnetpp.h>
#include <cqueue.h>

#include "WEQueue.h"

class WirelessEtherBasicFrame;

class WEQoSQueue : public WEQueue
{
public:

    typedef struct accessCategoryQ
    {
        double AIFS;
        unsigned int CWmin;
        unsigned int CWmax;
        unsigned int retryLimit;

        cQueue queue;
        WirelessEtherBasicFrame* readyFrame;
        double contentionStartTime;
        double timeToSend;
        unsigned int CW;
        unsigned int backoffSlots;
        unsigned int retry;
        int collided;
        int attempted;
        int oRtCollided;
        int oRtAttempted;
        int rtCollided;
        int rtAttempted;
        int busySlots;
        int idleSlots;
        cStdDev* avgFrameSize;
        cStdDev* frameArrivalTime;
        cStdDev* accessTime;
        double lastSampledTime;
        double avgFrameArrivalTime;
        double avgAccessTime;
        double prevArrivalTime;
        double accessTimeStart;
        double bufferChecks;
        double bufferAvail;
        double receiveIdleStart;
        double receiveIdleTime;
        double lambda;
    }
    AccessCategoryQ;

    WEQoSQueue();
    virtual ~WEQoSQueue();

    virtual void initialise(WirelessEtherModule *);

    virtual void setMaxQueueSize(unsigned int);
    virtual void setRetryLimit(unsigned int);
    virtual unsigned int getRetry();
    virtual unsigned int getCW();
    virtual unsigned int getBackoffSlots();
    virtual int getQueueType();
    virtual int getAttempted(int);
    virtual int getCollided(int);
    virtual double getProbTxInSlot(int, double, WirelessEtherModule*);
    virtual double getLambda(int);
    virtual double getAvgFrameSize(int);
    virtual int size();
    virtual int sizeOfQ(int);

    virtual void reset();
    virtual void prepareNextFrame(double);
    virtual void insertFrame(WirelessEtherBasicFrame*, double);
    virtual WirelessEtherBasicFrame* getReadyFrame();
    virtual bool initiateRetry(double);
    virtual double getTimeToSend();
    virtual void contentionInterrupted(double);
    virtual void startingContention(double);
    virtual void startIdleReceive(double);
    virtual void endIdleReceive(double);
    virtual void startIdleCount(double);
    virtual void endIdleCount(double);
    virtual void startBusyCount(double);
    virtual void endBusyCount(double);
    virtual void sendingReadyFrame(double);

protected:
    void resetParamsForQ(int);
    void prepareNextFrameForQ(double, int);
    bool initiateRetryForQ(double, int);

    bool frameIsBroadcast(int);
    double calculateTimeToSendForQ(int);
    int soonestToSendQ();
    int resolveCollisionsWithQ(int);

    double slotTime;
    int maxQueueSize;
    double samplingStart;

    bool readyQSending;
    bool contentionStart;
    int servedQ;
    WirelessEtherModule *mod;

    double busyStartTime;
    double idleStartTime;

    std::list<int> acQToRetry;
    AccessCategoryQ acQ[4];
};


#endif
