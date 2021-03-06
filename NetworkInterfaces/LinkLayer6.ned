//
// Copyright (C) 2001 Monash University, Australia
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


import
    "NetworkInterface6";



//
// Network interface, with the actual L2 protocol left as parameter.
//
// Outgoing packets are queued before entering the Link Layer protocol
// (performance modelling).
//
// The Link Layer protocol should be selected with a string parameter
// (it is instantiated via "<tt>like</tt>"). Possible values are
// IPv6PPPInterface, EtherModule, WirelessEtherModule, WirelessAccessPoint.
// (These modules don't have NED declarations, only the C++ implementations.)
//
module LinkLayer6
    parameters:
        // processing delay
        nwi_delay : numeric const,

        // Link Layer network name
        NWIName : string;
    gates:
        in: physIn; // from physical layer
        out: physOut; // to physical layer
        in: netwIn; // from upper layer (L3)
        out: netwOut; // to upper layer (L3)
        in: extSignalIn[];
        out: extSignalOut[];

    submodules:
        networkInterface: NWIName like NetworkInterface6;
            parameters:

            gatesizes:
                extSignalIn[sizeof(extSignalIn)],
                extSignalOut[sizeof(extSignalOut)];
            display: "p=87,125;i=bwxconn";
    connections:

        // IP layer to Network interface
        netwIn --> networkInterface.netwIn;
        netwOut <-- networkInterface.netwOut;

        // physical connection
        networkInterface.physOut --> physOut;
        networkInterface.physIn <-- physIn;

        for i=0..sizeof(extSignalIn)-1 do
          networkInterface.extSignalIn[i] <-- extSignalIn[i];
        endfor;

        for i=0..sizeof(extSignalOut)-1 do
          networkInterface.extSignalOut[i] --> extSignalOut[i];
        endfor;

endmodule

