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



import
    "IPv6Processing",
    "InputQueue";


//
// Represents the network layer (L3).
//
// The module has ports to connect to a higher layer (TCP,UDP) and
// several network interfaces (see LinkLayer and LinkLayer6).
//
// Incoming packets from all network interfaces are fed into a common
// queue (performance modelling) before entering the Network Layer protocols.
//
module NetworkLayer6
    parameters:
        // processing delay times
        icmp_delay : numeric,
        fragmentation_delay : numeric,
        multicast_delay : numeric,
        ipoutput_delay : numeric,
        ipsend_delay : numeric,
        iptunneling_delay : numeric,
        localdeliver_delay : numeric,
        prerouting_delay : numeric,
        routing_delay : numeric,
        inputqueue_delay : numeric,

        // other network parameters
        //# routingFile : string,
        IPForward : bool,
        numOfPorts : numeric;
    gates:
        in: physIn[];
        in: TCPIn;
        in: UDPIn;
        in: pingIn;
        out: physOut[];
        out: TCPOut;
        out: UDPOut;
        out: pingOut;
        out: errorOut;

    submodules:
        proc: IPv6Processing;
            parameters:
                // delay times
                icmp_delay = icmp_delay,
                fragmentation_delay = fragmentation_delay,
                multicast_delay = multicast_delay,
                ipoutput_delay = ipoutput_delay,
                ipsend_delay = ipsend_delay,
                iptunneling_delay = iptunneling_delay,
                localdeliver_delay = localdeliver_delay,
                prerouting_delay = prerouting_delay,
                routing_delay = routing_delay,

                // other parameters
                // IPForward = IPForward,
                numOfPorts = numOfPorts,
                numOfTransportProt = 2,
                multicastTimeToLive = 6,
                fragmentTimeout = "5s";
            gatesizes:
                transportIn[2],
                transportOut[2],
                queueOut[numOfPorts];
            display: "p=148,90;i=cogwheel";
        inputQueue: IPInputQueue;
            parameters:
                procDelay = inputqueue_delay;
            gatesizes:
                fromNW[numOfPorts];
            display: "p=79,157;i=queue";

    connections nocheck:
        // transport Layer
        proc.transportOut[0] --> TCPOut;
        proc.transportIn[0] <-- TCPIn;

        proc.transportOut[1] --> UDPOut;
        proc.transportIn[1] <-- UDPIn;

        // ICMP interface
        proc.ICMPOut --> pingOut;
        proc.ICMPIn <-- pingIn;

        proc.errorOut --> errorOut;

        // L2 input Queue to IP
        inputQueue.toIP --> proc.queueIn;

        // L2 Output Queue from/to IP
        for i=0..numOfPorts-1 do
            inputQueue.fromNW[i] <-- physIn[i];
            proc.queueOut[i] --> physOut[i];
        endfor;

    display: "p=10,10;b=244,213,rect";
endmodule

