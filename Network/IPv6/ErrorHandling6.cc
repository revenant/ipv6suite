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
#include "ErrorHandling6.h"
#include "IPv6Datagram.h"
#include "ICMPv6Message.h"


Define_Module(ErrorHandling6);

void ErrorHandling6::initialize()
{
}

void ErrorHandling6::handleMessage(cMessage *msg)
{
    ICMPv6Message *icmpMsg = check_and_cast<ICMPv6Message *>(msg);
    IPv6Datagram *d = check_and_cast<IPv6Datagram *>(icmpMsg->encapsulatedMsg());

    ev << "Error Handler: ICMPv6 message received:\n";
    ev << " Type:    " << (int)icmpMsg->type()
       << " Code:    " << (int)icmpMsg->code()
       << " OptInfo: " << icmpMsg->optInfo()
       << " Bytelength: " << d->length()/8
       << " Src:     " << d->srcAddress()
       << " Dest:    " << d->destAddress()
       << " Time:    " << simTime()
       << "\n";

    delete icmpMsg;
}

