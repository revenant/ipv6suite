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

#ifndef __WEQUEUE__
#define __WEQUEUE__

#include <omnetpp.h>
#include "WirelessEtherModule.h"

class WirelessEtherBasicFrame;

class WEQueue : public cPolymorphic
{
public:
    WEQueue() {}

    virtual ~WEQueue() {}

    virtual void initialise(WirelessEtherModule*) {}

    virtual void setMaxQueueSize(unsigned int) {}
    virtual void setRetryLimit(unsigned int) {}
    virtual unsigned int getRetry() { return 0; }
    virtual unsigned int getCW() { return 0; }
    virtual unsigned int getBackoffSlots() { return 0; }
    virtual int getQueueType() { return 1; }
    virtual int getAttempted(int) { return 0; }
    virtual int getCollided(int) { return 0; }
    virtual double getProbTxInSlot(int, double, WirelessEtherModule*) { return 0; }
    virtual double getLambda(int) { return 1; }
    virtual double getAvgFrameSize(int) { return 0; }
    virtual int size() { return 0; }
    virtual int sizeOfQ(int) { return size(); }

    virtual void reset() {}
    virtual void prepareNextFrame(double) {}
    virtual void insertFrame(WirelessEtherBasicFrame*, double) {}
    virtual WirelessEtherBasicFrame* getReadyFrame() { return NULL; }
    virtual bool initiateRetry(double) { return false; }
    virtual double getTimeToSend() { return 0; }
    virtual void contentionInterrupted(double) {}
    virtual void startingContention(double) {}
    virtual void startIdleReceive(double) {}
    virtual void endIdleReceive(double) {}
    virtual void startIdleCount(double) {}
    virtual void endIdleCount(double) {}
    virtual void startBusyCount(double) {}
    virtual void endBusyCount(double) {}
    virtual void sendingReadyFrame(double) {}

protected:

};

#endif
