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
  minX = par("rangeMinX");
  minY = par("rangeMinY");
  maxX = par("rangeMaxX");
  maxY = par("rangeMaxY");
  srcPrefix = par("srcPrefix").stringValue();
  destPrefix = par("destPrefix").stringValue();  

  for (int i = 0; i < numNodes; i++)
  {     
    Position pos;
    pos.x = intuniform(minX, maxX);
    pos.y = intuniform(minY, maxY);
    positions.push_back(pos);
  }
}

///////////

Define_Module(DynamicIPv6CBRLoader);

void DynamicIPv6CBRLoader::initialize(int stage)
{
  DynamicBRLoader::initialize(stage);

  if (stage == 0)
  {
    for (int i = 0; i < numNodes; i++)
    {      
      std::stringstream src, dest;
      src << srcPrefix << i;
      createModule(src.str(), positions[i]);
    }
  }
  else if (stage == 2)
  {
    for (int i = 0; i < numNodes; i++)
    {      
      std::stringstream src, dest;
      src << srcPrefix << i;
      dest << destPrefix << i;

      simulation.systemModule()->submodule(src.str().c_str())->
        submodule("brSrcModel")->par("destAddr") = dest.str().c_str();
    }
  }
}

void DynamicIPv6CBRLoader::createModule(std::string src, Position pos)
{
  cModuleType *moduleType = findModuleType("WirelessIPv6TestNode");

  std::stringstream posX, posY;
  cModule* peer = moduleType->create(src.c_str(), parentModule());

  posX << pos.x;
  posY << pos.y; 

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
}
