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
#include "IPv4InterfaceData.h"
#include "InterfaceTable.h"
#include "InterfaceTableAccess.h"


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

void RoutingTable::initialize(int stage)
{
    // L2 modules register themselves in stage 0, so we can only configure
    // the interfaces in stage 1. So we'll just do the whole initialize()
    // stuff in stage 1.
    if (stage!=1)
        return;

    ift = InterfaceTableAccess().get();

    IPForward = par("IPForward").boolValue();
    const char *filename = par("routingFile");

    // At this point, all L2 modules have registered themselves (added their
    // interface entries). Create the per-interface IPv4 data structures.
    InterfaceTable *interfaceTable = InterfaceTableAccess().get();
    for (int i=0; i<interfaceTable->numInterfaces(); ++i)
        addPv4InterfaceEntryFor(interfaceTable->interfaceAt(i));

    // Add one extra interface, the loopback here.
    InterfaceEntry *lo0 = addLocalLoopback();

    // read routing table file (and interface configuration)
    RoutingTableParser parser(ift, this);
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
        for (int i=0; i<ift->numInterfaces(); ++i)
        {
            InterfaceEntry *ie = ift->interfaceAt(i);
            if (!ie->isLoopback() && ie->ipv4()->inetAddress().getInt() > routerId.getInt())
                routerId = ie->ipv4()->inetAddress();
        }
    }
    else
    {
        // use routerId both as routerId and loopback address
        routerId = IPAddress(routerIdStr);

        lo0->ipv4()->setInetAddress(routerId);
        lo0->ipv4()->setNetmask(IPAddress("255.255.255.255"));
    }

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

void RoutingTable::addPv4InterfaceEntryFor(InterfaceEntry *ie)
{
    IPv4InterfaceData *d = new IPv4InterfaceData();
    ie->setIPv4Data(d);

    // metric: some hints: OSPF cost (2e9/bps value), MS KB article Q299540, ...
    d->setMetric((int)ceil(2e9/ie->datarate())); // use OSPF cost as default
}

InterfaceEntry *RoutingTable::interfaceByAddress(const IPAddress& addr)
{
    Enter_Method("interfaceByAddress(%s)=?", addr.str().c_str());
    if (addr.isNull())
        return NULL;
    for (int i=0; i<ift->numInterfaces(); ++i)
    {
        InterfaceEntry *ie = ift->interfaceAt(i);
        if (ie->ipv4()->inetAddress()==addr)
            return ie;
    }
    return NULL;
}


InterfaceEntry *RoutingTable::addLocalLoopback()
{
    // TBD only add if not yet exists
    InterfaceEntry *ie = new InterfaceEntry();

    ie->setName("lo0");
    ie->setOutputPort(-1);
    ie->setMtu(3924);
    ie->setLoopback(true);

    // add IPv4 info. Set 127.0.0.1/8 as address by default --
    // we may reconfigure later it to be the routerId
    IPv4InterfaceData *d = new IPv4InterfaceData();
    d->setInetAddress(IPAddress("127.0.0.1"));
    d->setNetmask(IPAddress("255.0.0.0"));
    d->setMetric(1);
    ie->setIPv4Data(d);

    // add interface to table
    ift->addInterface(ie);
    return ie;
}

//---

bool RoutingTable::localDeliver(const IPAddress& dest)
{
    Enter_Method("localDeliver(%s) y/n", dest.str().c_str());

    // check if we have an interface with this address, obeying interface's netmask
    for (int i=0; i<ift->numInterfaces(); i++)
    {
        InterfaceEntry *ie = ift->interfaceAt(i);
        if (IPAddress::maskedAddrAreEqual(dest, ie->ipv4()->inetAddress(), ie->ipv4()->netmask()))
            return true;
    }
    return false;
}

bool RoutingTable::multicastLocalDeliver(const IPAddress& dest)
{
    Enter_Method("multicastLocalDeliver(%s) y/n", dest.str().c_str());

    for (int i=0; i<ift->numInterfaces(); i++)
    {
        InterfaceEntry *ie = ift->interfaceAt(i);
        for (int j=0; j < ie->ipv4()->multicastGroups().size(); j++)
            if (dest.equals(ie->ipv4()->multicastGroups()[j]))
                return true;
    }
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
            r.interf = ift->interfaceByName(e->interfaceName.c_str()); // Ughhhh
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
    entry->interfacePtr = ift->interfaceByName(entry->interfaceName.c_str());
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

