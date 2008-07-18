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


//
// WirelessEthernet control signal (also for simulation wise)
// @author Eric Wu
//
message WESignal
{
  properties:
    omitGetVerb=true;
  fields:
    int channelNum;
    double power;
    string sourceName;
};

message WESignalIdle extends WESignal
{
  fields:
};

message WESignalData extends WESignal
{
  fields:
};


