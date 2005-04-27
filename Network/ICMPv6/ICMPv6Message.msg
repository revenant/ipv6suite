//
// Copyright (C) 2001 CTIE, Monash University
// Copyright (C) 2005 Andras Varga
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

cplusplus {{
#define ICMPv6_HEADER_OCTETLENGTH  8
}}

enum ICMPv6Type
{
    ICMPv6_UNSPECIFIED = 0;
    ICMPv6_DESTINATION_UNREACHABLE = 1;
    ICMPv6_PACKET_TOO_BIG = 2;
    ICMPv6_TIME_EXCEEDED = 3;
    ICMPv6_PARAMETER_PROBLEM = 4;
    ICMPv6_ECHO_REQUEST = 128;
    ICMPv6_ECHO_REPLY = 129;
    ICMPv6_MLD_QUERY = 130;
    ICMPv6_MLD_REPORT = 131;
    ICMPv6_MLD_DONE = 132;
    ICMPv6_ROUTER_SOL = 133;
    ICMPv6_ROUTER_AD = 134;
    ICMPv6_NEIGHBOUR_SOL = 135;
    ICMPv6_NEIGHBOUR_AD = 136;
    ICMPv6_REDIRECT = 137;
    ICMPv6_MLDv2_REPORT = 143;
};


cplusplus {{
typedef int ICMPv6Code;
}}


//
// ICMPv6 "codes" for type ICMPv6_DESTINATION_UNREACHABLE
//
enum ICMPv6DEST_UN
{
  NO_ROUTE_TO_DEST = 0;
  ADDRESS_UNREACHABLE = 3;
  // 1 -- Communication with dest prohibited (firewall filter)
  // 2 -- Unused
  // 4 -- port unreachable
};


//
// ICMPv6 "codes" for type ICMPv6_TIME_EXCEEDED
//
enum ICMPv6_TIME_EX
{
    ND_HOP_LIMIT_EXCEEDED = 0;
    ND_FRAGMENT_REASSEMBLY_TIME = 1;
};


//
// Represents an ICMPv6 packet.
//
// Notes:
//    1. number of octets excluding the error datagram that is usually appended
//       in optInfo, i.e. the Type|CODE|CHECKSUM|UNUSED/POINTER/MTU/OTHER
//       as defined in RFC2463
//    2. Any ICMP type with MSB set, i.e. >=128 is an Informational ICMP message
//
message ICMPv6Message
{
    properties:
        omitGetVerb = true;
    fields:
        int type enum(ICMPv6Type);
        int code;
        unsigned int optInfo;  // 2nd 32 bits of ICMPv6 header
};

