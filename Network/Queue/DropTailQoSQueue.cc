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


#include <omnetpp.h>
#include "DropTailQoSQueue.h"


Define_Module(DropTailQoSQueue);

void DropTailQoSQueue::initialize()
{
    // configuration
    frameCapacity = par("frameCapacity");

    // state
//    queue.setName("queue");
    packetRequested = 0;

    // statistics
    numReceived = 0;
    numDropped = 0;
    WATCH(numReceived);
    WATCH(numDropped);
}

void DropTailQoSQueue::handleMessage(cMessage *msg)
{
    numReceived++;
    if (packetRequested>0)
    {
//        ASSERT(queue.empty());
        packetRequested--;
        send(msg, "out");
    }
//    else if (frameCapacity && queue.length() >= frameCapacity)
    {
        ev << "Queue full, dropping packet.\n";
        delete msg;
        numDropped++;
    }
//    else
    {
//        queue.insert(msg);
    }

    if (ev.isGUI())
    {
        char buf[40];
        sprintf(buf, "rcvd: %d pks\ndropped: %d pks", numReceived, numDropped);
        displayString().setTagArg("t",0,buf);
    }
}

void DropTailQoSQueue::requestPacket()
{
//    if (queue.empty())
    {
        packetRequested++;
    }
//    else
    {
//        cMessage *msg = (cMessage *)queue.pop();
//        send(msg, "out");
    }
}



