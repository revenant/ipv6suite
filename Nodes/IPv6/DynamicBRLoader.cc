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

const double SERVCE_INITIATION = 5.0;
const double SERVCE_INITIATION_COMPLETE = 5.0;
const int CREATE_RATE = 1;

Define_Module(DynamicBRLoader);

void DynamicBRLoader::initialize(int stage)
{
  numNodes = par("numNodes");
  if ( numNodes == 0 )
    return;

  minX = par("rangeMinX");
  minY = par("rangeMinY");
  maxX = par("rangeMaxX");
  maxY = par("rangeMaxY");
  srcPrefix = par("srcPrefix").stringValue();
  destPrefix = par("destPrefix").stringValue();  

  // random number generations to make sure same values are generated
  // for different source code compliations
  for (int i = 0; i < numNodes; i++)
  {     
    Position pos;
    pos.x = intuniform(minX, maxX);
    pos.y = intuniform(minY, maxY);
    positions.push_back(pos);

    // Service starts some time after the module has been created to
    // allow the node is connected to the network (e.g. IEEE 802.11)
    simtime_t rndBegin = SERVCE_INITIATION + numNodes * CREATE_RATE;
    simtime_t rndEnd = rndBegin + SERVCE_INITIATION_COMPLETE;
    simtime_t tStart = uniform( rndBegin, rndEnd );
    startTimes.push_back(tStart);
  }

  index = 0;
  parameterMessage = new cMessage;  

  // start operating one node/sec to avoid heavy congestions due to
  // IEEE 802.11 active scanning by many nodes in one time
  scheduleAt(index, parameterMessage);
}

///////////

Define_Module(DynamicIPv6CBRLoader);

void DynamicIPv6CBRLoader::handleMessage(cMessage* msg)
{
  std::stringstream src, dest;
  src << srcPrefix << index;
  dest << destPrefix << index;

  cModule* module = createModule(src.str(), positions[index]);  
  module->submodule("brSrcModel")->par("destAddr") = dest.str().c_str();
  module->submodule("brSrcModel")->par("tStart") = startTimes[index];
  module->scheduleStart(simTime());
  module->callInitialize();

  index++;
  
  if ( index < numNodes )
  {
    scheduleAt(index*CREATE_RATE, msg);
    return;
  }

  // all nodes are created, clear temporary use of memory
  positions.clear();
  startTimes.clear();
  delete msg;
}

cModule* DynamicIPv6CBRLoader::createModule(std::string src, Position pos)
{
  cModuleType *moduleType = findModuleType("WirelessIPv6TestNode");

  std::stringstream posX, posY;
  cModule* node = moduleType->create(src.c_str(),parentModule());

  posX << pos.x;
  posY << pos.y; 

  cDisplayString nodedisp;
  nodedisp.setTagArg("p", 0, posX.str().c_str());
  nodedisp.setTagArg("p", 1, posY.str().c_str());
  nodedisp.setTagArg("i", 0, "old/ball2_s");
  node->setDisplayString(nodedisp.getString());
  node->par("brModelType") = "IPv6CBRSrcModel";
  node->setGateSize("wlin", 1);
  node->setGateSize("wlout", 1);

  // create submodules contained in the node
  node->buildInside();

  // parse parameters into CBR source model
  node->submodule("brSrcModel")->par("msgType") = par("msgType");
  node->submodule("brSrcModel")->par("bitRate") = par("bitRate");
  node->submodule("brSrcModel")->par("fragmentLen") =par("fragmentLen");

  return node;
}
