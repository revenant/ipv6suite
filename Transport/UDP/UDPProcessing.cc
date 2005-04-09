//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004 Andras Varga
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


//
// Author: Jochen Reber
// Cleanup and rewrite: Andras Varga 2004
//

#include <omnetpp.h>
#include <string.h>

#include "UDPPacket.h"
#include "UDPProcessing.h"
#include "IPControlInfo_m.h"
#include "IPv6ControlInfo_m.h"

Define_Module( UDPProcessing );

void UDPProcessing::initialize()
{
    applTable.size = gateSize("to_application");
    applTable.port = new int[applTable.size];  // FIXME free it in dtor (or change to std::map)

    // if there's only one app and without "local_port" parameter, don't dispatch
    // incoming packets by port number.
    if (applTable.size==1 && gate("to_application",0)->toGate() &&
        gate("to_application",0)->toGate()->ownerModule()->hasPar("local_port")==false)
    {
        dispatchByPort = false;
    }
    else
    {
        dispatchByPort = true;
        for (int i=0; i<applTable.size; i++)
        {
            // "local_port" parameter of connected app module
            cGate *appgate = gate("to_application",i)->toGate();
            applTable.port[i] = appgate ? appgate->ownerModule()->par("local_port") : -1;
        }
    }

    WATCH(dispatchByPort);

    numSent = 0;
    numPassedUp = 0;
    numDroppedWrongPort = 0;
    numDroppedBadChecksum = 0;
    WATCH(numSent);
    WATCH(numPassedUp);
    WATCH(numDroppedWrongPort);
    WATCH(numDroppedBadChecksum);
}

void UDPProcessing::handleMessage(cMessage *msg)
{
    // received from IP layer
    if (msg->arrivedOn("from_ip") || msg->arrivedOn("from_ipv6"))
    {
        processMsgFromIp(check_and_cast<UDPPacket *>(msg));
    }
    else // received from application layer
    {
        processMsgFromApp(msg);
    }

    if (ev.isGUI())
        updateDisplayString();
}

void UDPProcessing::updateDisplayString()
{
    char buf[80];
    sprintf(buf, "passed up: %d pks\nsent: %d pks", numPassedUp, numSent);
    if (numDroppedWrongPort>0)
    {
        sprintf(buf+strlen(buf), "\ndropped (no app): %d pks", numDroppedWrongPort);
        displayString().setTagArg("i",1,"red");
    }
    displayString().setTagArg("t",0,buf);
}

void UDPProcessing::processMsgFromIp(UDPPacket *udpPacket)
{
    // errorcheck, if applicable
    if (!udpPacket->checksumValid())
    {
        // throw packet away if bit error discovered
        // assume checksum found biterror
        delete udpPacket;
        numDroppedBadChecksum++;
        return;
    }

    // look up app gate
    int destPort = udpPacket->destinationPort();
    int appGateIndex = findAppGateForPort(destPort);
    if (appGateIndex == -1)
    {
        delete udpPacket;
        numDroppedWrongPort++;
        return;
    }

    // get src/dest addresses
    IPvXAddress srcAddr, destAddr;
    int inputPort;
    if (dynamic_cast<IPControlInfo *>(udpPacket->controlInfo())!=NULL)
    {
        IPControlInfo *controlInfo = (IPControlInfo *)udpPacket->removeControlInfo();
        srcAddr = controlInfo->srcAddr();
        destAddr = controlInfo->destAddr();
        inputPort = controlInfo->inputPort();
        delete controlInfo;
    }
    else if (dynamic_cast<IPv6ControlInfo *>(udpPacket->controlInfo())!=NULL)
    {
        IPv6ControlInfo *controlInfo = (IPv6ControlInfo *)udpPacket->removeControlInfo();
        srcAddr = controlInfo->srcAddr();
        destAddr = controlInfo->destAddr();
        inputPort = controlInfo->inputPort();
        delete controlInfo;
    }
    else
    {
        error("(%s)%s arrived without control info", udpPacket->className(), udpPacket->name());
    }

    // send payload with UDPControlInfo up to the application
    UDPControlInfo *udpControlInfo = new UDPControlInfo();
    udpControlInfo->setSrcAddr(srcAddr);
    udpControlInfo->setDestAddr(destAddr);
    udpControlInfo->setSrcPort(udpPacket->sourcePort());
    udpControlInfo->setDestPort(udpPacket->destinationPort());
    udpControlInfo->setInputPort(inputPort);

    cMessage *payload = udpPacket->decapsulate();
    payload->setControlInfo(udpControlInfo);
    delete udpPacket;

    send(payload, "to_application", appGateIndex);
    numPassedUp++;
}

void UDPProcessing::processMsgFromApp(cMessage *appData)
{
    UDPControlInfo *udpControlInfo = check_and_cast<UDPControlInfo *>(appData->removeControlInfo());

    UDPPacket *udpPacket = new UDPPacket(appData->name());
    udpPacket->setLength(8*UDP_HEADER_BYTES);
    udpPacket->encapsulate(appData);

    // set source and destination port
    udpPacket->setSourcePort(udpControlInfo->getSrcPort());
    udpPacket->setDestinationPort(udpControlInfo->getDestPort());

    if (!udpControlInfo->getDestAddr().isIPv6())
    {
        // send to IPv4
        IPControlInfo *ipControlInfo = new IPControlInfo();
        ipControlInfo->setProtocol(IP_PROT_UDP);
        ipControlInfo->setSrcAddr(udpControlInfo->getSrcAddr().get4());
        ipControlInfo->setDestAddr(udpControlInfo->getDestAddr().get4());
        udpPacket->setControlInfo(ipControlInfo);
        delete udpControlInfo;

        send(udpPacket,"to_ip");
    }
    else
    {
        // send to IPv6
        IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
        ipControlInfo->setProtocol(IP_PROT_UDP);
        ipControlInfo->setSrcAddr(udpControlInfo->getSrcAddr().get6());
        ipControlInfo->setDestAddr(udpControlInfo->getDestAddr().get6());
        udpPacket->setControlInfo(ipControlInfo);
        delete udpControlInfo;

        send(udpPacket,"to_ipv6");
    }
    numSent++;
}

int UDPProcessing::findAppGateForPort(int destPort)
{
    if (!dispatchByPort)
        return 0;

    for (int i=0; i<applTable.size; i++)
        if (applTable.port[i]==destPort)
            return i;
    return -1;
}

