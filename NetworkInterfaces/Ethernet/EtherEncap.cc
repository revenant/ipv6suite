/*
 * Copyright (C) 2003 CTIE, Monash University
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <stdio.h>
#include <omnetpp.h>

#include "Ethernet.h"
#include "EtherFrame_m.h"
#include "Ieee802Ctrl_m.h"
#include "EtherCtrl_m.h"
#include "utils.h"
#include "InterfaceTable.h"
#include "InterfaceTableAccess.h"
#include "EtherMAC.h"


/**
 * Performs Ethernet II encapsulation/decapsulation. More info in the NED file.
 */
class EtherEncap : public cSimpleModule
{
  protected:
    int seqNum;

    // statistics
    long totalFromHigherLayer;  // total number of packets received from higher layer
    long totalFromMAC;          // total number of frames received from MAC
    long totalPauseSent;        // total number of PAUSE frames sent

  public:
    Module_Class_Members(EtherEncap,cSimpleModule,0);

    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    virtual void finish();
    virtual void registerInterface();

    virtual void processPacketFromHigherLayer(cMessage *msg);
    virtual void processFrameFromMAC(EtherFrame *msg);
    virtual void handleSendPause(cMessage *msg);

    virtual void updateDisplayString();
};

Define_Module(EtherEncap);

void EtherEncap::initialize()
{
    seqNum = 0;
    WATCH(seqNum);

    totalFromHigherLayer = totalFromMAC = totalPauseSent = 0;
    WATCH(totalFromHigherLayer);
    WATCH(totalFromMAC);
    WATCH(totalPauseSent);

    // register ourselves in InterfaceTable
    registerInterface();
}

void EtherEncap::handleMessage(cMessage *msg)
{
    if (msg->arrivedOn("lowerLayerIn"))
    {
        processFrameFromMAC(check_and_cast<EtherFrame *>(msg));
    }
    else
    {
        // from higher layer
        switch(msg->kind())
        {
            case ETHCTRL_DATA:
            case 0: // default message kind (0) is also accepted
              processPacketFromHigherLayer(msg);
              break;

            case ETHCTRL_SENDPAUSE:
              // higher layer want MAC to send PAUSE frame
              handleSendPause(msg);
              break;

            default:
              error("received message `%s' with unknown message kind %d", msg->name(), msg->kind());
        }
    }

    if (ev.isGUI())
        updateDisplayString();
}

void EtherEncap::updateDisplayString()
{
    char buf[80];
    sprintf(buf, "passed up: %d\nsent: %d", totalFromMAC, totalFromHigherLayer);
    displayString().setTagArg("t",0,buf);
}

void EtherEncap::processPacketFromHigherLayer(cMessage *msg)
{
    if (msg->length()>8*MAX_ETHERNET_DATA)
        error("packet from higher layer (%d bytes) exceeds maximum Ethernet payload length (%d)", msg->length()/8, MAX_ETHERNET_DATA);

    totalFromHigherLayer++;

    // Creates MAC header information and encapsulates received higher layer data
    // with this information and transmits resultant frame to lower layer

    // create Ethernet frame, fill it in from Ieee802Ctrl and encapsulate msg in it
    EV << "Encapsulating higher layer packet `" << msg->name() <<"' for MAC\n";

    Ieee802Ctrl *etherctrl = check_and_cast<Ieee802Ctrl*>(msg->removeControlInfo());
    EthernetIIFrame *frame = new EthernetIIFrame(msg->name(), ETH_FRAME);

    frame->setSrc(etherctrl->getSrc());  // if blank, will be filled in by MAC
    frame->setDest(etherctrl->getDest());
    frame->setEtherType(etherctrl->getEtherType());
    frame->setLength(8*ETHER_MAC_FRAME_BYTES);
    delete etherctrl;

    frame->encapsulate(msg);
    if (frame->length() < 8*MIN_ETHERNET_FRAME)
        frame->setLength(8*MIN_ETHERNET_FRAME);  // "padding"

    send(frame, "lowerLayerOut");
}

void EtherEncap::processFrameFromMAC(EtherFrame *frame)
{
    totalFromMAC++;

    // decapsulate and attach control info
    cMessage *higherlayermsg = frame->decapsulate();

    // add Ieee802Ctrl to packet
    Ieee802Ctrl *etherctrl = new Ieee802Ctrl();
    etherctrl->setSrc(frame->getSrc());
    etherctrl->setDest(frame->getDest());
    higherlayermsg->setControlInfo(etherctrl);

    EV << "Decapsulating frame `" << frame->name() <<"', passing up contained "
          "packet `" << higherlayermsg->name() << "' to higher layer\n";

    // pass up to higher layers.
    send(higherlayermsg, "upperLayerOut");
    delete frame;
}

void EtherEncap::handleSendPause(cMessage *msg)
{
    EtherCtrl *etherctrl = dynamic_cast<EtherCtrl*>(msg->removeControlInfo());
    if (!etherctrl)
        error("PAUSE command `%s' from higher layer received without EtherCtrl", msg->name());
    int pauseUnits = etherctrl->getPauseUnits();
    delete etherctrl;

    EV << "Creating and sending PAUSE frame, with duration=" << pauseUnits << " units\n";

    // create Ethernet frame
    char framename[30];
    sprintf(framename, "pause-%d-%d", id(), seqNum++);
    EtherPauseFrame *frame = new EtherPauseFrame(framename, ETH_PAUSE);
    frame->setPauseTime(pauseUnits);

    frame->setLength(8*(ETHER_MAC_FRAME_BYTES+ETHER_PAUSE_COMMAND_BYTES));
    if (frame->length() < 8*MIN_ETHERNET_FRAME)
        frame->setLength(8*MIN_ETHERNET_FRAME);

    send(frame, "lowerLayerOut");
    delete msg;

    totalPauseSent++;
}

void EtherEncap::finish()
{
    if (par("writeScalars").boolValue())
    {
        recordScalar("packets from higher layer", totalFromHigherLayer);
        recordScalar("frames from MAC", totalFromMAC);
    }
}

void EtherEncap::registerInterface()
{
    InterfaceEntry *e = new InterfaceEntry();

    // interface name: NetworkInterface module's name without special characters ([])
    // --> Emin : Parent module name is used since EtherEncap belongs to EthernetInterface.
    char *interfaceName = new char[strlen(parentModule()->fullName())+1];
    char *d=interfaceName;
    for (const char *s=parentModule()->fullName(); *s; s++)
        if (isalnum(*s))
            *d++ = *s;
    *d = '\0';

    e->setName(interfaceName);
    delete [] interfaceName;

    e->_linkMod = NULL; // XXX remove _linkMod on the long term!! --AV
    MACAddress myMACAddress = ((EtherMAC *)parentModule()->submodule("mac"))->getMACAddress();
    e->setLLAddrStr(myMACAddress.str().c_str());

    // port: index of gate where parent module's "netwIn" is connected (in IP)
    int outputPort = parentModule()->gate("netwIn")->fromGate()->index();
    e->setOutputPort(outputPort);

    // MTU is 1500 on Ethernet
    e->setMtu(1500);

    // capabilities
    e->setBroadcast(true);
    e->setMulticast(true);
    e->setPointToPoint(false);

    // add
    InterfaceTable *ift = InterfaceTableAccess().get();
    ift->addInterface(e);
}


