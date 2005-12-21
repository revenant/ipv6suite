//
// Copyright 2004 Monash University, Australia
//
// This library is free software, you can redistribute it and/or modify
// it under  the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation;
// either version 2 of the License, or any later version.
// The library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//

#include "IPv6CBRSrcModel.h"
#include "BRMsg_m.h"
#include "IPv6ControlInfo_m.h"
#include "IPvXAddress.h"
#include "IPProtocolId_m.h"

#include "IPAddressResolver.h"

Define_Module(IPv6CBRSrcModel);

void IPv6CBRSrcModel::initialize()
{
  CBRSrcModel::initialize();
}

void IPv6CBRSrcModel::handleMessage(cMessage* msg)
{
  CBRSrcModel::handleMessage(msg);
}

cMessage* IPv6CBRSrcModel::createControlInfo(BRMsg*)
{
  BRMsg* pkt;

  IPv6ControlInfo *ctrlInfo = new IPv6ControlInfo;
  ctrlInfo->setProtocol(IP_PROT_UDP);
  ctrlInfo->setTimeToLive(64);



  IPvXAddress destAddr;
  if (destAddr.tryParse(par("destAddr")))
    destAddr.set(par("destAddr"));
  else
    destAddr = IPAddressResolver().resolve(par("destAddr"));
  
  ASSERT(!destAddr.isNull());
  ctrlInfo->setDestAddr(destAddr.get6());

  return ctrlInfo;
}
