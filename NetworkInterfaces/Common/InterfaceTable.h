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

/**
 * Interface entry for the interface table in InterfaceTable.
 *
 * @see InterfaceTable
 */
class InterfaceEntry : public cPolymorphic
{
  public:
    int id;             //< identifies the interface (not the same as outputPort!)
    std::string name;   //< interface name (must be unique)
    int outputPort;     //< output gate index (-1 if unused, e.g. loopback interface)
    int mtu;            //< Maximum Transmission Unit (e.g. 1500 on Ethernet)
    bool isDown;        //< current state (up or down)
    bool broadcast;     //< interface supports broadcast
    bool multicast;     //< interface supports multicast
    bool pointToPoint;  //< interface is point-to-point link
    bool loopback;      //< interface is loopback interface

  private:
    // copying not supported: following are private and also left undefined
    InterfaceEntry(const InterfaceEntry& obj);
    InterfaceEntry& operator=(const InterfaceEntry& obj);

  public:
    InterfaceEntry();
    virtual ~InterfaceEntry() {}
    virtual std::string info() const;
    virtual std::string detailedInfo() const;
};


/**
 * Represents the interface table. This object has one instance per host
 * or router. It has methods to manage the interface table,
 * so one can access functionality similar to the "route" and "ifconfig" commands.
 *
 * See the NED documentation for general overview.
 *
 * This is a simple module without gates, it requires function calls to it
 * (message handling does nothing). Methods are provided for reading and
 * updating the interface table.
 *
 * Interfaces are dynamically registered: at the start of the simulation,
 * every L2 module adds its own interface entry to the table.
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

    void initialize();

    /**
     * Raises an error.
     */
    void handleMessage(cMessage *);

    /** @name Debug/utility */
    //@{
    void printIfconfig();
    void printInterfaceTable();
    //@}

    /** @name Interfaces */
    //@{
    /**
     * Returns the number of interfaces. Interface ids are in range
     * 0..numInterfaces()-1.
     */
    int numInterfaces()  {return interfaces.size();}

    /**
     * Returns the InterfaceEntry specified by its id (0..numInterfaces-1).
     */
    InterfaceEntry *interfaceById(int id);

    /**
     * Add an interface.
     */
    void addInterface(InterfaceEntry *entry);

    /**
     * Delete an interface. Throws an error if the interface is not in the
     * interface table. Indices of interfaces above this one will change!
     */
    void deleteInterface(InterfaceEntry *entry);

    /**
     * Returns an interface given by its port number (gate index).
     * Returns NULL if not found (e.g. the loopback interface has no
     * port number).
     */
    InterfaceEntry *interfaceByPortNo(int portNo);

    /**
     * Returns an interface given by its name. Returns NULL if not found.
     */
    InterfaceEntry *interfaceByName(const char *name);
    //@}
};

#endif

