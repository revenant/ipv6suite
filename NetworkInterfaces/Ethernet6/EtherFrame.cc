// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/NetworkInterfaces/Ethernet6/Attic/EtherFrame.cc,v 1.6 2005/02/16 00:41:32 andras Exp $
//
// Eric Wu
// Copyright (C) 2001 Monash University, Melbourne, Australia
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
    @file PPPFrame .cc
    @brief PPP frame format definition Implementation
    @author Eric Wu
*/

#include <sys.h> // Dout
#include "debug.h" // Dout

#include <omnetpp.h>
#include <cstring>
#include <cstdio>

#include "EtherFrame.h"

// All field length is in bits

const int PREAMBLE = 64;
const int DEST_ADDR_LEN = 48;
const int SRC_ADDR_LEN = 48;
const int TYPE_LEN = 16;
const int CRC = 32;
const int POSTAMBLE = 8;

// constructors
EtherFrame::EtherFrame(const char* name): cMessage(name)
{}

EtherFrame::EtherFrame(const EtherFrame& p)
{
    setName( p.name() );
    operator=(p);
}

// assignment operator
EtherFrame& EtherFrame::operator=(const EtherFrame& p)
{
    cMessage::operator=(p);
    // _protocol = p._protocol;
    _srcAddr = p._srcAddr;
    _destAddr = p._destAddr;
    return *this;
}

// information functions
std::string EtherFrame::info()
{
    std::stringstream out;
    out << "prot= " << _protocol
        << " src=" << _srcAddr.stringValue()
        << " dest=" << _destAddr.stringValue();
    return out.str();
}

void EtherFrame::writeContents(std::ostream& os)
{
    os << "EtherFrame: "
        << "\nProtocol :" << (int)_protocol
        << "\nsrcmac   :" << _srcAddr.stringValue()
        << "\ndestmac  :" << _destAddr.stringValue()
        << "\n";
}

const char *EtherFrame::dumpContents(void)
{
     static char buff[400];
     sprintf(buff, "EtherFrame:\nProtocol :%x\nsrcmac   :%s\n",
              (int)_protocol, srcAddrString());
     // two sprintfs since only one static storage array for stringValue();
     sprintf(buff+strlen(buff), "destmac  :%s\n", destAddrString());
     return (const char *) buff;
}

const char *EtherFrame::destAddrString(void) const
{
/*  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: "
       << " --------------------------------------------- \n"
       << "_destAddr.stringValue()- " << _destAddr.stringValue() << "\n"
       << " --------------------------------------------- \n");
*/
     return _destAddr.stringValue();
}

const char *EtherFrame::srcAddrString(void) const
{
/*  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: "
       << " --------------------------------------------- \n"
       << "_srcAddr.stringValue()- " << _srcAddr.stringValue() << "\n"
       << " --------------------------------------------- \n");
*/
     return _srcAddr.stringValue();
}

#if defined __CN_PAYLOAD_H

#include <netinet/if_ether.h>

struct network_payload *EtherFrame::networkOrder()  const
{
       struct network_payload *packet;
       cPacket *enc;
       struct network_payload *payload = NULL;
       unsigned char data[ETH_FRAME_LEN];

       ev << "EtherFrame::networkOrder()" << endl;
       // set source and dest and zlen
       // start with packet containing no data currently
       // all data to go behind start pointer.
       if(! (packet = new struct network_payload(ETH_FRAME_LEN, 0))){
           return NULL;
       }
       enc = static_cast<cPacket*> (encapsulatedMsg());
      payload = enc->networkOrder();
       pack_mac_addr(this->destAddrString(),
                     (unsigned char *)&((struct ethhdr *)data)->h_dest);
       pack_mac_addr(this->srcAddrString(),
                     (unsigned char *)&((struct ethhdr *)data)->h_source);

       if(enc){
            unsigned short ethproto = 0;
               //((struct ethhdr *)data)->h_proto =
            switch(((cPacket *)enc)->protocol()){
               case PR_IPV6:
                  ev << "enc proto  PR_IPV6"  << endl;
                  ethproto = ETH_P_IPV6;
                  break;
               case PR_IP:
                  ev << "enc proto  PR_IP"  << endl;
                  ethproto = ETH_P_IP;
                  break;
               default:
                  ev << "enc proto  "<<((cPacket *)enc)->protocol() << endl;
                  // probably wrong:
                  ethproto = _protocol;
                  break;
            }
            ((struct ethhdr *)data)->h_proto = htons(ethproto);
       }
       else{
          ((struct ethhdr *)data)->h_proto =
                         htons((unsigned short)_protocol);
       }

       if(payload){
          packet->set_data(data, ETH_HLEN);
          packet->set_payload(payload,((payload->len()+ETH_HLEN)>ETH_DATA_LEN)?
             ETH_DATA_LEN:(payload->len()+ETH_HLEN));

          delete(payload);
       }
       else{
           // set source and dest and zlen
          memset(data + ETH_HLEN, 0, ETH_ZLEN-ETH_HLEN);
          packet->set_data(data, ETH_ZLEN);
       }
       return packet;
}

int EtherFrame::pack_mac_addr(const char *straddr, unsigned char *pack) const
{
    int i, j;
    unsigned char val;
    char ch;

   if(!(straddr && pack )|| (strlen(straddr) != 17))
        return -1;

   for(i = 0; i < 6 ; i ++){
      pack[i] = 0;
      for( j = 0;  j < 2; j++){
         ch = straddr[ (3 * i )+j];
         switch(ch){
             case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                   val = (ch - 'a' )  + 0x0a;
                   break;
             case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                   val = (ch - 'A' )  + 0x0a;
                   break;
             case '0': case '1': case '2': case '3': case '4':
             case '5': case '6': case '7': case '8': case '9':
                   val = (ch - '0' );
                   break;
             default:
               return -1;
                   break;
         }
         if (j){
             pack[i] <<= 4;
         }
         pack[i] |= val;
      }
   }
  return 0;
}
#endif //defined __CN_PAYLOAD_H

/* encapsulate a packet of type cPacket of the Network Layer;
    protocol set by default to IP;
    assumes that networkPacket->length() is
    length of transport packet in bits
    adds to it the Ethernet header length in bits */
/* XXX seems like this is not strictly necessary --AV
void EtherFrame::encapsulate(cPacket* networkPacket)
{
    cPacket::encapsulate(networkPacket);
    setName(networkPacket->name());
}
*/
int EtherFrame::headerByteLength() const
{
  int headerlen = (PREAMBLE +
                   DEST_ADDR_LEN +
                   SRC_ADDR_LEN +
                   TYPE_LEN +
                   CRC +
                   POSTAMBLE) / 8;
  return headerlen;
}

/* XXX no need -- removed to reduce clutter.  --AV
cPacket* EtherFrame::decapsulate()
{
  return (cPacket *)(cPacket::decapsulate());
}
*/

void EtherFrame::setProtocol(int prot)
{
  _protocol = prot;
}

void EtherFrame::setSrcAddress( const  MACAddress6& src)
{
  _srcAddr = src;
/*
  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: "
       << " --------------------------------------------- \n"
       << "_srcAddr - " << _srcAddr.stringValue() << "\n"
       << "src - " << src.stringValue() << "\n"
       << " --------------------------------------------- \n");*/
}

void EtherFrame::setDestAddress( const  MACAddress6& dest)
{
   _destAddr = dest;
/*
  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: "
       << " --------------------------------------------- \n"
       << "_destAddr - " << _destAddr.stringValue() << "\n"
       << "dest - " << dest.stringValue() << "\n"
       << " --------------------------------------------- \n");*/
}

void EtherFrame::setDataLength(int dataLen)
{
  setLength(dataLen + headerByteLength());
}

int EtherFrame::protocol() const
{
  return _protocol;
}

const MACAddress6& EtherFrame::srcAddress(void)  const
{
  return _srcAddr;
}

const MACAddress6& EtherFrame::destAddress(void) const
{
  return _destAddr;
}
/*
const int EtherFrame::packetLength(void)
{
  return headerByteLength() + _dataLen;
}
*/
