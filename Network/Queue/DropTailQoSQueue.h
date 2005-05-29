//
// Copyright (C) 2005 Andras Varga
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


#ifndef __DROPTAILQUEUE_H__
#define __DROPTAILQUEUE_H__

#include <omnetpp.h>
#include "IPassiveQueue.h"
#include "IQoSClassifier.h"

/**
 * Drop-tail QoS queue. See NED for more info.
 */
class DropTailQoSQueue : public cSimpleModule, public IPassiveQueue
{
  protected:
    // configuration
    int frameCapacity;

    // state
    cQueue *queues;
    int packetRequested;
    IQoSClassifier *classifier;

    // statistics
    int numReceived;
    int numDropped;

  public:
    Module_Class_Members(DropTailQoSQueue, cSimpleModule, 0);
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

    /**
     * The queue should send a packet whenever this method is invoked.
     * If the queue is currently empty, it should send a packet when
     * when one becomes available.
     */
    virtual void requestPacket();
};

#endif


