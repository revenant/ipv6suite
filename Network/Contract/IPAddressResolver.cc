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



IPvXAddress IPAddressResolver::resolve(const char *s, int addrType)
{
    IPvXAddress addr;
    if (!tryResolve(s, addr, addrType))
        opp_error("IPAddressResolver: address `%s' not configured (yet?)", s);
    return addr;
}

bool IPAddressResolver::tryResolve(const char *s, IPvXAddress& result, int addrType)
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

    // parse fields: modname, ifname, protocol
    std::string modname, ifname, protocol;
    modname.assign(s, (slashp?slashp:leftparenp?leftparenp:endp)-s);
    if (slashp)
        ifname.assign(slashp+1, (leftparenp?leftparenp:endp)-slashp-1);
    if (leftparenp)
        protocol.assign(leftparenp+1, rightparenp-leftparenp-1);

    // find module and check protocol
    cModule *mod = simulation.moduleByPath(modname.c_str());
    if (!mod)
        opp_error("IPAddressResolver: module `%s' not found", modname.c_str());
    if (!protocol.empty() && protocol!="ipv4" && protocol!="ipv6")
        opp_error("IPAddressResolver: error parsing address spec `%s': address type must be `(ipv4)' or `(ipv6)'", s);
    if (!protocol.empty())
        addrType = protocol=="ipv4" ? ADDR_IPv4 : ADDR_IPv6;

    // get address from the given module/interface
    if (ifname.empty())
        result = addressOf(mod, addrType);
    else
        result = addressOf(mod, ifname.c_str(), addrType);
    return !result.isNull();
}

IPvXAddress IPAddressResolver::addressOf(cModule *host, int addrType)
{
    InterfaceTable *ift = interfaceTableOf(host);
    return getAddressFrom(ift, addrType);
}

IPvXAddress IPAddressResolver::addressOf(cModule *host, const char *ifname, int addrType)
{
    InterfaceTable *ift = interfaceTableOf(host);
    InterfaceEntry *ie = ift->interfaceByName(ifname);
    if (!ie)
        opp_error("IPAddressResolver: no interface called `%s' in `%s'", ifname, ift->fullPath().c_str());
    return getAddressFrom(ie, addrType);
}

IPvXAddress IPAddressResolver::getAddressFrom(InterfaceTable *ift, int addrType)
{
    IPvXAddress ret;
    if (addrType==ADDR_IPv6 || addrType==ADDR_PREFER_IPv6)
    {
        ret = getIPv6AddressFrom(ift);
        if (ret.isNull() && addrType==ADDR_PREFER_IPv6)
            ret = getIPv4AddressFrom(ift);
    }
    else if (addrType==ADDR_IPv4 || addrType==ADDR_PREFER_IPv4)
    {
        ret = getIPv4AddressFrom(ift);
        if (ret.isNull() && addrType==ADDR_PREFER_IPv4)
            ret = getIPv6AddressFrom(ift);
    }
    else
    {
        opp_error("IPAddressResolver: unknown addrType %d", addrType);
    }
    return ret;
}

IPvXAddress IPAddressResolver::getAddressFrom(InterfaceEntry *ie, int addrType)
{
    IPvXAddress ret;
    if (addrType==ADDR_IPv6 || addrType==ADDR_PREFER_IPv6)
    {
        if (ie->ipv6())
            ret = getInterfaceIPv6Address(ie);
        if (ret.isNull() && addrType==ADDR_PREFER_IPv6 && ie->ipv4())
            ret = ie->ipv4()->inetAddress();
    }
    else if (addrType==ADDR_IPv4 || addrType==ADDR_PREFER_IPv4)
    {
        if (ie->ipv4())
            ret = ie->ipv4()->inetAddress();
        if (ret.isNull() && addrType==ADDR_PREFER_IPv4 && ie->ipv6())
            ret = getInterfaceIPv6Address(ie);
    }
    else
    {
        opp_error("IPAddressResolver: unknown addrType %d", addrType);
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

    for (unsigned int i=0; i<ift->numInterfaces(); i++)
    {
        InterfaceEntry *ie = ift->interfaceAt(i);
        if (ie->ipv4() && !ie->ipv4()->inetAddress().isNull() && !ie->isLoopback())
        {
            if (!addr.isNull() && ie->ipv4()->inetAddress()!=addr)
                opp_error("IPAddressResolver: IP address is ambiguous: different "
                          "interfaces in `%s' have different IP addresses",
                          ift->fullPath().c_str());
            addr = ie->ipv4()->inetAddress();
        }
    }
    return addr;
}

IPv6Address_ IPAddressResolver::getIPv6AddressFrom(InterfaceTable *ift)
{
    // browse interfaces: for the purposes of this function, all of them should
    // share the same IP address
    if (ift->numInterfaces()==0)
        opp_error("IPAddressResolver: interface table `%s' has no interface registered "
                  "(yet? try in a later init stage!)", ift->fullPath().c_str());

    // we prefer globally routable addresses, but if there's none, link scope addresses are also accepted
    IPv6Address_ addr;
    addr = getIPv6AddressFrom(ift, ipv6_addr::Scope_Global);
    if (addr.isNull())
        addr = getIPv6AddressFrom(ift, ipv6_addr::Scope_Organization);
    if (addr.isNull())
        addr = getIPv6AddressFrom(ift, ipv6_addr::Scope_Site);
    if (addr.isNull())
        addr = getIPv6AddressFrom(ift, ipv6_addr::Scope_Link);
    if (addr.isNull())
        addr = getIPv6AddressFrom(ift, ipv6_addr::Scope_Node);
    return addr;
}

IPv6Address_ IPAddressResolver::getIPv6AddressFrom(InterfaceTable *ift, int scope)
{
    IPv6Address_ addr;
    for (unsigned int i=0; i<ift->numInterfaces() && addr.isNull(); i++)
    {
        InterfaceEntry *ie = ift->interfaceAt(i);

        if (!ie->ipv6() || ie->isLoopback())
            continue;

        for (unsigned int j = 0; j < ie->ipv6()->inetAddrs.size(); j++)
        {
            if (ie->ipv6()->inetAddrs[j].scope()==scope)
            {
                addr.set(ie->ipv6()->inetAddrs[j].addressSansPrefix().c_str()); // FIXME conversion via string
                break;
            }
        }
    }
    return addr;
}

IPv6Address_ IPAddressResolver::getInterfaceIPv6Address(InterfaceEntry *ie)
{
    IPv6Address_ addr;
    int scope = ipv6_addr::Scope_None;

    // code below assumes that "better" (more routable) addresses compare larger than less weaker ones
    ASSERT(ipv6_addr::Scope_Global > ipv6_addr::Scope_Organization &&
           ipv6_addr::Scope_Organization > ipv6_addr::Scope_Site &&
           ipv6_addr::Scope_Site > ipv6_addr::Scope_Link &&
           ipv6_addr::Scope_Link > ipv6_addr::Scope_Node &&
           ipv6_addr::Scope_Node > ipv6_addr::Scope_None);

    // find "best" address
    for (unsigned int j = 0; j < ie->ipv6()->inetAddrs.size(); j++)
        if (ie->ipv6()->inetAddrs[j].scope() > scope)
            addr.set(ie->ipv6()->inetAddrs[j].addressSansPrefix().c_str()); // FIXME conversion via string
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


