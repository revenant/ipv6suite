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
#include "InterfaceEntry.h"


InterfaceEntry::InterfaceEntry()
{
    _outputPort = -1;

    _mtu = 0;

    _down = false;
    _broadcast = false;
    _multicast = false;
    _pointToPoint= false;
    _loopback = false;

    _ipv4data = NULL;
    _ipv6data = NULL;
    _protocol3data = NULL;
    _protocol4data = NULL;
}

std::string InterfaceEntry::info() const
{
    std::stringstream out;
    out << (!_name.empty() ? name() : "*");
    out << "  gateIndex:" << outputPort();
    out << "  MTU:" << mtu();
    if (isDown()) out << " DOWN";
    if (isLoopback()) out << " LOOPBACK";
    out << "  LLAddr:" << (llAddrStr()[0] ? llAddrStr() : "n/a");

    if (_ipv4data)
        out << " " << ((cPolymorphic*)_ipv4data)->info(); // Khmm...
    if (_ipv6data)
        out << " " << ((cPolymorphic*)_ipv6data)->info(); // Khmm...
    if (_protocol3data)
        out << " " << _protocol3data->info();
    if (_protocol4data)
        out << " " << _protocol4data->info();
    return out.str();
}

std::string InterfaceEntry::detailedInfo() const
{
    std::stringstream out;
    out << "name:" << (!_name.empty() ? name() : "*");
    out << "\toutputPort:" << outputPort() << "\n";
    out << "MTU: " << mtu() << " \t";
    if (isDown()) out << "DOWN ";
    if (isBroadcast()) out << "BROADCAST ";
    if (isMulticast()) out << "MULTICAST ";
    if (isPointToPoint()) out << "POINTTOPOINT ";
    if (isLoopback()) out << "LOOPBACK ";
    out << "\n";
    out << "(IP/IPv6 info in RoutingTables)\n";

    return out.str();
}

