//
// Copyright (C) 2005 Eric Wu
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

#ifndef __FLATNETWORKCONFIGURATOR6_H__
#define __FLATNETWORKCONFIGURATOR6_H__

#include <algorithm>
#include <omnetpp.h>

struct ipv6_addr;

/**
 * Configures IP addresses and routing tables for a "flat" network,
 * "flat" meaning that all hosts and routers will have the same
 * network address.
 *
 * For more info please see the NED file.
 */
class FlatNetworkConfigurator6 : public cSimpleModule
{
  public:
    Module_Class_Members(FlatNetworkConfigurator6, cSimpleModule, 0);

  protected:
    virtual int numInitStages() const  {return 4;}
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);

 private:
    std::vector<ipv6_addr> nodeAddresses;
};

#endif

