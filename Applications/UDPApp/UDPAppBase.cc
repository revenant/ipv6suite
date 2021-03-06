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
#include "UDPAppBase.h"
#include "UDPControlInfo_m.h"
#include <iomanip>

void UDPAppBase::bindToPort(int port)
{
    ev << "Binding to UDP port " << port << endl;

    cMessage *msg = new cMessage("UDP_C_BIND", UDP_C_BIND);
    UDPControlInfo *ctrl = new UDPControlInfo();
    ctrl->setSrcPort(port);
    msg->setControlInfo(ctrl);
    send(msg, "to_udp");
}

void UDPAppBase::sendToUDP(cMessage *msg, int srcPort, const IPvXAddress& destAddr, int destPort)
{
  UDPControlInfo *ctrl = new UDPControlInfo();
  if (msg->kind() == 1)
    ctrl->setTrace(true);

    // send message to UDP, with the appropriate control info attached
    msg->setKind(UDP_C_DATA);

    ctrl->setSrcPort(srcPort);
    ctrl->setDestAddr(destAddr);
    ctrl->setDestPort(destPort);
    msg->setControlInfo(ctrl);

    ev << "Sending packet: ";
    printPacket(msg);

    send(msg, "to_udp");
}

void UDPAppBase::printPacket(cMessage *msg)
{
    UDPControlInfo *ctrl = check_and_cast<UDPControlInfo *>(msg->controlInfo());

    IPvXAddress srcAddr = ctrl->getSrcAddr();
    IPvXAddress destAddr = ctrl->getDestAddr();
    int srcPort = ctrl->getSrcPort();
    int destPort = ctrl->getDestPort();

    ev  << msg << "  (" <<std::dec<< (msg->length()/8) << " bytes)" << endl;
    ev  << srcAddr << " :" << srcPort << " --> " << destAddr << ":" << destPort << endl;
}


