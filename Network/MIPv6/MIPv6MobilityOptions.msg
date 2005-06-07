//
// Copyright (C) 2005 CTIE, Monash University
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

enum MobilityOptType
{
  MOPT_Pad1 = 0; // pad1 (NOT IMPLEMENTED YET!)
  MOPT_PadN = 1; // padN (NOT IMPLEMENTED YET!)
  MOPT_UI = 2; // unique identifier
  MOPT_ACoA = 3; // alternate care-of address
  MOPT_NI = 4; // nonce indices
  MOPT_Auth = 5; // authentication data
  MOPT_HandoverDelay = 6;
};

//
// Definitions of mobility options for Mobile IPv6 mobility messages 
// such BU, BACK, BR, etc.
//
message MobilityOptionBase
{
    properties:
        omitGetVerb = true;

    fields:
        int optType enum (MobilityOptType);
        int optLength;
};

message HandoverDelay extends MobilityOptionBase
{
    fields:
        double delay;
};


