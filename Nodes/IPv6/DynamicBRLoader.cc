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


#include "DynamicBRLoader.h"
#include <iostream>
#include <sstream>
#include <string>

Define_Module(DynamicBRLoader);

void DynamicBRLoader::initialize(int stage)
{
  numNodes = par("numNodes");
}

///////////

Define_Module(DynamicIPv6CBRLoader);

void DynamicIPv6CBRLoader::initialize(int stage)
{
  DynamicBRLoader::initialize(stage);

  for (int i = 0; i < numNodes; i++)
  {
    std::stringstream src, dest;
    src << "node_" << i << "A";
    dest << "node_" << i << "B";

    // create communicating peers
    createModule(src.str(), dest.str());
    createModule(dest.str(), src.str());    
  }
}

void DynamicIPv6CBRLoader::createModule(std::string src, std::string dest)
{
  cModuleType *moduleType = findModuleType("WirelessIPv6TestNode");

  std::stringstream posX, posY;
  cModule* peer = moduleType->create(src.c_str(), parentModule());

  // TODO: pretty bad.. these positions are only for MIPv6NetworkSimulMove2.xml only
  posX << intuniform(190, 440);
  if ( src.find("A") != std::string::npos )
    posY << intuniform(40, 240);  
  else if ( src.find("B") != std::string::npos )
    posY << intuniform(390, 610); 

  cDisplayString peerdisp;
  peerdisp.setTagArg("p", 0, posX.str().c_str());
  peerdisp.setTagArg("p", 1, posY.str().c_str());
  peerdisp.setTagArg("i", 0, "old/ball2_s");

  peer->setDisplayString(peerdisp.getString());
  peer->par("brModelType") = "IPv6CBRSrcModel";
  peer->setGateSize("wlin", 1);
  peer->setGateSize("wlout", 1);

  peer->buildInside();
  peer->submodule("brSrcModel")->par("msgType") = par("msgType");
  peer->submodule("brSrcModel")->par("tStart") = par("tStart");
  peer->submodule("brSrcModel")->par("bitRate") = par("bitRate");
  peer->submodule("brSrcModel")->par("fragmentLen") =par("fragmentLen");
  peer->submodule("brSrcModel")->par("destAddr") = dest.c_str();
}
