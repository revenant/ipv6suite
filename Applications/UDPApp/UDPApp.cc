//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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
#include "UDPApp.h"
#include "UDPControlInfo_m.h"
#include "StringTokenizer.h"
#include "IPAddressResolver.h"



Define_Module(UDPApp);

int UDPApp::counter;

void UDPApp::initialize(int stage)
{
    // because of IPAddressResolver, we need to wait until interfaces are registered,
    // address auto-assignment takes place etc.
    if (stage!=3)
        return;

    counter = 0;
    numSent = 0;
    WATCH(numSent);

    localPort = par("local_port");
    destPort = par("dest_port");
    msgLength = par("message_length");

    const char *destAddrs = par("dest_addresses");
    StringTokenizer tokenizer(destAddrs);
    const char *token;
    while ((token = tokenizer.nextToken())!=NULL)
        destAddresses.push_back(IPAddressResolver().resolve(token));

    if (destAddresses.empty())
        return;

    bindToPort(localPort);

    cMessage *timer = new cMessage("sendTimer");
    scheduleAt((double)par("message_freq"), timer);
}

IPvXAddress UDPApp::chooseDestAddr()
{
    int k = intrand(destAddresses.size());
    return destAddresses[k];
}

void UDPApp::sendPacket()
{
    char msgName[32];
    sprintf(msgName,"udpAppData-%d", counter++);

    cMessage *payload = new cMessage(msgName);
    payload->setLength(msgLength);

    IPvXAddress destAddr = chooseDestAddr();
    sendToUDP(payload, localPort, destAddr, destPort);

    numSent++;
}

void UDPApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        // send, then reschedule next sending
        sendPacket();
        scheduleAt(simTime()+(double)par("message_freq"), msg);
    }
    else
    {
        // process incoming packet
        processPacket(msg);
    }

    if (ev.isGUI())
    {
        char buf[40];
        sprintf(buf, "rcvd: %d pks\nsent: %d pks", numReceived, numSent);
        displayString().setTagArg("t",0,buf);
    }
}


