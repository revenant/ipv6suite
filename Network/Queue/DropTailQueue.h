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
#include "PassiveQueueBase.h"

/**
 * Drop-tail queue. See NED for more info.
 */
class DropTailQueue : public PassiveQueueBase
{
  protected:
    // configuration
    int frameCapacity;

    // state
    cQueue queue;

    // FIXME TBD add statistics, queue length output vector etc

  public:
    Module_Class_Members(DropTailQueue, PassiveQueueBase, 0);
    virtual void initialize();

    /**
     * Redefined from PassiveQueueBase.
     */
    virtual bool enqueue(cMessage *msg);

    /**
     * Redefined from PassiveQueueBase.
     */
    virtual cMessage *dequeue();
};

#endif


