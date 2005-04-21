//
// Copyright (C) 2004 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//


#include "IPAddressResolver.h"
#include "InterfaceTable.h"
#include "IPv4InterfaceData.h"
#include "RoutingTable.h"
#ifdef WITH_IPv6
#include "IPv6InterfaceData.h"
#include "RoutingTable6.h"
#endif

IPvXAddress IPAddressResolver::resolve(const char *s, bool preferIPv6)
{
    IPvXAddress addr;
    if (!tryResolve(s, addr, preferIPv6))
        opp_error("IPAddressResolver: required address `%s' hasn't been assigned yet (try later!)", s);
    return addr;
}

bool IPAddressResolver::tryResolve(const char *s, IPvXAddress& result, bool preferIPv6)
{
    // empty address
    result = IPvXAddress();
    if (!s || !*s)
        return true;

    // handle address literal
    if (result.tryParse(s))
        return true;

    // must be "modulename/interfacename(protocol)" syntax then,
    // "/interfacename" and "(protocol)" being optional
    const char *slashp = strchr(s,'/');
    const char *leftparenp = strchr(s,'(');
    const char *rightparenp = strchr(s,')');
    const char *endp = s+strlen(s);

    // rudimentary syntax check
    if ((slashp && leftparenp && slashp>leftparenp) ||
        (leftparenp && !rightparenp) ||
        (!leftparenp && rightparenp) ||
        (rightparenp && rightparenp!=endp-1))
    {
        opp_error("IPAddressResolver: syntax error parsing address spec `%s'", s);
    }

    // parse fields
    std::string modname, ifname, protocol;
    modname.assign(s, (slashp?slashp:leftparenp?leftparenp:endp)-s);
    if (slashp)
        ifname.assign(slashp+1, (leftparenp?leftparenp:endp)-slashp-1);
    if (leftparenp)
        protocol.assign(leftparenp+1, rightparenp-leftparenp-1);

    // find module, use protocol
    cModule *mod = simulation.moduleByPath(modname.c_str());
    if (!mod)
        opp_error("IPAddressResolver: module `%s' not found", modname.c_str());
    if (!protocol.empty() && protocol!="ipv4" && protocol!="ipv6")
        opp_error("IPAddressResolver: error parsing address spec `%s': protocol must be `ipv4' or `ipv6'", s);

    // FIXME TBD: FINISH!!! parse requested address type, and pass it as preferIPv6 below
    result = addressOf(mod);
    // FIXME TBD: if address is not of requested type, throw an error

    return true;
}

IPvXAddress IPAddressResolver::addressOf(cModule *host, bool preferIPv6)
{
    InterfaceTable *ift = interfaceTableOf(host);
    return getAddressFrom(ift, preferIPv6);
}

IPvXAddress IPAddressResolver::getAddressFrom(InterfaceTable *ift, bool preferIPv6)
{
    IPvXAddress ret;
    if (preferIPv6)
    {
        ret = getIPv6AddressFrom(ift);
        if (ret.isNull())
            ret = getIPv4AddressFrom(ift);
    }
    else
    {
        ret = getIPv4AddressFrom(ift);
        if (ret.isNull())
            ret = getIPv6AddressFrom(ift);
    }
    return ret;
}

IPAddress IPAddressResolver::getIPv4AddressFrom(InterfaceTable *ift)
{
    // browse interfaces: for the purposes of this function, all of them should
    // share the same IP address
    IPAddress addr;
    if (ift->numInterfaces()==0)
        opp_error("IPAddressResolver: interface table `%s' has no interface registered "
                  "(yet? try in a later init stage!)", ift->fullPath().c_str());

    for (int i=0; i<ift->numInterfaces(); i++)
    {
        InterfaceEntry *ie = ift->interfaceAt(i);
        if (!ie->ipv4()->inetAddress().isNull() && !ie->isLoopback())
        {
            if (!addr.isNull() && ie->ipv4()->inetAddress()!=addr)
                opp_error("IPAddressResolver: IP address is ambiguous: different "
                          "interfaces in `%s' have different IP addresses",
                          ift->fullPath().c_str());
            addr = ie->ipv4()->inetAddress();
        }
    }

    //if (addr.isNull())
    //    opp_error("IPAddressResolver: no interface in `%s' has an IP address "
    //              "assigned (yet? try in a later init stage!)", ift->fullPath().c_str());

    return addr;
}

IPv6Address_ IPAddressResolver::getIPv6AddressFrom(InterfaceTable *ift)
{
    // browse interfaces: for the purposes of this function, all of them should
    // share the same IP address
    IPv6Address_ addr;
    if (ift->numInterfaces()==0)
        opp_error("IPAddressResolver: interface table `%s' has no interface registered "
                  "(yet? try in a later init stage!)", ift->fullPath().c_str());

    for (int i=0; i<ift->numInterfaces(); i++)
    {
        InterfaceEntry *ie = ift->interfaceAt(i);

        if (ie->isLoopback())
          continue;
        
        for ( int j = 0; j < ie->ipv6()->inetAddrs.size(); j++)
        {
          if ( ie->ipv6()->inetAddrs[j].scope() != ipv6_addr::Scope_Link)
          {
            addr.set(ie->ipv6()->inetAddrs[j].addressSansPrefix().c_str());
            break;
          }
        }
    }
    return addr;
}

InterfaceTable *IPAddressResolver::interfaceTableOf(cModule *host)
{
    // find InterfaceTable
    cModule *mod = host->submodule("interfaceTable");
    if (!mod)
        mod = host->moduleByRelativePath("networkLayer.interfaceTable");
    if (!mod)
        opp_error("IPAddressResolver: InterfaceTable not found as `interfaceTable' or "
                  "`networkLayer.interfaceTable' within host/router module `%s'",
                  host->fullPath().c_str());
    return check_and_cast<InterfaceTable *>(mod);
}

RoutingTable *IPAddressResolver::routingTableOf(cModule *host)
{
    // find RoutingTable
    cModule *mod = host->submodule("routingTable");
    if (!mod)
        mod = host->moduleByRelativePath("networkLayer.routingTable");
    if (!mod)
        opp_error("IPAddressResolver: RoutingTable not found as `routingTable' or "
                  "`networkLayer.routingTable' within host/router module `%s'",
                  host->fullPath().c_str());
    return check_and_cast<RoutingTable *>(mod);
}

#ifdef WITH_IPv6
RoutingTable6 *IPAddressResolver::routingTable6Of(cModule *host)
{
    // find RoutingTable
    cModule *mod = host->submodule("routingTable6");
    if (!mod)
        mod = host->moduleByRelativePath("networkLayer.proc.routingTable6");
    if (!mod)
        opp_error("IPAddressResolver: RoutingTable6 not found as `routingTable6' or "
                  "`networkLayer.routingTable6' within host/router module `%s'",
                  host->fullPath().c_str());
    return check_and_cast<RoutingTable6 *>(mod);
}
#endif


