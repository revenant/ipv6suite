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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <algorithm>
#include <sstream>

#include "stlwatch.h"
#include "InterfaceTable.h"


InterfaceEntry::InterfaceEntry()
{
    id = -1;
    outputPort = -1;

    static const IPAddress allOnes("255.255.255.255");
    mask = allOnes;

    mtu = 0;
    metric = 0;

    broadcast = false;
    multicast = false;
    pointToPoint= false;
    loopback = false;

    // add default mouticast groups!
    multicastGroupCtr = 0;
    multicastGroup = NULL;
}

std::string InterfaceEntry::info() const
{
    std::stringstream out;
    out << (!name.empty() ? name.c_str() : "*");
    out << "  outputPort:" << outputPort;
    out << "  addr:" << inetAddr << "  mask:" << mask;
    out << "  MTU:" << mtu << "  Metric:" << metric;
    return out.str();
}

std::string InterfaceEntry::detailedInfo() const
{
    std::stringstream out;
    out << "name:" << (!name.empty() ? name.c_str() : "*")
        << "\toutputPort:" << outputPort << "\n";
    out << "inet addr:" << inetAddr << "\tMask: " << mask << "\n";

    out << "MTU: " << mtu << " \tMetric: " << metric << "\n";

    out << "Groups:";
    for (int j=0; j<multicastGroupCtr; j++)
        if (!multicastGroup[j].isNull())
            out << "  " << multicastGroup[j];
    out << "\n";

    if (broadcast) out << "BROADCAST ";
    if (multicast) out << "MULTICAST ";
    if (pointToPoint) out << "POINTTOPOINT ";
    if (loopback) out << "LOOPBACK ";
    out << "\n";

    return out.str();
}

//==============================================================


Define_Module( InterfaceTable );


std::ostream& operator<<(std::ostream& os, const InterfaceEntry& e)
{
    os << e.info();
    return os;
};

void InterfaceTable::initialize(int stage)
{
    // L2 modules register themselves in stage 0, so we can only configure
    // the interfaces in stage 1. So we'll just do the whole initialize()
    // stuff in stage 1.
    if (stage!=1)
        return;

    IPForward = par("IPForward").boolValue();
    const char *filename = par("routingFile");


    WATCH_PTRVECTOR(interfaces);

    updateDisplayString();
}

void InterfaceTable::updateDisplayString()
{
    if (!ev.isGUI())
        return;

    char buf[80];
    if (routerId.isNull())
        sprintf(buf, "%d+%d routes", routes.size(), multicastRoutes.size());
    else
        sprintf(buf, "routerId: %s\n%d+%d routes", routerId.str().c_str(), routes.size(), multicastRoutes.size());
    displayString().setTagArg("t",0,buf);
}

void InterfaceTable::handleMessage(cMessage *msg)
{
    opp_error("This module doesn't process messages");
}

void InterfaceTable::printIfconfig()
{
    ev << "---- IF config ----\n";
    for (InterfaceVector::iterator i=interfaces.begin(); i!=interfaces.end(); ++i)
        ev << (*i)->detailedInfo() << "\n";
    ev << "\n";
}

void InterfaceTable::printInterfaceTable()
{
    ev << "-- Routing table --\n";
    ev.printf("%-16s %-16s %-16s %-3s %s\n",
              "Destination", "Gateway", "Netmask", "Iface");

    for (int i=0; i<numRoutingEntries(); i++)
        ev << routingEntry(i)->detailedInfo() << "\n";
    ev << "\n";
}

//---

InterfaceEntry *InterfaceTable::interfaceById(int id)
{
    if (id<0 || id>=(int)interfaces.size())
        opp_error("interfaceById(): nonexistent interface %d", id);
    return interfaces[id];
}

void InterfaceTable::addInterface(InterfaceEntry *entry)
{
    // check name and outputPort are unique
    if (interfaceByName(entry->name.c_str())!=NULL)
        opp_error("addInterface(): interface '%s' already registered", entry->name.c_str());
    if (entry->outputPort!=-1 && interfaceByPortNo(entry->outputPort)!=NULL)
        opp_error("addInterface(): interface with output=%d already registered", entry->outputPort);

    // insert
    entry->id = interfaces.size();
    interfaces.push_back(entry);
}

void InterfaceTable::deleteInterface(InterfaceEntry *entry)
{
    // check if any route table entries refer to this interface
    for (int k=0; k<numRoutingEntries(); k++)
        if (routingEntry(k)->interfacePtr==entry)
            opp_error("deleteInterface(): interface '%s' is still referenced by routes", entry->name.c_str());

    InterfaceVector::iterator i = std::find(interfaces.begin(), interfaces.end(), entry);
    if (i==interfaces.end())
        opp_error("deleteInterface(): interface '%s' not found in interface table", entry->name.c_str());

    interfaces.erase(i);
    delete entry;

    // renumber other interfaces
    for (i=interfaces.begin(); i!=interfaces.end(); ++i)
        (*i)->id = i-interfaces.begin();
}

InterfaceEntry *InterfaceTable::interfaceByPortNo(int portNo)
{
    // linear search is OK because normally we have don't have many interfaces (1..4, rarely more)
    for (InterfaceVector::iterator i=interfaces.begin(); i!=interfaces.end(); ++i)
        if ((*i)->outputPort==portNo)
            return *i;
    return NULL;
}

InterfaceEntry *InterfaceTable::interfaceByName(const char *name)
{
    Enter_Method("interfaceByName(%s)=?", name);
    if (!name)
        return NULL;
    for (InterfaceVector::iterator i=interfaces.begin(); i!=interfaces.end(); ++i)
        if (!strcmp(name, (*i)->name.c_str()))
            return *i;
    return NULL;
}

InterfaceEntry *InterfaceTable::interfaceByAddress(const IPAddress& addr)
{
    // This used to check the network part of the interface IP address.
    // No clue what it was good for, but screwed up routing for me. --Andras
    Enter_Method("interfaceByAddress(%s)=?", addr.str().c_str());
    if (addr.isNull())
        return NULL;
    for (InterfaceVector::iterator i=interfaces.begin(); i!=interfaces.end(); ++i)
        if ((*i)->inetAddr==addr)
            return *i;
    return NULL;
}

InterfaceEntry *InterfaceTable::addLocalLoopback()
{
    InterfaceEntry *loopbackInterface = new InterfaceEntry();

    loopbackInterface->name = "lo0";

    // 127.0.0.1/8 by default -- we may reconfigure later it to be the routerId
    loopbackInterface->inetAddr = IPAddress("127.0.0.1");
    loopbackInterface->mask = IPAddress("255.0.0.0");

    loopbackInterface->mtu = 3924;
    loopbackInterface->metric = 1;
    loopbackInterface->loopback = true;

    loopbackInterface->multicastGroupCtr = 0;
    loopbackInterface->multicastGroup = NULL;

    // add interface to table
    addInterface(loopbackInterface);

    return loopbackInterface;
}

