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

#ifndef __INTERFACETABLE_H
#define __INTERFACETABLE_H

#include <vector>
#include <omnetpp.h>
#include "INETDefs.h"

// Forward declarations. Do NOT #include the corresponding header files
// since that would create dependence on IPv4 and IPv6 stuff!
class IPv4InterfaceData;
class IPv6InterfaceData;

/**
 * Interface entry for the interface table in InterfaceTable.
 *
 * @see InterfaceTable
 */
class InterfaceEntry : public cPolymorphic
{
  private:
    std::string _name;   //< interface name (must be unique)
    int _outputPort;     //< output gate index (-1 if unused, e.g. loopback interface)
    int _mtu;            //< Maximum Transmission Unit (e.g. 1500 on Ethernet)
    bool _down;          //< current state (up or down)
    bool _broadcast;     //< interface supports broadcast
    bool _multicast;     //< interface supports multicast
    bool _pointToPoint;  //< interface is point-to-point link
    bool _loopback;      //< interface is loopback interface
    double _datarate;    //< data rate in bit/s

    IPv4InterfaceData *_ipv4data;   //< IPv4-specific interface info (IP address, etc)
    IPv6InterfaceData *_ipv6data;   //< IPv6-specific interface info (IPv6 addresses, etc)
    cPolymorphic *_protocol3data;   //< extension point: data for a 3rd network-layer protocol
    cPolymorphic *_protocol4data;   //< extension point: data for a 4th network-layer protocol

  private:
    // copying not supported: following are private and also left undefined
    InterfaceEntry(const InterfaceEntry& obj);
    InterfaceEntry& operator=(const InterfaceEntry& obj);

  public:
    InterfaceEntry();
    virtual ~InterfaceEntry() {}
    virtual std::string info() const;
    virtual std::string detailedInfo() const;

    const char *name() const       {return _name.c_str();}
    int outputPort() const         {return _outputPort;}
    int mtu() const                {return _mtu;}
    bool isDown() const            {return _down;}
    bool isBroadcast() const       {return _broadcast;}
    bool isMulticast() const       {return _multicast;}
    bool isPointToPoint() const    {return _pointToPoint;}
    bool isLoopback() const        {return _loopback;}
    double datarate() const        {return _datarate;}

    void setName(const char *s)  {_name = s;}
    void setOutputPort(int i)    {_outputPort = i;}
    void setMtu(int m)           {_mtu = m;}
    void setDown(bool b)         {_down = b;}
    void setBroadcast(bool b)    {_broadcast = b;}
    void setMulticast(bool b)    {_multicast = b;}
    void setPointToPoint(bool b) {_pointToPoint = b;}
    void setLoopback(bool b)     {_loopback = b;}
    void setDatarate(double d)   {_datarate = d;}

    IPv4InterfaceData *ipv4()    {return _ipv4data;}
    IPv6InterfaceData *ipv6()    {return _ipv6data;}
    cPolymorphic *protocol3()    {return _protocol3data;}
    cPolymorphic *protocol4()    {return _protocol4data;}

    void setIPv4Data(IPv4InterfaceData *p)  {_ipv4data = p;}
    void setIPv6Data(IPv6InterfaceData *p)  {_ipv6data = p;}
    void setProtocol3Data(cPolymorphic *p)  {_protocol3data = p;}
    void setProtocol4Data(cPolymorphic *p)  {_protocol4data = p;}
};


/**
 * Represents the interface table. This object has one instance per host
 * or router. It has methods to manage the interface table,
 * so one can access functionality similar to the "ifconfig" command.
 *
 * See the NED documentation for general overview.
 *
 * This is a simple module without gates, it requires function calls to it
 * (message handling does nothing). Methods are provided for reading and
 * updating the interface table.
 *
 * Interfaces are dynamically registered: at the start of the simulation,
 * every L2 module adds its own InterfaceEntry to the table; after that,
 * IPv4's RoutingTable and IPv6's RoutingTable6 (an possibly, further
 * L3 protocols) add protocol-specific data on each InterfaceEntry
 * (see IPv4InterfaceData, IPv6InterfaceData, and InterfaceEntry::setIPv4Data(),
 * InterfaceEntry::setIPv6Data())
 *
 * Interfaces are represented by InterfaceEntry objects.
 *
 * @see InterfaceEntry
 */
class InterfaceTable: public cSimpleModule
{
  private:
    typedef std::vector<InterfaceEntry *> InterfaceVector;
    InterfaceVector interfaces;

  protected:
    // displays summary above the icon
    void updateDisplayString();

  public:
    Module_Class_Members(InterfaceTable, cSimpleModule, 0);

    int numInitStages() const {return 2;}
    void initialize(int stage);

    /**
     * Raises an error.
     */
    void handleMessage(cMessage *);

    /**
     * Adds an interface.
     *
     * Note: Interface deletion is not supported, but one can mark one
     * as "down".
     */
    void addInterface(InterfaceEntry *entry);

    /**
     * Returns the number of interfaces.
     */
    int numInterfaces()  {return interfaces.size();}

    /**
     * Returns the InterfaceEntry specified by an index 0..numInterfaces-1.
     */
    InterfaceEntry *interfaceAt(int pos);

    /**
     * Returns an interface given by its port number (gate index).
     * Returns NULL if not found.
     */
    InterfaceEntry *interfaceByPortNo(int portNo);

    /**
     * Returns an interface given by its name. Returns NULL if not found.
     */
    InterfaceEntry *interfaceByName(const char *name);
};

#endif

