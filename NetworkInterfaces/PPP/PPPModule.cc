//
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

#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "InterfaceTable.h"
#include "InterfaceTableAccess.h"
#include "PPPModule.h"
#include "IPassiveQueue.h"



Define_Module(PPPModule);

void PPPModule::initialize(int stage)
{
    if (stage==3)
    {
        // update display string when addresses have been autoconfigured etc.
        updateDisplayString();
        return;
    }

    // all initialization is done in the first stage
    if (stage!=0)
        return;

    queue.setName("queue");
    endTransmissionEvent = new cMessage("pppEndTxEvent");

    maxQueueLength = par("maxQueueLength");

    interfaceEntry = NULL;

    numSent = numRcvdOK = numBitErr = numDropped = 0;
    WATCH(numSent);
    WATCH(numRcvdOK);
    WATCH(numBitErr);
    WATCH(numDropped);

    // find queueModule
    queueModule = NULL;
    if (par("queueModule").stringValue()[0])
    {
        cModule *mod = parentModule()->submodule(par("queueModule").stringValue());
        queueModule = check_and_cast<IPassiveQueue *>(mod);
    }

    // we're connected if other end of connection path is an input gate
    cGate *physOut = gate("physOut");
    connected = physOut->destinationGate()->type()=='I';

    // if we're connected, get the gate with transmission rate
    gateToWatch = physOut;
    datarate = 0;
    if (connected)
    {
        while (gateToWatch)
        {
            // does this gate have data rate?
            cSimpleChannel *chan = dynamic_cast<cSimpleChannel*>(gateToWatch->channel());
            if (chan && (datarate=chan->datarate())>0)
                break;
            // otherwise just check next connection in path
            gateToWatch = gateToWatch->toGate();
        }
        if (!gateToWatch)
            error("gate physOut must be connected (directly or indirectly) to a link with data rate");
    }

    // register our interface entry in RoutingTable
    interfaceEntry = registerInterface(datarate);

    // if not connected, make it gray
    if (ev.isGUI())
    {
        if (!connected)
        {
            displayString().setTagArg("i",1,"#707070");
            displayString().setTagArg("i",2,"100");
        }
    }

    // request first frame to send
    if (queueModule)
    {
        EV << "Requesting first frame from queue module\n";
        queueModule->requestPacket();
    }
}

InterfaceEntry *PPPModule::registerInterface(double datarate)
{
    InterfaceEntry *e = new InterfaceEntry();

    // interface name: our module name without special characters ([])
    char *interfaceName = new char[strlen(parentModule()->fullName())+1];
    char *d=interfaceName;
    for (const char *s=parentModule()->fullName(); *s; s++)
        if (isalnum(*s))
            *d++ = *s;
    *d = '\0';

    e->setName(interfaceName);
    delete [] interfaceName;

    e->_linkMod = this; // XXX remove _linkMod on the long term!! --AV

    // port: index of gate where our "netwIn" is connected (in IP)
    int outputPort = parentModule()->gate("netwIn")->fromGate()->index();
    e->setOutputPort(outputPort);

    // generate a link-layer address to be used as interface token for IPv6
    InterfaceToken token(0, simulation.getUniqueNumber(), 64);
    e->setInterfaceToken(token);

    // and convert it to a string, for llAddrStr
    char buf[32];
    sprintf(buf, "%8.8lx:%8.8lx", token.normal(), token.low());
    e->setLLAddrStr(buf);  //XXX doesn't e->setLLAddrStr("unspec-ppp") suffice? --AV

    // MTU: typical values are 576 (Internet de facto), 1500 (Ethernet-friendly),
    // 4000 (on some point-to-point links), 4470 (Cisco routers default, FDDI compatible)
    e->setMtu(4470);

    // capabilities
    e->setMulticast(true);
    e->setPointToPoint(true);

    // add
    InterfaceTable *ift = InterfaceTableAccess().get();
    ift->addInterface(e);

    return e;
}


void PPPModule::startTransmitting(cMessage *msg)
{
    // if there's any control info, remove it; then encapsulate the packet
    delete msg->removeControlInfo();
    PPPFrame *pppFrame = encapsulate(msg);
    if (ev.isGUI()) displayBusy();

    ev << "Starting transmission of " << pppFrame << endl;
    send(pppFrame, "physOut");

    // schedule an event for the time when last bit will leave the gate.
    simtime_t endTransmission = gateToWatch->transmissionFinishes();
    scheduleAt(endTransmission, endTransmissionEvent);
}

void PPPModule::handleMessage(cMessage *msg)
{
    if (!connected)
    {
        ev << "Interface is not connected, dropping packet " << msg << endl;
        delete msg;
        numDropped++;
    }
    else if (msg==endTransmissionEvent)
    {
        // Transmission finished, we can start next one.
        ev << "Transmission finished.\n";
        if (ev.isGUI()) displayIdle();

        if (!queue.empty())
        {
            msg = (cMessage *) queue.getTail();
            startTransmitting(msg);
            numSent++;
        }
        else if (queueModule)
        {
            // tell queue module that we've become idle
            queueModule->requestPacket();
        }
    }
    else if (msg->arrivedOn("physIn"))
    {
        // check for bit errors
        if (msg->hasBitError())
        {
            ev << "Bit error in " << msg << endl;
            numBitErr++;
            delete msg;
        }
        else
        {
            // pass up payload
            cMessage *payload = decapsulate(check_and_cast<PPPFrame *>(msg));
            numRcvdOK++;
            send(payload,"netwOut");
        }
    }
    else // arrived on gate "netwIn"
    {
        if (endTransmissionEvent->isScheduled())
        {
            // We are currently busy, so just queue up the packet.
            ev << "Received " << msg << " for transmission but transmitter busy, queueing.\n";
            if (ev.isGUI() && queue.length()>=3) displayString().setTagArg("i",1,"red");

            if (maxQueueLength && queue.length() >= maxQueueLength)
            {
                ev << "Queue full, dropping packet.\n";
                delete msg;
                numDropped++;
            }
            else
            {
                queue.insert(msg);
            }

        }
        else
        {
            // We are idle, so we can start transmitting right away.
            ev << "Received " << msg << " for transmission\n";
            startTransmitting(msg);
            numSent++;
        }
    }

    if (ev.isGUI())
        updateDisplayString();

}

void PPPModule::displayBusy()
{
    displayString().setTagArg("i",1, queue.length()>=3 ? "red" : "yellow");
    gateToWatch->displayString().setTagArg("o",0,"yellow");
    gateToWatch->displayString().setTagArg("o",1,"3");
    gate("physOut")->displayString().setTagArg("o",0,"yellow");
    gate("physOut")->displayString().setTagArg("o",1,"3");
}

void PPPModule::displayIdle()
{
    displayString().setTagArg("i",1,"");
    gateToWatch->displayString().setTagArg("o",0,"black");
    gateToWatch->displayString().setTagArg("o",1,"1");
    gate("physOut")->displayString().setTagArg("o",0,"black");
    gate("physOut")->displayString().setTagArg("o",1,"1");
}

void PPPModule::updateDisplayString()
{
    char buf[80];
    if (ev.disabled())
    {
        // speed up things
        displayString().setTagArg("t",0,"");
    }
    else if (connected)
    {
        char drate[40];
        if (datarate>=1e9) sprintf(drate,"%gG", datarate/1e9);
        else if (datarate>=1e6) sprintf(drate,"%gM", datarate/1e6);
        else if (datarate>=1e3) sprintf(drate,"%gK", datarate/1e3);
        else sprintf(drate,"%gbps", datarate);

/* TBD FIXME find solution for displaying IP address without dependence on IPv6 or IPv6
        IPAddress addr = interfaceEntry->ipv4()->inetAddress();
        sprintf(buf, "%s / %s\nrcv:%ld snt:%ld", addr.isNull()?"-":addr.str().c_str(), drate, numRcvdOK, numSent);
*/
        sprintf(buf, "%s\nrcv:%ld snt:%ld", drate, numRcvdOK, numSent);

        if (numBitErr>0 || numDropped>0)
            sprintf(buf+strlen(buf), "\nerr:%ld drop:%ld", numBitErr, numDropped);

        displayString().setTagArg("t",0,buf);
    }
    else
    {
        sprintf(buf, "not connected\ndropped:%ld", numDropped);
        displayString().setTagArg("t",0,buf);
    }
}

PPPFrame *PPPModule::encapsulate(cMessage *msg)
{
    PPPFrame *pppFrame = new PPPFrame(msg->name());
    pppFrame->setLength(8*PPP_OVERHEAD_BYTES);
    pppFrame->encapsulate(msg);
    return pppFrame;
}

cMessage *PPPModule::decapsulate(PPPFrame *pppFrame)
{
    cMessage *msg = pppFrame->decapsulate();
    delete pppFrame;
    return msg;
}


