#if !defined __MESSAGE_H__
#define __MESSAGE_H__

#ifndef STRING
#include <string>
#define STRING
#endif //STRING
#ifndef IPv6_ADDR_H
#include "ipv6_addr.h"
#endif //IPv6_ADDR_H
#ifndef CTYPEDMESSAGE_H
#include "cTypedMessage.h"
#endif //CTYPEDMESSAGE_H

class IPv6Datagram;
class IPDatagram;

struct AddrResInfo
{
  IPv6Datagram* dgram;
  ipv6_addr nextHop;
  unsigned int ifIndex;
  string linkLayerAddr;
};

struct LLInterfaceInfo
{
  IPv6Datagram* dgram;
  string destLLAddr;
};

struct GenericUDPInfo
{
  string data;
  simtime_t timeStamp;
};

struct VideoStreamInfo : public GenericUDPInfo
{
  simtime_t expireTime;
};

typedef cTypedMessage<LLInterfaceInfo> LLInterfacePkt;
typedef cTypedMessage<AddrResInfo> AddrResMsg;

#endif //__MESSAGE_H__
