//
// Copyright (C) 2003, 2004 CTIE, Monash University
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


//
// @page ipv6-index.html
//
// <center><h1>About IPv6Suite</h1></center>
//
// <a href="http://ctieware.eng.monash.edu.au/twiki/bin/view/Simulation/IPv6Suite">IPv6Suite</a> is an OMNeT++ model suite for accurate simulation of IPv6
// protocols and networks. The list of implemented standards is 
// <a href="http://ctieware.eng.monash.edu.au/twiki/bin/view/Simulation/IPv6Suite">here</a>. 
//
// It is a substantial undertaking. We have written 21,521 logical lines 
// of C++ code so far.
// If you find it useful, and publish your work that uses the IPv6Suite, 
// please drop an email to Ahmet Sekercioglu: asekerci@ieee.org. 
// If you make improvements and/or additional models, please let us know also. 
//
// A few useful starting points for browsing the documentation:
//  - WirelessEtherNetwork or WirelessEtherNetwork2 is a good starting point
//    if you are looking for modelling wireless (802.11) networks
//
// Overview on several topics are also available:
//   - list of <a href="http://ctieware.eng.monash.edu.au/twiki/bin/view/Simulation/IPv6Suite">standards</a> implemented 
//   - <a href="config.html">configuring IPv6 models</a>
//   - <a href="mobility.html">mobility support</a>
//
// Related documentation:
//   - <a href="../IPv6Suite-doc/index.html" target="_top">C++ source documentation (Doxygen)</a>
//   - <a href="../xsddoc/index.html" target="_top">XML Network configuration schema</a>
//   - <a href="http://ctieware.eng.monash.edu.au/twiki/bin/view/Simulation/Documentation" target="_top">Documentation portal</a>
//   - <a href="http://ctieware.eng.monash.edu.au/twiki/bin/view/Simulation/Installation" target="_top">Installation</a>
//
//#------------------------------------------------------------------------
// @page standards.html, Implemented Standards
//
// IPv6Suite models the functionality of the following RFCs: 
//  - RFC 2373 IP Version 6 Addressing Architecture 
//  - RFC 2460 Internet Protocol, Version 6 (IPv6) Specification} 
//  - RFC 2461 Neighbor Discovery for IP Version 6 (IPv6) 
//  - RFC 2462 IPv6 Stateless Address Autoconfiguration 
//  - RFC 2463 Internet Control Message Protocol (ICMPv6) for the Internet 
//    Protocol Version 6 (IPv6) Specification 
//  - RFC 2472 IP Version 6 over PPP 
//  - RFC 2473 Generic Packet Tunneling in IPv6 
//  - RFC 2464 Transmission of IPv6 Packets over Ethernet Networks
//  - Mobility Support in IPv6 (MIPv6) revision 18
//  - Hierarchical Mobile IPv6 Mobility Management (HMIPv6) revision 6
//  - Optimistic Duplicate Address Detection revision 4
//  - Fast Solicited Router Advertisements revision 4
// 
// The wireless link/physical layer based on the IEEE 802.11b.
//#------------------------------------------------------------------------
// @page config.html, Configuring IPv6 models
//
// <b>XML configuration file</b>
// 
// A large part of the IPv6 model configuration is in the configuration
// XML file. The name of the XML file is specified as an argument to
// WorldProcessor. 
// 
// Important: The WorldProcessor has to initialize first among all
// modules, so it has to be the first submodule in the top-level module
// (the one that contains the network topology). 
//
// The XML file contains the following configuration:
//  - interfaces (IPv6 address, site prefixes)
//  - tunneling
//  - source routes
//  - Mobile IPv6 configuration: home agent, etc.
//  - mobile node movement
//
// Click here for <a href="../xsddoc/index.html" target="_top">detailed documentation</a>,
// generated from the XML schema (XSD) that defines the structure of the XML file.
//
// <b>omnetpp.ini</b>
//
// Several other parameters are taken from the <tt>omnetpp.ini</tt> file, such as:
//  - most parameters for the IPv4 models (IPSuite) 
//  - application parameters
//  - network interface types (i.e. IPv6PPPInterface, EtherModule)
//  - etc.
//
// Most examples in the default IPv6Suite include <tt>default.ini</tt> which
// contains default settings.
//
//#------------------------------------------------------------------------
// @page mobility.html, Mobility Support
//
// Mobility support in the IPv6Suite covers the following areas:
//  - movement of mobile nodes (trajectories); see <a href="config.html">configuring 
//    the model</a>, XML configuration file
//  - MIPv6 (Mobile IPv6) and HMIPv6 (Hierarchical Mobile IPv6) support -- 
//    these models are incomplete and currently under development
//  - Wireless LAN (802.11) models; see WirelessEtherModule, WirelessEtherBridge,
//    WirelessAccessPoint modules
//
