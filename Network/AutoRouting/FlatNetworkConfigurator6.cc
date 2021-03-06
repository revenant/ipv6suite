//
// Copyright (C) 2005 Eric Wu
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

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <algorithm>
#include "FlatNetworkConfigurator6.h"
#include "IPv6InterfaceData.h"
#include "RoutingTable6.h"
#include "InterfaceTable.h"

#include "IPAddressResolver.h"
#include "ipv6addrconv.h"



Define_Module(FlatNetworkConfigurator6);


void FlatNetworkConfigurator6::initialize(int stage)
{
    // FIXME function too long, break it up!!!

    cTopology topo("topo");

    std::vector<std::string> types = cStringTokenizer(par("moduleTypes"), " ").asVector();
    topo.extractByModuleType(types);
    ev << "cTopology found " << topo.nodes() << " nodes\n";

    // Although bus types are not auto-configured, FNC must still know them
    // since topology may depend on them.
    std::vector<std::string> nonIPTypes = cStringTokenizer(par("nonIPModuleTypes"), " ").asVector();

    if (stage == 2)
    {
        for (int i = 0; i < topo.nodes(); i++)
        {
            // skip bus types
            if (std::find(nonIPTypes.begin(), nonIPTypes.end(), topo.node(i)->module()->className()) != nonIPTypes.end())
                continue;

            // find interface table and assign address to all (non-loopback) interfaces
            cModule *mod = topo.node(i)->module();

            // only assign address when it is a router. host can
            // obtain addresses through mannual address configuration
            // or auto address configuration
            if (mod->submodule("networkLayer")->par("IPForward").boolValue() == false)
              continue;

            InterfaceTable *ift = IPAddressResolver().interfaceTableOf(mod);

            for (unsigned int k = 0; k < ift->numInterfaces(); k++)
            {
                InterfaceEntry *ie = ift->interfaceAt(k);
                if (!ie->isLoopback())
                {
                    bool linkLocalAddrAssigned = false;
                    bool globalAddrAssigned = false;

                    for (unsigned int l = 0; l < ie->ipv6()->tentativeAddrs.size(); l++)
                    {
                        if (ie->ipv6()->tentativeAddrs[l].scope() == ipv6_addr::Scope_Global)
                            globalAddrAssigned = true;

                        if (ie->ipv6()->tentativeAddrs[l].scope() == ipv6_addr::Scope_Link)
                            linkLocalAddrAssigned = true;
                    }

                    ipv6_addr addr = {(unsigned int) uniform(0, 0xffffffff),(unsigned int) uniform(0, 0xffffffff),(unsigned int) uniform(0, 0xffffffff),(unsigned int) uniform(0, 0xffffffff) };
                    while (ipv6_addr_scope(addr) != ipv6_addr::Scope_Global)
                    {
                        addr.extreme = (unsigned int) uniform(0, 0xffffffff);
                        addr.high = (unsigned int) uniform(0, 0xffffffff);
                    }

                    if (!globalAddrAssigned)
                    {
                        // add in global address
                        ie->ipv6()->tentativeAddrs.push_back(IPv6Address(addr, 64));
                        ev << "tentative global address, "<< IPv6Address(addr, 64) 
                           <<" assigned to "<< mod->name() 
                           << ", interface ID: "<< ie->outputPort() << "\n";
                    }

                    if (!linkLocalAddrAssigned)
                    {
                        // add in link-local address
                        ipv6_addr linklocalAddr = addr;
                        linklocalAddr.extreme = 0xfe800000;
                        linklocalAddr.high = 0x0;
                        ie->ipv6()->tentativeAddrs.push_back(IPv6Address(linklocalAddr, 128));

                        ev << "link local address, "<< IPv6Address(linklocalAddr, 128) 
                           <<" assigned to "<< mod->name() 
                           << ", interface ID: "<< ie->outputPort() << "\n";
                    }
                }
            }
        }
    }

    if (stage != 3)
        return;

    int numIPNodes = 0;

    // add default route to nodes with exactly one (non-loopback) interface
    std::vector<bool> usesDefaultRoute;
    usesDefaultRoute.resize(topo.nodes());
    for (int i = 0; i < topo.nodes(); i++)
    {
        cTopology::Node * node = topo.node(i);

        // skip bus types
        if (std::find(nonIPTypes.begin(), nonIPTypes.end(), topo.node(i)->module()->className()) != nonIPTypes.end())
            continue;

        numIPNodes++;

        InterfaceTable *ift = IPAddressResolver().interfaceTableOf(node->module());
        RoutingTable6 *rt = IPAddressResolver().routingTable6Of(node->module());

        // count non-loopback interfaces
        int numIntf = 0;
        InterfaceEntry *ie = NULL;
        for (unsigned int k = 0; k < ift->numInterfaces(); k++)
            if (!ift->interfaceAt(k)->isLoopback())
            {
                ie = ift->interfaceAt(k);
                numIntf++;
            }

        usesDefaultRoute[i] = (numIntf == 1);
        if (numIntf != 1)
            continue;           // only deal with nodes with one interface plus loopback

        // not connect to any node, could be wireless
        if (!node->outLinks())
            continue;

        // determine the next hop address
        cGate *remoteGate = node->out(0)->remoteGate();

        int remoteGateIdx = remoteGate->index();
        cModule *nextHop = remoteGate->ownerModule();
        InterfaceEntry *nextHopIf = IPAddressResolver().interfaceTableOf(nextHop)->interfaceByPortNo(remoteGateIdx);

	// for default route, destination address is 0:0:0:0:0:0:0:0/0
	// [ref] RoutingTable6::addRoute()
        IPv6Address defaultAddr("0:0:0:0:0:0:0:0/0"); 

        for (unsigned int k = 0; k < nextHopIf->ipv6()->tentativeAddrs.size(); k++)
        {
            IPv6Address nextHopAddr = nextHopIf->ipv6()->tentativeAddrs[k];

            if (nextHopAddr.scope() == ipv6_addr::Scope_Link)
            {
                rt->addRoute(0, nextHopAddr, defaultAddr, true);
                break;
            }
        }
    }

    // fill in routing tables
    for (int i = 0; i < topo.nodes(); i++)
    {
        cTopology::Node * destNode = topo.node(i);

        // skip bus types
        if (std::find(nonIPTypes.begin(), nonIPTypes.end(), destNode->module()->className()) != nonIPTypes.end())
            continue;

	//        bool isDestHost = !(IPAddressResolver().routingTable6Of(destNode->module())->isRouter());

        // get a list of addresse from the dest node
        std::vector<IPv6Address> destAddrs;
        InterfaceTable *destIft = IPAddressResolver().interfaceTableOf(destNode->module());
        for (unsigned int x = 0; x < destIft->numInterfaces(); x++)
        {
            InterfaceEntry *destIf = destIft->interfaceAt(x);

            if (destIf->isLoopback())
                continue;

            for (unsigned int y = 0; y < destIf->ipv6()->tentativeAddrs.size(); y++)
                destAddrs.push_back(destIf->ipv6()->tentativeAddrs[y]);
        }

        std::string destModName = destNode->module()->fullName();

        // calculate shortest paths from everywhere towards destNode
        topo.unweightedSingleShortestPathsTo(destNode);

        // add route (with host=destNode) to every routing table in the network
        // (excepting nodes with only one interface -- there we'll set up a default route)
        for (int j = 0; j < topo.nodes(); j++)
        {
            if (i == j)
                continue;
            if (std::find(nonIPTypes.begin(), nonIPTypes.end(), topo.node(j)->module()->className()) != nonIPTypes.end())
                continue;

            cTopology::Node * atNode = topo.node(j);
            if (atNode->paths() == 0)
                continue;       // not connected
            if (usesDefaultRoute[j])
                continue;       // already added default route here

            int outputPort = atNode->path(0)->localGate()->index();

            RoutingTable6 *rt = IPAddressResolver().routingTable6Of(atNode->module());

            // determine next hop link address
            cGate *remoteGate = atNode->path(0)->remoteGate();
            cModule *nextHop = remoteGate->ownerModule();
            InterfaceTable *nextHopIft = IPAddressResolver().interfaceTableOf(nextHop);
            InterfaceEntry *nextHopOnlinkIf = nextHopIft->interfaceByPortNo(remoteGate->index());

            // future reference for off-link address
            IPv6Address nextHopLinkLocalAddr;
            for (unsigned int k = 0; k < nextHopOnlinkIf->ipv6()->tentativeAddrs.size(); k++)
            {
                nextHopLinkLocalAddr = nextHopOnlinkIf->ipv6()->tentativeAddrs[k];
                if (nextHopLinkLocalAddr.scope() == ipv6_addr::Scope_Link)
                    break;
            }

            // traverse through address of each node
            for (unsigned int k = 0; k < destAddrs.size(); k++)
            {

              // add to destination cache 
              if ( destAddrs[k].scope() != ipv6_addr::Scope_Link )
                rt->addRoute(outputPort, nextHopLinkLocalAddr, destAddrs[k], true);
            }
        }
    }

    // update display string
    char buf[80];
    sprintf(buf, "%d IPv6 nodes\n%d non-IP nodes", numIPNodes, topo.nodes() - numIPNodes);
    displayString().setTagArg("t", 0, buf);
}

void FlatNetworkConfigurator6::handleMessage(cMessage * msg)
{
    error("this module doesn't handle messages, it runs only in initialize()");
}

