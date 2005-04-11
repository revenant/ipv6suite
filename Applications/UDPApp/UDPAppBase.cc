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

void UDPAppBase::bindToPort(int port)
{
    // bind ourselves to local_port in UDP
    cMessage *msg = new cMessage("UDP_C_BIND", UDP_C_BIND);
    UDPControlInfo *ctrl = new UDPControlInfo();
    ctrl->setSrcPort(port);
    msg->setControlInfo(ctrl);
    send(msg, "to_udp");
}

void UDPAppBase::sendToUDP(cMessage *msg, int srcPort, const IPvXAddress& destAddr, int destPort)
{
    // send message to UDP, with the appropriate control info attached
    msg->setKind(UDP_C_DATA);

    UDPControlInfo *ctrl = new UDPControlInfo();
    ctrl->setSrcPort(srcPort);
    ctrl->setDestAddr(destAddr);
    ctrl->setDestPort(destPort);
    msg->setControlInfo(ctrl);

    send(msg, "to_udp");
}

void UDPAppBase::printPacket(cMessage *msg)
{
    UDPControlInfo *ctrl = check_and_cast<UDPControlInfo *>(msg->controlInfo());

    IPvXAddress src = ctrl->getSrcAddr();
    IPvXAddress dest = ctrl->getDestAddr();
    int sentPort = ctrl->getSrcPort();
    int recPort = ctrl->getDestPort();

    ev  << msg << endl;
    ev  << "Payload length: " << (msg->length()/8) << " bytes" << endl;
    ev  << "Src/Port: " << src << " :" << sentPort << "  ";
    ev  << "Dest/Port: " << dest << ":" << recPort << endl;
}


