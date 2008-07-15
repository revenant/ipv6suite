//
// Copyright (C) 2005 Andras Varga
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

#ifndef __INTERFACEENTRY_H
#define __INTERFACEENTRY_H

#include <vector>
#include <omnetpp.h>
#include "INETDefs.h"

// Forward declarations. Do NOT #include the corresponding header files
// since that would create dependence on IPv4 and IPv6 stuff!
class IPv4InterfaceData;
class IPv6InterfaceData;

/**
 * An "interface token" as defined in RFC 1971 (IPv6 Stateless Autoconfiguration).
 * This class supports tokens of length 1..64-bits. An interface token needs
 * to be provided by L2 modules in order to be able to form IPv6 link local
 * addresses.
 */
class InterfaceToken
{
  private:
    uint32 _normal, _low;
    short _len; // in bits, 1..64
  public:
    InterfaceToken()  {_normal=_low=_len=0;}
    InterfaceToken(uint32 low, uint32 normal, int len)  {_normal=normal; _low=low; _len=len;}
    InterfaceToken(const InterfaceToken& t)  {operator=(t);}
    void operator=(const InterfaceToken& t)  {_normal=t._normal; _low=t._low; _len=t._len;}
    int length() const {return _len;}
    uint32 low() const {return _low;}
    uint32 normal() const {return _normal;}
};

/**
 * Interface entry for the interface table in InterfaceTable.
 *
 * @see InterfaceTable
 */
class InterfaceEntry : public cPolymorphic
{
  private:
    std::string _name;     //< interface name (must be unique)
    int _outputPort;       //< output gate index (-1 if unused, e.g. loopback interface)
    int _mtu;              //< Maximum Transmission Unit (e.g. 1500 on Ethernet)
    bool _down;            //< current state (up or down)
    bool _broadcast;       //< interface supports broadcast
    bool _multicast;       //< interface supports multicast
    bool _pointToPoint;    //< interface is point-to-point link
    bool _loopback;        //< interface is loopback interface
    double _datarate;      //< data rate in bit/s
    std::string _llAddrStr;//< link layer address (MAC address) as string
    InterfaceToken _token; //< for IPv6 stateless autoconfig (RFC 1971)

    IPv4InterfaceData *_ipv4data;   //< IPv4-specific interface info (IP address, etc)
    IPv6InterfaceData *_ipv6data;   //< IPv6-specific interface info (IPv6 addresses, etc)
    cPolymorphic *_protocol3data;   //< extension point: data for a 3rd network-layer protocol
    cPolymorphic *_protocol4data;   //< extension point: data for a 4th network-layer protocol

  private:
    // copying not supported: following are private and also left undefined
    InterfaceEntry(const InterfaceEntry& obj);
    InterfaceEntry& operator=(const InterfaceEntry& obj);

  public:
    InterfaceEntry();
    virtual ~InterfaceEntry() {}
    virtual std::string info() const;
    virtual std::string detailedInfo() const;

    const char *name() const       {return _name.c_str();}
    int outputPort() const         {return _outputPort;}
    int mtu() const                {return _mtu;}
    bool isDown() const            {return _down;}
    bool isBroadcast() const       {return _broadcast;}
    bool isMulticast() const       {return _multicast;}
    bool isPointToPoint() const    {return _pointToPoint;}
    bool isLoopback() const        {return _loopback;}
    double datarate() const        {return _datarate;}
    const char *llAddrStr() const  {return _llAddrStr.c_str();}
    const InterfaceToken& interfaceToken() const {return _token;}

    cModule *_linkMod;  // XXX FIXME should be eliminated!!!!

    void setName(const char *s)  {_name = s;}
    void setOutputPort(int i)    {_outputPort = i;}
    void setMtu(int m)           {_mtu = m;}
    void setDown(bool b)         {_down = b;}
    void setBroadcast(bool b)    {_broadcast = b;}
    void setMulticast(bool b)    {_multicast = b;}
    void setPointToPoint(bool b) {_pointToPoint = b;}
    void setLoopback(bool b)     {_loopback = b;}
    void setDatarate(double d)   {_datarate = d;}
    void setLLAddrStr(const char *s) {_llAddrStr = s;}
    void setInterfaceToken(const InterfaceToken& token) {_token=token;}

    IPv4InterfaceData *ipv4()    {return _ipv4data;}
    IPv6InterfaceData *ipv6()    {return _ipv6data;}
    cPolymorphic *protocol3()    {return _protocol3data;}
    cPolymorphic *protocol4()    {return _protocol4data;}

    void setIPv4Data(IPv4InterfaceData *p)  {_ipv4data = p;}
    void setIPv6Data(IPv6InterfaceData *p)  {_ipv6data = p;}
    void setProtocol3Data(cPolymorphic *p)  {_protocol3data = p;}
    void setProtocol4Data(cPolymorphic *p)  {_protocol4data = p;}
};

#endif

