//
// Copyright (C) 2004 Andras Varga
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


//  Cleanup and rewrite: Andras Varga, 2004

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <algorithm>
#include <sstream>

#include "stlwatch.h"
#include "RoutingTable.h"
#include "RoutingTableParser.h"
#include "InterfaceTable.h"
#include "InterfaceTableAccess.h"



IPv4InterfaceEntry::IPv4InterfaceEntry(InterfaceEntry *e)
{
    ifBase = e;

    static const IPAddress allOnes("255.255.255.255");
    _netmask = allOnes;

    _metric = 0;

    // TBD add default multicast groups!
    multicastGroupCtr = 0;
    multicastGroup = NULL;
}

std::string IPv4InterfaceEntry::info() const
{
    std::stringstream out;
    out << (name()[0] ? name() : "*");
    out << "  outputPort:" << outputPort();
    out << "  addr:" << inetAddress() << "  mask:" << netmask();
    out << "  MTU:" << mtu() << "  Metric:" << metric();
    return out.str();
}

std::string IPv4InterfaceEntry::detailedInfo() const
{
    std::stringstream out;
    out << (name()[0] ? name() : "*");
    out << "\toutputPort:" << outputPort() << "\n";
    out << "inet addr:" << inetAddress() << "\tMask: " << netmask() << "\n";

    out << "MTU: " << mtu() << " \tMetric: " << metric() << "\n";

    out << "Groups:";
    for (int j=0; j<multicastGroupCtr; j++)
        if (!multicastGroup[j].isNull())
            out << "  " << multicastGroup[j];
    out << "\n";

    if (isDown()) out << "DOWN ";
    if (isBroadcast()) out << "BROADCAST ";
    if (isMulticast()) out << "MULTICAST ";
    if (isPointToPoint()) out << "POINTTOPOINT ";
    if (isLoopback()) out << "LOOPBACK ";
    out << "\n";

    return out.str();
}

//================================================================

RoutingEntry::RoutingEntry()
{
    interfacePtr = NULL;

    metric = 0;
    type = DIRECT;
    source = MANUAL;

    age = -1;
}

std::string RoutingEntry::info() const
{
    std::stringstream out;
    out << "dest:"; if (host.isNull()) out << "*  "; else out << host << "  ";
    out << "gw:"; if (gateway.isNull()) out << "*  "; else out << gateway << "  ";
    out << "mask:"; if (netmask.isNull()) out << "*  "; else out << netmask << "  ";
    out << "if:"; if (interfaceName.empty()) out << "*  "; else out << interfaceName.c_str() << "  ";
    out << (type==DIRECT ? "DIRECT" : "REMOTE");
    return out.str();
}

std::string RoutingEntry::detailedInfo() const
{
    return std::string();
}


//==============================================================


Define_Module( RoutingTable );


std::ostream& operator<<(std::ostream& os, const RoutingEntry& e)
{
    os << e.info();
    return os;
};

std::ostream& operator<<(std::ostream& os, const IPv4InterfaceEntry& e)
{
    os << e.info();
    return os;
};

void RoutingTable::initialize(int stage)
{
    // L2 modules register themselves in stage 0, so we can only configure
    // the interfaces in stage 1. So we'll just do the whole initialize()
    // stuff in stage 1.
    if (stage!=1)
        return;

    IPForward = par("IPForward").boolValue();
    const char *filename = par("routingFile");

    // At this point, all L2 modules have registered themselves (added their
    // interface entries). Create the per-interface IPv4 data structures.
    InterfaceTable *interfaceTable = InterfaceTableAccess().get();
    for (int i=0; i<interfaceTable->numInterfaces(); ++i)
        addPv4InterfaceEntryFor(interfaceTable->interfaceAt(i));

    // Add one extra interface, the loopback here.
    IPv4InterfaceEntry *lo0 = addLocalLoopback();

    // read routing table file (and interface configuration)
    RoutingTableParser parser(this);
    if (*filename && parser.readRoutingTableFromFile(filename)==-1)
        error("Error reading routing table file %s", filename);

    const char *routerIdStr = par("routerId").stringValue();
    if (!strcmp(routerIdStr, ""))
    {
        routerId = IPAddress();
    }
    else if (!strcmp(routerIdStr, "auto"))
    {
        // choose highest interface address as routerId
        routerId = IPAddress();
        for (InterfaceVector::iterator i=interfaces.begin(); i!=interfaces.end(); ++i)
            if (!(*i)->isLoopback() && (*i)->inetAddress().getInt() > routerId.getInt())
                routerId = (*i)->inetAddress();
    }
    else
    {
        // use routerId both as routerId and loopback address
        routerId = IPAddress(routerIdStr);

        lo0->setInetAddress(routerId);
        lo0->setNetmask(IPAddress("255.255.255.255"));
    }

    WATCH_PTRVECTOR(interfaces);
    WATCH_PTRVECTOR(routes);
    WATCH_PTRVECTOR(multicastRoutes);
    //WATCH(routerId);

    //printIfconfig();
    //printRoutingTable();

    updateDisplayString();
}

void RoutingTable::updateDisplayString()
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

void RoutingTable::handleMessage(cMessage *msg)
{
    opp_error("This module doesn't process messages");
}

void RoutingTable::printIfconfig()
{
    ev << "---- IF config ----\n";
    for (InterfaceVector::iterator i=interfaces.begin(); i!=interfaces.end(); ++i)
        ev << (*i)->detailedInfo() << "\n";
    ev << "\n";
}

void RoutingTable::printRoutingTable()
{
    ev << "-- Routing table --\n";
    ev.printf("%-16s %-16s %-16s %-3s %s\n",
              "Destination", "Gateway", "Netmask", "Iface");

    for (int i=0; i<numRoutingEntries(); i++)
        ev << routingEntry(i)->detailedInfo() << "\n";
    ev << "\n";
}

//---

void RoutingTable::addPv4InterfaceEntryFor(InterfaceEntry *e)
{
    IPv4InterfaceEntry *ipv4Interface = new IPv4InterfaceEntry(e);
    interfaces.push_back(ipv4Interface);

    // metric: some hints: OSPF cost (2e9/bps value), MS KB article Q299540, ...
    //XXX !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ipv4Interface->setMetric((int)ceil(2e9/datarate)); // use OSPF cost as default
}

IPv4InterfaceEntry *RoutingTable::interfaceAt(int id)
{
    if (id<0 || id>=(int)interfaces.size())
        opp_error("interfaceAt(): nonexistent interface %d", id);
    return interfaces[id];
}

IPv4InterfaceEntry *RoutingTable::interfaceByPortNo(int portNo)
{
    // linear search is OK because normally we have don't have many interfaces (1..4, rarely more)
    for (InterfaceVector::iterator i=interfaces.begin(); i!=interfaces.end(); ++i)
        if ((*i)->outputPort()==portNo)
            return *i;
    return NULL;
}

IPv4InterfaceEntry *RoutingTable::interfaceByName(const char *name)
{
    Enter_Method("interfaceByName(%s)=?", name);
    if (!name)
        return NULL;
    for (InterfaceVector::iterator i=interfaces.begin(); i!=interfaces.end(); ++i)
        if (!strcmp(name, (*i)->name()))
            return *i;
    return NULL;
}

IPv4InterfaceEntry *RoutingTable::interfaceByAddress(const IPAddress& addr)
{
    // This used to check the network part of the interface IP address.
    // No clue what it was good for, but screwed up routing for me. --Andras
    Enter_Method("interfaceByAddress(%s)=?", addr.str().c_str());
    if (addr.isNull())
        return NULL;
    for (InterfaceVector::iterator i=interfaces.begin(); i!=interfaces.end(); ++i)
        if ((*i)->inetAddress()==addr)
            return *i;
    return NULL;
}

IPv4InterfaceEntry *RoutingTable::addLocalLoopback()
{
/* FIXME TBD XXXXXXXXX!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    IPv4InterfaceEntry *loopbackInterface = new IPv4InterfaceEntry();

    loopbackInterface->setName = "lo0";

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
*/
return NULL; //XXXXXXXXXXXXXXXXXXXXXXXXXX!!!!!!!!!!!!!!!!!!!
}

//---

bool RoutingTable::localDeliver(const IPAddress& dest)
{
    Enter_Method("localDeliver(%s) y/n", dest.str().c_str());

    // check if we have an interface with this address, obeying interface's netmask
    for (InterfaceVector::iterator i=interfaces.begin(); i!=interfaces.end(); ++i)
        if (IPAddress::maskedAddrAreEqual(dest, (*i)->inetAddress(), (*i)->netmask()))
            return true;
    return false;
}

bool RoutingTable::multicastLocalDeliver(const IPAddress& dest)
{
    Enter_Method("multicastLocalDeliver(%s) y/n", dest.str().c_str());

    for (InterfaceVector::iterator i=interfaces.begin(); i!=interfaces.end(); ++i)
        for (int j=0; j < (*i)->multicastGroupCtr; j++)
            if (dest.equals((*i)->multicastGroup[j]))
                return true;
    return false;
}


RoutingEntry *RoutingTable::selectBestMatchingRoute(const IPAddress& dest)
{
    // find best match (one with longest prefix)
    // default route has zero prefix length, so (if exists) it'll be selected as last resort
    RoutingEntry *bestRoute = NULL;
    uint32 longestNetmask = 0;
    for (RouteVector::iterator i=routes.begin(); i!=routes.end(); ++i)
    {
        RoutingEntry *e = *i;
        if (IPAddress::maskedAddrAreEqual(dest, e->host, e->netmask) &&  // match
            (!bestRoute || e->netmask.getInt()>longestNetmask))  // longest so far
        {
            bestRoute = e;
            longestNetmask = e->netmask.getInt();
        }
    }
    return bestRoute;
}

int RoutingTable::outputPortNo(const IPAddress& dest)
{
    Enter_Method("outputPortNo(%s)=?", dest.str().c_str());

    RoutingEntry *e = selectBestMatchingRoute(dest);
    if (!e) return -1;
    return e->interfacePtr->outputPort();
}

IPAddress RoutingTable::nextGatewayAddress(const IPAddress& dest)
{
    Enter_Method("nextGatewayAddress(%s)=?", dest.str().c_str());

    RoutingEntry *e = selectBestMatchingRoute(dest);
    if (!e) return IPAddress();
    return e->gateway;
}


MulticastRoutes RoutingTable::multicastRoutesFor(const IPAddress& dest)
{
    Enter_Method("multicastRoutesFor(%s)=?", dest.str().c_str());

    MulticastRoutes res;
    res.reserve(16);
    for (RouteVector::iterator i=multicastRoutes.begin(); i!=multicastRoutes.end(); ++i)
    {
        RoutingEntry *e = *i;
        if (IPAddress::maskedAddrAreEqual(dest, e->host, e->netmask))
        {
            MulticastRoute r;
            r.interf = interfaceByName(e->interfaceName.c_str()); // Ughhhh
            r.gateway = e->gateway;
            res.push_back(r);
        }
    }
    return res;

}


int RoutingTable::numRoutingEntries()
{
    return routes.size()+multicastRoutes.size();
}

RoutingEntry *RoutingTable::routingEntry(int k)
{
    if (k < (int)routes.size())
        return routes[k];
    k -= routes.size();
    if (k < (int)multicastRoutes.size())
        return multicastRoutes[k];
    return NULL;
}

RoutingEntry *RoutingTable::findRoutingEntry(const IPAddress& target,
                                             const IPAddress& netmask,
                                             const IPAddress& gw,
                                             int metric,
                                             char *dev)
{
    int n = numRoutingEntries();
    for (int i=0; i<n; i++)
        if (routingEntryMatches(routingEntry(i), target, netmask, gw, metric, dev))
            return routingEntry(i);
    return NULL;
}

void RoutingTable::addRoutingEntry(RoutingEntry *entry)
{
    Enter_Method("addRoutingEntry(...)");

    // check for null address and default route
    if ((entry->host.isNull() || entry->netmask.isNull()) &&
        (!entry->host.isNull() || !entry->netmask.isNull()))
        error("addRoutingEntry(): to add a default route, set both host and netmask to zero");

    // fill in interface ptr from interface name
    entry->interfacePtr = interfaceByName(entry->interfaceName.c_str());
    if (!entry->interfacePtr)
        error("addRoutingEntry(): interface `%s' doesn't exist", entry->interfaceName.c_str());

    // add to tables
    if (!entry->host.isMulticast())
    {
        routes.push_back(entry);
    }
    else
    {
        multicastRoutes.push_back(entry);
    }

    updateDisplayString();
}


bool RoutingTable::deleteRoutingEntry(RoutingEntry *entry)
{
    Enter_Method("deleteRoutingEntry(...)");

    RouteVector::iterator i = std::find(routes.begin(), routes.end(), entry);
    if (i!=routes.end())
    {
        routes.erase(i);
        delete entry;
        updateDisplayString();
        return true;
    }
    i = std::find(multicastRoutes.begin(), multicastRoutes.end(), entry);
    if (i!=multicastRoutes.end())
    {
        multicastRoutes.erase(i);
        delete entry;
        updateDisplayString();
        return true;
    }
    return false;
}


bool RoutingTable::routingEntryMatches(RoutingEntry *entry,
                                const IPAddress& target,
                                const IPAddress& nmask,
                                const IPAddress& gw,
                                int metric,
                                const char *dev)
{
    if (!target.isNull() && !target.equals(entry->host))
        return false;
    if (!nmask.isNull() && !nmask.equals(entry->netmask))
        return false;
    if (!gw.isNull() && !gw.equals(entry->gateway))
        return false;
    if (metric && metric!=entry->metric)
        return false;
    if (dev && strcmp(dev, entry->interfaceName.c_str()))
        return false;

    return true;
}

