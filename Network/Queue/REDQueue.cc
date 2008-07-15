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
#include "REDQueue.h"


Define_Module(REDQueue);

void REDQueue::initialize()
{
    PassiveQueueBase::initialize();

    // configuration
    frameCapacity = par("frameCapacity");

    // state
    queue.setName("queue");

    error("REDQueue not implemented yet!");
}

bool REDQueue::enqueue(cMessage *msg)
{
    if (frameCapacity && queue.length() >= frameCapacity)
    {
        ev << "Queue full, dropping packet.\n";
        delete msg;
        return true;
    }
    else
    {
        queue.insert(msg);
        return false;
    }
}

cMessage *REDQueue::dequeue()
{
    if (queue.empty())
        return NULL;
    return (cMessage *)queue.pop();
}


