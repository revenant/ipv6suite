//
// Copyright (C) 2005 Monash University
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
// IEEE 802.11 implementation, to be used in wireless Access Points.
//
// NED interface conforms to NetworkInterface6.
//
simple WirelessAccessPoint
    parameters:
        procdelay : numeric const,
        ssid: string,
        pathLossExponent: numeric const,
        pathLossStdDev: numeric const, // dB; standard deviation of Gauss. dist. for path loss model
        txPower: numeric const,  // mW
        thresholdPower: numeric const, // dBm; threshold power
        hoThresholdPower: numeric const, // dBm; handover threshold power
        probeEnergyTimeout: numeric const,
        probeResponseTimeout: numeric const,
        authenticationTimeout: numeric const,
        associationTimeout: numeric const,
        retry: numeric const,
        fastActiveScan: bool,
        scanShortCircuit: bool,
        crossTalk: bool,
        shadowing: bool,
        channelsNotToScan: string,
        signalStrengthMaxSample: numeric const,
        address: string,
	dataRate: numeric,
        bandwidthRequirements: numeric const,
        recordStatisticVector: bool,
        activeScan: bool,
	noAuth: bool,
        channelScanTime: numeric,
	bufferSize: numeric,
	registerInterface: bool,
	queueType: string,
	queueSize: numeric const,
	errorRate: numeric const;
    gates:
        in: physIn, netwIn, extSignalIn[];
        out: physOut, netwOut, extSignalOut[];
endsimple

