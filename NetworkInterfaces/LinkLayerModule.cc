// -*- C++ -*-
// Copyright (C) 2001, 2004 Monash University, Melbourne, Australia
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

/*
    @file LinkLayerModule.cc
    @brief Header file for LinkLayerModule.h

    an 'abstract' class for all network interface classes

    @author Eric Wu
*/

#include "LinkLayerModule.h"
#include "opp_utils.h"
#include <sstream>

LinkLayerModule::~LinkLayerModule()
{}

void LinkLayerModule::initialize()
{
  delay = par("procdelay");
  iface_type = 0; // unknown protocol
  cntReceivedPackets = 0;
  new cWatch ( "received pkts", cntReceivedPackets );
}

void LinkLayerModule::setIface_name(int llProtocol)
{
  int ll_index = 0;

  for (int i = 0; i <  parentModule()->index(); i++)
  {
    // label interface name according to the link layer
    cModule* mod = OPP_Global::findModuleByName(this, "linkLayers");
    LinkLayerModule* llmodule = 0;
    mod = mod->parentModule()->submodule("linkLayers", i);

    if (mod)
    {
      llmodule = (LinkLayerModule*)(mod->submodule("networkInterface"));

      if(llmodule->getInterfaceType() == llProtocol)
        ll_index ++;
    }
  }

  std::string ll_name;

  if (llProtocol == PR_PPP)
    ll_name = "ppp";
  else if (llProtocol == PR_ETHERNET)
    ll_name = "eth";
  else if (llProtocol == PR_WETHERNET)
    ll_name = "wlan";

  std::stringstream ss_n;
  ss_n << ll_name << ll_index;

  iface_name = ss_n.str();
}



