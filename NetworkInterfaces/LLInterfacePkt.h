#ifndef __LLINTERFACEPKT_H__
#define __LLINTERFACEPKT_H__


#include <omnetpp.h>
#include <string>
#include "cTypedMessage.h"

struct LLInterfaceInfo
{
  // XXX was IPv6Datagram* dgram, but Link Layer doesn't need it to be IPv6
  cMessage *dgram;
  std::string destLLAddr;
};

typedef cTypedMessage<LLInterfaceInfo> LLInterfacePkt;

#endif //__LLINTERFACEPKT_H__
