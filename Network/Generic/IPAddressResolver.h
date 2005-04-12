//
// Copyright (C) 2001  Vincent Oberle (vincent@oberle.com)
// Institute of Telematics, University of Karlsruhe, Germany.
// University Comillas, Madrid, Spain.
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


#ifndef __IPADDRESSRESOLVER_H
#define __IPADDRESSRESOLVER_H

#include <omnetpp.h>
#include "IPvXAddress.h"

class InterfaceTable;
class RoutingTable;
class RoutingTable6;

/**
 * Utility class for finding IPv4 or IPv6 address of a host or router.
 */
class IPAddressResolver
{
  private:
    // internal
    IPAddress getIPv4AddressFrom(InterfaceTable *ift);

    // internal
    IPv6Address_ getIPv6AddressFrom(InterfaceTable *ift);

  public:
    IPAddressResolver() {}
    ~IPAddressResolver() {}

    /**
     * Accepts dotted decimal notation ("127.0.0.1"), module name of the host
     * or router ("host[2]"), and empty string (""). For the latter, it returns
     * the null address. If module name is specified, the module will be
     * looked up using <tt>simulation.moduleByPath()</tt>, and then
     * addressOf() will be called to determine its IP address.
     */
    IPvXAddress resolve(const char *str);

    /**
     * Similar to resolve(), but returns false (instead of throwing an error)
     * if the address cannot be resolved because the given host (or interface)
     * doesn't have an address assigned yet. (It still throws an error
     * on any other error condition).
     */
    bool tryResolve(const char *str, IPvXAddress& result);

    /** @name Utility functions supporting resolve() */
    //@{
    /**
     * Returns IP or IPv6 address of the given host or router. If different interfaces
     * of the host/router have different IP addresses, the function throws
     * an error.
     *
     * This function uses routingTableOf() to find the RoutingTable module,
     * then invokes getAddressFrom() to extract the IP address.
     */
    IPvXAddress addressOf(cModule *host, bool preferIPv6=false);

    /**
     * Returns the IP or IPv6 address of the given host or router, given its InterfaceTable
     * module. If different interfaces have different IP addresses, the function
     * throws an error.
     */
    IPvXAddress getAddressFrom(InterfaceTable *ift, bool preferIPv6=false);

    /**
     * The function tries to look up the InterfaceTable module as submodule
     * <tt>"interfaceTable"</tt> or <tt>"networkLayer.interfaceTable"</tt> within
     * the host/router module. Throws an error if not found.
     */
    InterfaceTable *interfaceTableOf(cModule *host);

    /**
     * The function tries to look up the RoutingTable module as submodule
     * <tt>"routingTable"</tt> or <tt>"networkLayer.routingTable"</tt> within
     * the host/router module. Throws an error if not found.
     */
    RoutingTable *routingTableOf(cModule *host);

#ifdef WITH_IPv6
    /**
     * The function tries to look up the RoutingTable6 module as submodule
     * <tt>"routingTable6"</tt> or <tt>"networkLayer.routingTable6"</tt> within
     * the host/router module. Throws an error if not found.
     */
    RoutingTable6 *routingTable6Of(cModule *host);
#endif
    //@}
};


#endif


