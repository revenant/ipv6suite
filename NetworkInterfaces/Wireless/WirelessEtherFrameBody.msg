// WirelessEtherFramebody.msg
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
class MACAddress6;

//
// Capability Information field format
//
struct CapabilityInformation
{
    fields:
        bool ESS;
        bool IBSS;
        bool CFPollable;
        bool CFPollRequest;
        bool privacy;
};

//
// Additional parameters for handover decisions
// (Not part of standard, for experimental purposes)
//
struct HandoverParameters
{
    fields:
        double avgBackoffTime;
        double avgWaitTime;
        double avgErrorRate;
        double estAvailBW;
};

enum ReasonCode
{
  RC_UNSPECIFIED = 1;
  RC_PREV_AUTH_EXPIRED = 2;
  RC_DEAUTH_MS_LEAVING = 3;
  RC_DISASS_INACTIVITY = 4;
  RC_DISASS_TOO_MANY_MS = 5;
  RC_NONAUTH_CLASS2 = 6;
  RC_NONASS_CLASS3 = 7;
  RC_DIASS_MS_LEAVING = 8;
  RC_NONAUTH_ASS_REQUEST = 9;
};

enum StatusCode
{
  SC_SUCCESSFUL = 0;
  SC_UNSPECIFIED = 1;
  SC_UNSUP_CAP = 2;
  SC_RESASS_DENIED = 3;
  SC_ASS_DENIED_UNKNOWN = 4;
  SC_AUTH_ALG0_UNSUP = 5;
  SC_AUTH_OUT_OF_SEQ = 6;
  SC_CHALLANGE_FAILURE = 7;
  SC_AUTH_TIMEOUT = 8;
  SC_ASS_TOO_MANY_MS = 9;
  SC_DATARATE_SUP = 10;
};

//
// Structure to store a single supported rate information
//
struct SupportedRatesElement
{
    fields:
        bool supported; //true=supported, false=unsupported
        double rate;     //in Mbit/sec
};

//
// Frame body base class used to hide various frame body types
//
message FrameBody extends cPacket
{
    fields:
        unsigned short length;
};

//
// Authentication frame body format
//
message AuthenticationFrameBody extends FrameBody
{
    fields:
        unsigned short sequenceNumber;
        int statusCode enum(StatusCode);
};

//
// De-authentication frame body format
//
message DeAuthenticationFrameBody extends FrameBody
{
    fields:
        int reasonCode enum(ReasonCode);
};

//
// Dis-association frame body format (same as De-authentication)
//
message DisAssociationFrameBody extends DeAuthenticationFrameBody
{
    fields:
};

//
// Probe request frame body format
//
message ProbeRequestFrameBody extends FrameBody
{
    fields:
        string SSID;
        SupportedRatesElement supportedRates[];
};

//
// Association request frame body format
//
message AssociationRequestFrameBody extends ProbeRequestFrameBody
{
    fields:
        CapabilityInformation capabilityInformation;
};

//
// Re-association request frame body format (same as association)
//
message ReAssociationRequestFrameBody extends AssociationRequestFrameBody
{
    fields:
        MACAddress6 currentAP;
};

//
// Association response frame body format
//
message AssociationResponseFrameBody extends AuthenticationFrameBody
{
    fields:
        CapabilityInformation capabilityInformation;
        SupportedRatesElement supportedRates[];
};

//
// Re-association response frame body format
//
message ReAssociationResponseFrameBody extends AssociationResponseFrameBody
{
    fields:
};

//
// Beacon frame body format
//
message BeaconFrameBody extends AssociationRequestFrameBody
{
    fields:
        double timestamp;
        unsigned short beaconInterval;
        unsigned short DSChannel;
        HandoverParameters handoverParameters;
};

//
// Probe response frame body format (same as Beacon)
//
message ProbeResponseFrameBody extends BeaconFrameBody
{
    fields:
};
