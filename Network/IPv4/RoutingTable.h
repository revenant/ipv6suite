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
//  Author:     Jochen Reber
//    Date:       18.5.00
//    On Linux:   19.5.00 - 29.5.00
//  Modified by Vincent Oberle
//    Date:       1.2.2001
//  Cleanup and rewrite: Andras Varga, 2004
//

#ifndef __ROUTINGTABLE_H
#define __ROUTINGTABLE_H

#include <vector>
#include <omnetpp.h>
#include "INETDefs.h"
#include "IPAddress.h"
#include "InterfaceTable.h"

class RoutingTableParser;


/*
 * Constants
 */
const int MAX_FILESIZE = 5000;
const int MAX_ENTRY_STRING_SIZE = 20;
const int MAX_GROUP_STRING_SIZE = 160;

/**
 * Interface entry for the interface table in RoutingTable.
 *
 * @see RoutingTable
 */
class IPv4InterfaceEntry : public cPolymorphic
{
  private:
    InterfaceEntry *ifBase; //< pointer into InterfaceTable
    IPAddress _inetAddr;    //< IP address of interface
    IPAddress _netmask;     //< netmask
    int _metric;            //< link "cost"; see e.g. MS KB article Q299540

  public: //XXX FIXME TBD just for now
    int multicastGroupCtr; //< table size
    IPAddress *multicastGroup;  //< dynamically allocated IPAddress table

  private:
    // copying not supported: following are private and also left undefined
    IPv4InterfaceEntry(const IPv4InterfaceEntry& obj);
    IPv4InterfaceEntry& operator=(const IPv4InterfaceEntry& obj);

  public:
    IPv4InterfaceEntry(InterfaceEntry *e);
    virtual ~IPv4InterfaceEntry() {}
    virtual std::string info() const;
    virtual std::string detailedInfo() const;

    IPAddress inetAddress() const  {return _inetAddr;}
    IPAddress netmask() const      {return _netmask;}
    int metric() const             {return _metric;}

    const char *name() const       {return ifBase->name();}
    int outputPort() const         {return ifBase->outputPort();}
    int mtu() const                {return ifBase->mtu();}
    bool isDown() const            {return ifBase->isDown();}
    bool isBroadcast() const       {return ifBase->isBroadcast();}
    bool isMulticast() const       {return ifBase->isMulticast();}
    bool isPointToPoint() const    {return ifBase->isPointToPoint();}
    bool isLoopback() const        {return ifBase->isLoopback();}
    //XXX int multicastGroupCtr; //< table size
    //XXX IPAddress *multicastGroup;  //< dynamically allocated IPAddress table

    void setInetAddress(IPAddress a) {_inetAddr = a;}
    void setNetmask(IPAddress m)     {_netmask = m;}
    void setMetric(int m)            {_metric = m;}
    //XXX int multicastGroupCtr; //< table size
    //XXX IPAddress *multicastGroup;  //< dynamically allocated IPAddress table

    void setName(const char *s)  {ifBase->setName(s);}
    void setOutputPort(int i)    {ifBase->setOutputPort(i);}
    void setMtu(int m)           {ifBase->setMtu(m);}
    void setDown(bool b)         {ifBase->setDown(b);}
    void setBroadcast(bool b)    {ifBase->setBroadcast(b);}
    void setMulticast(bool b)    {ifBase->setMulticast(b);}
    void setPointToPoint(bool b) {ifBase->setPointToPoint(b);}
    void setLoopback(bool b)     {ifBase->setLoopback(b);}
};


/**
 * Routing entry in RoutingTable.
 *
 * @see RoutingTable
 */
class RoutingEntry : public cPolymorphic
{
  public:
    /**
     * Route type
     */
    enum RouteType
    {
        DIRECT,  // Directly attached to the router
        REMOTE   // Reached through another router
    };

    /**
     * How the route was discovered
     */
    enum RouteSource
    {
        MANUAL,
        RIP,
        OSPF,
        BGP
    };

    /// Destination
    IPAddress host;

    /// Route mask
    IPAddress netmask;

    /// Next hop
    IPAddress gateway;

    /// Interface name and pointer
    opp_string interfaceName;
    IPv4InterfaceEntry *interfacePtr;

    /// Route type: Direct or Remote
    RouteType type;

    /// Source of route, MANUAL by reading a file,
    /// routing protocol name otherwise
    RouteSource source;

    /// Metric ("cost" to reach the destination)
    int metric;

    /// Route age (in seconds, since the route was last updated)
    //TBD not implemented yet
    int age;

  private:
    // copying not supported: following are private and also left undefined
    RoutingEntry(const RoutingEntry& obj);
    RoutingEntry& operator=(const RoutingEntry& obj);

  public:
    RoutingEntry();
    virtual ~RoutingEntry() {}
    virtual std::string info() const;
    virtual std::string detailedInfo() const;
};


/** Returned as the result of multicast routing */
struct MulticastRoute
{
    IPv4InterfaceEntry *interf;
    IPAddress gateway;
};
typedef std::vector<MulticastRoute> MulticastRoutes;


/**
 * Represents the routing table. This object has one instance per host
 * or router. It has methods to manage the route table and the interface table,
 * so one can active functionality similar to the "route" and "ifconfig" commands.
 *
 * See the NED documentation for general overview.
 *
 * This is a simple module without gates, it requires function calls to it
 * (message handling does nothing). Methods are provided for reading and
 * updating the interface table and the route table, as well as for unicast
 * and multicast routing.
 *
 * Interfaces are dynamically registered: at the start of the simulation,
 * every L2 module adds its own interface entry to the table.
 *
 * The route table is read from a file (RoutingTableParser); the file can
 * also fill in or overwrite interface settings. The route table can also
 * be read and modified during simulation, typically by routing protocol
 * implementations (e.g. OSPF).
 *
 * Interfaces are represented by IPv4InterfaceEntry objects, and entries in the
 * route table by RoutingEntry objects. Both can be polymorphic: if an
 * interface or a routing protocol needs to store additional data, it can
 * simply subclass from IPv4InterfaceEntry or RoutingEntry, and add the derived
 * object to the table.
 *
 * Uses RoutingTableParser to read routing files (.irt, .mrt).
 *
 *
 * @see IPv4InterfaceEntry, RoutingEntry
 */
class RoutingTable: public cSimpleModule
{
  private:

    //
    // Interfaces:
    //
    typedef std::vector<IPv4InterfaceEntry *> InterfaceVector;
    InterfaceVector interfaces;

    IPAddress routerId;
    bool IPForward;

    //
    // Routes:
    //
    typedef std::vector<RoutingEntry *> RouteVector;
    RouteVector routes;          // Unicast route array
    RouteVector multicastRoutes; // Multicast route array

  protected:
    // Add the entry of the local loopback interface
    IPv4InterfaceEntry *addLocalLoopback();

    // check if a route table entry corresponds to the following parameters
    bool routingEntryMatches(RoutingEntry *entry,
                             const IPAddress& target,
                             const IPAddress& nmask,
                             const IPAddress& gw,
                             int metric,
                             const char *dev);

    // the routing function
    RoutingEntry *selectBestMatchingRoute(const IPAddress& dest);

    // displays summary above the icon
    void updateDisplayString();

  public:
    Module_Class_Members(RoutingTable, cSimpleModule, 0);

    int numInitStages() const  {return 2;}
    void initialize(int stage);

    /**
     * Raises an error.
     */
    void handleMessage(cMessage *);

    /** @name Debug/utility */
    //@{
    void printIfconfig();
    void printRoutingTable();
    //@}

    /** @name Interfaces */
    //@{
    void addPv4InterfaceEntryFor(InterfaceEntry *e);

    /**
     * Returns the number of interfaces. Interface ids are in range
     * 0..numInterfaces()-1.
     */
    int numInterfaces()  {return interfaces.size();}

    /**
     * Returns the IPv4InterfaceEntry specified by its index 0..numInterfaces-1.
     */
    IPv4InterfaceEntry *interfaceAt(int id);

    /**
     * Returns an interface given by its port number (gate index).
     * Returns NULL if not found (e.g. the loopback interface has no
     * port number).
     */
    IPv4InterfaceEntry *interfaceByPortNo(int portNo);

    /**
     * Returns an interface given by its name. Returns NULL if not found.
     */
    IPv4InterfaceEntry *interfaceByName(const char *name);

    /**
     * Returns an interface given by its address. Returns NULL if not found.
     */
    IPv4InterfaceEntry *interfaceByAddress(const IPAddress& address);
    //@}

    /**
     * IP forwarding on/off
     */
    bool ipForward()  {return IPForward;}

    /**
     * Returns routerId.
     */
    IPAddress getRouterId()  {return routerId;}

    /** @name Routing functions (query the route table) */
    //@{
    /**
     * Checks if the address is a local one, i.e. one of the host's.
     */
    bool localDeliver(const IPAddress& dest);

    /**
     * Returns the port number to send the packets with dest as
     * destination address, or -1 if destination is not in routing table.
     */
    int outputPortNo(const IPAddress& dest);

    /**
     * Returns the gateway to send the destination,
     * address if the destination is not in routing table or there is
     * no gateway (local delivery).
     */
    // TBD maybe join with outputPortNo()
    IPAddress nextGatewayAddress(const IPAddress& dest);
    //@}

    /** @name Multicast routing functions */
    //@{

    /**
     * Checks if the address is in one of the local multicast group
     * address list.
     */
    bool multicastLocalDeliver(const IPAddress& dest);

    /**
     * Returns routes for a multicast address.
     */
    MulticastRoutes multicastRoutesFor(const IPAddress& dest);
    //@}

    /** @name Route table manipulation */
    //@{

    /**
     * Total number of routing entries (unicast, multicast entries and default route).
     */
    int numRoutingEntries();

    /**
     * Return kth routing entry.
     */
    RoutingEntry *routingEntry(int k);

    /**
     * Find first routing entry with the given parameters.
     */
    RoutingEntry *findRoutingEntry(const IPAddress& target,
                                   const IPAddress& netmask,
                                   const IPAddress& gw,
                                   int metric = 0,
                                   char *dev = NULL);

    /**
     * Adds a route to the routing table.
     */
    void addRoutingEntry(RoutingEntry *entry);

    /**
     * Deletes the given routes from the routing table.
     * Returns true if the route was deleted correctly, false if it was
     * not in the routing table.
     */
    bool deleteRoutingEntry(RoutingEntry *entry);
    //@}

};

#endif

