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


//XXX enum EtherSignalType
//{
//  EST_None = 0;
//  EST_Data = 1;
//  EST_Idle = 2;
//  EST_Jam = 3;
//  EST_JamEnd = 4;

//  WIRELESS_EST_None = 5;
//  WIRELESS_EST_Idle = 6;
//  WIRELESS_EST_Data = 7;
//};


//
// Ethernet control signal (also for simulation wise)
// @author Eric Wu
//
message EtherSignal
{
  fields:
    string srcModPathName;
};

message EtherSignalData extends EtherSignal
{
  fields:
};

message EtherSignalJam extends EtherSignal
{
  fields:
};

message EtherSignalJamEnd extends EtherSignal
{
  fields:
};

message EtherSignalIdle extends EtherSignal
{
  fields:
};


