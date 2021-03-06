// WirelessEtherFrame.msg
//
// Eric Wu and Steve Woon
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

//
// Required for MACAddress6 declarations
//
cplusplus
{{
   #include "MACAddress6.h"
}};
class noncobject MACAddress6;

//
// Enumerations of different frame types
//
enum FrameType
{
  FT_MANAGEMENT = 0;
  FT_CONTROL = 1;
  FT_DATA = 2;
  FT_RESERVED = 3;
};

enum SubType
{
  ST_ASSOCIATIONREQUEST = 4;
  ST_ASSOCIATIONRESPONSE = 5;
  ST_REASSOCIATIONREQUEST = 6;
  ST_REASSOCIATIONRESPONSE = 7;
  ST_PROBEREQUEST = 8;
  ST_PROBERESPONSE = 9;
  ST_BEACON = 10;
  ST_DISASSOCIATION = 11;
  ST_AUTHENTICATION = 12;
  ST_DEAUTHENTICATION = 13;
  ST_DATA = 14;
  ST_RTS = 15;
  ST_CTS = 16;
  ST_ACK = 17;
};

//
// Frame control format
//
struct FrameControl
{
    fields:
    unsigned short protocolVer;  //2 bits
    int type enum(FrameType);    //2 bits
        unsigned short subtype;  //4 bits
        bool toDS;               //1 bit
        bool fromDS;             //1 bit
        //bool moreFrag;
        bool retry;              //1 bit
};

//
// Duration/ID format
//
struct DurationID
{
    fields:
        bool bit15;
        bool bit14;
        unsigned short bit14to0;
};

//
// Sequence control format
//
struct SequenceControl
{
    fields:
        unsigned short fragmentNumber;
        unsigned short sequenceNumber;
};

//
// Format of smallest Wireless Ethernet frame (ACK and CTS)
//
message WirelessEtherBasicFrame
{
    fields:
        FrameControl frameControl;
        DurationID durationID;
        MACAddress6 address1;
	// SWOON HACK: To find achievable throughput
	int appType;
	double probTxInSlot;
	int avgFrameSize;
};

//
// Format of Wireless Ethernet RTS frame
//
message WirelessEtherRTSFrame extends WirelessEtherBasicFrame
{
    fields:
        MACAddress6 address2;
};

//
// Format of Wireless Ethernet management frame
//
message WirelessEtherManagementFrame extends WirelessEtherRTSFrame
{
    fields:
        MACAddress6 address3;
        SequenceControl sequenceControl;
};

//
// Format of Wireless Ethernet data frame
//
message WirelessEtherDataFrame extends WirelessEtherManagementFrame
{
    fields:
        MACAddress6 address4;
};
