// WirelessExternalSignal.msg
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

enum SignalType
{
  ST_CONNECTION_STATUS = 0;
  ST_STATS = 1;
  ST_MONITOR = 2;
  ST_CHANNEL = 3;
  ST_STATS_REQUEST = 4;
  ST_HANDOVER = 5;
};

enum Status
{
  S_DISCONNECTED = 0;
  S_CONNECTED = 1;
  S_ASSOCIATED = 2;
};

//
// Signalling used for signalling with upper layer signalling
//
message WirelessExternalSignal extends cMessage
{
    fields:
    int type enum(SignalType);
};

message WirelessExternalSignalConnectionStatus extends WirelessExternalSignal
{
  fields:
    int state enum(Status);
    MACAddress6 connectedAddress;
    int connectedChannel;
    double signalStrength;
    double estAvailBW;
    double errorPercentage;
    double avgBackoffTime;
    double avgWaitTime;
};

message WirelessExternalSignalStats extends WirelessExternalSignal
{
  fields:
    double signalStrength;
    double errorPercentage;
    double avgBackoffTime;
    double avgWaitTime;
};

message WirelessExternalSignalChannel extends WirelessExternalSignal
{
  fields:
    int chan;
};

message WirelessExternalSignalHandover extends WirelessExternalSignal
{
  fields:
    MACAddress6 target;
};

