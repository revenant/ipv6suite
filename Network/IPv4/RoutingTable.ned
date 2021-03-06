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
// Keeps the table of all routing and interface information.
//
// Interfaces are dynamically registered by the corresponding L2 modules, e.g.
// PPPInterface. In addition to registered interfaces, a loopback interface
// will also be created. For hosts, the routerId parameter should be empty (""),
// in which case the loopback IP address will be set to 127.0.0.1/8.
// For routers which have a routerId set, the loopback address will be set to
// routerId. (See also RFC 2072 "\Router Renumbering Guide".)
//
// RouterId may also be set to the string <tt>"auto"</tt>, in which case the
// highest interface address will be chosen as routerId; the loopback address
// will remain 127.0.0.1/8.
//
// The routing table is read from a file (parameter routingFile);
// the file can also fill in or overwrite interface settings.
// The file format is documented <a href="irt.html">here</a>.
//
// Note that many protocols don't require routerId to be routable, but some
// others do -- so it is probably a good idea to set up routable routerIds.
//
// This module has no gates; all functionality can be accessed via member
// functions of the C++ module class. For detailed info, please see the C++
// documentation of the class (Doxygen).
//
// @author: Jochen Reber, Andras Varga
//
simple RoutingTable
    parameters:
        routerId: string, // for routers, the router id using IP address dotted
                          // notation; should be left empty ("") for hosts
        IPForward: bool,  // turns IP forwarding on/off
        routingFile: string;  // routing table file name
endsimple

