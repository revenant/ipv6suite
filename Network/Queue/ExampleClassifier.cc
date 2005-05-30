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
#include "ExampleClassifier.h"
#include "IPDatagram.h"
#ifdef WITH_IPv6
#include "IPv6Datagram.h"
#endif

Register_Class(ExampleClassifier);

int ExampleClassifier::numQueues()
{
    return 4;
}

int ExampleClassifier::classifyPacket(cMessage *msg)
{
    if (dynamic_cast<IPDatagram *>(msg))
    {
        // IPv4 QoS: map DSCP to queue number
        IPDatagram *datagram = (IPDatagram *)msg;
        int dscp = datagram->diffServCodePoint();
        return classifyByDSCP(dscp);
    }
    else if (dynamic_cast<IPv6Datagram *>(msg))
    {
        // IPv6 QoS: map Traffic Class to queue number
        IPv6Datagram *datagram = (IPv6Datagram *)msg;
        int dscp = datagram->trafficClass();
        return classifyByDSCP(dscp);
    }
    else
    {
        return 3; // lowest priority ("best effort")
    }
}

int ExampleClassifier::classifyByDSCP(int dscp)
{
    // DSCP is 6 bits, mask out all others
    dscp = (dscp & 0x3f);

    // DSCP format:
    //    xxxxx0: used by standards; see RFC 2474
    //    xxxxx1: experimental or local use
    // source: Stallings, High-Speed Networks, 2nd Ed, p494

    // all-zero DSCP maps to Best Effort (lowest priority)
    if (dscp==0)
        return 3;

    // assume Best Effort service for experimental or local DSCP's too
    if (dscp & 1)
        return 3;

    // from here on, we deal with non-zero standardized DSCP values only


}

