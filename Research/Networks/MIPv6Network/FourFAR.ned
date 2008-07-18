//
// Copyright (C) 2001, 2003 Monash University, Australia
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

import  //
    "Router6",
    "UDPNode",
    "WorldProcessor",
    "WirelessAccessPoint",
    "WirelessMobileNode",
    "DynamicBRLoader",
    "Cables";

module FourFAR
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=436,56;i=misc/sun";
        mn1: WirelessIPv6TestNode;
            parameters:
                brModelType = "IPv6CBRSrcModel",
                IPForward = false;
            gatesizes:
                wlin[1],
                wlout[1];
            display: "p=62,183;i=device/palm_s";
        mn2: WirelessIPv6TestNode;
            parameters:
                brModelType = "IPv6CBRSrcModel",
                IPForward = false;
            gatesizes:
                wlin[1],
                wlout[1];
            display: "p=806,165;i=device/palm_s";
        coreRouter1: Router6;
            gatesizes:
                in[6],
                out[6];
            display: "p=363,292;i=device/router_s";
        coreRouter2: Router6;
            gatesizes:
                in[6],
                out[6];
            display: "p=419,234;i=device/router_s";
        router1: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=47,228;i=device/router_s";
        router2: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=751,170;i=device/router_s";
        router4: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=587,170;i=device/router_s";
        router5: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=155,308;i=device/router_s";
        router6: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=587,250;i=device/router_s";
        router3: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=155,228;i=device/router_s";
        router8: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=677,170;i=device/router_s";
        router9: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=245,308;i=device/router_s";
        router10: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=677,250;i=device/router_s";
        router7: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=245,228;i=device/router_s";
        ap1: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=102,188;i=device/accesspoint_s";
        ap2: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=814,194;i=device/accesspoint_s";
        ap4: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=632,210;i=device/accesspoint_s";
        ap5: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=200,268;i=device/accesspoint_s";
        ap6: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=632,290;i=device/accesspoint_s";
        ap3: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=200,188;i=device/accesspoint_s";
        ap8: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=722,210;i=device/accesspoint_s";
        ap9: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=290,268;i=device/accesspoint_s";
        ap10: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=722,290;i=device/accesspoint_s";
        ap7: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=290,188;i=device/accesspoint_s";
        loadGenerator1: DynamicIPv6CBRLoader;
            display: "p=188,44;i=block/users";
        loadGenerator2: DynamicIPv6CBRLoader;
            display: "p=676,44;i=block/users";
        homeloadGenerator1: DynamicIPv6CBRLoader;
            display: "p=68,380;i=block/users";
        homeloadGenerator2: DynamicIPv6CBRLoader;
            display: "p=780,380;i=block/users";
        configurator6: FlatNetworkConfigurator6;
            parameters:
                moduleTypes = "Router6 MobileNode UDPNode WirelessIPv6TestNode",
                nonIPModuleTypes = "AccessPoint ";
            display: "p=108,68;i=block/cogwheel";
    connections nocheck:
        coreRouter1.out[0] --> internetCable --> coreRouter2.in[0];
        coreRouter1.in[0] <-- internetCable <-- coreRouter2.out[0];

        router1.out[0] --> ap1.in[0];
        router1.in[0] <-- ap1.out[0];

        router1.out[1] --> homeLink --> coreRouter1.in[1];
        router1.in[1] <-- homeLink <-- coreRouter1.out[1];

        router2.out[0] --> ap2.in[0];
        router2.in[0] <-- ap2.out[0];

        router2.out[1] --> homeLink --> coreRouter2.in[1];
        router2.in[1] <-- homeLink <-- coreRouter2.out[1];

        router3.out[0] --> ap3.in[0];
        router3.in[0] <-- ap3.out[0];

        router3.out[1] --> intranetCable --> coreRouter1.in[2];
        router3.in[1] <-- intranetCable <-- coreRouter1.out[2];

        router4.out[0] --> ap4.in[0];
        router4.in[0] <-- ap4.out[0];

        router4.out[1] --> intranetCable --> coreRouter2.in[2];
        router4.in[1] <-- intranetCable <-- coreRouter2.out[2];

        router5.out[0] --> ap5.in[0];
        router5.in[0] <-- ap5.out[0];

        router5.out[1] --> intranetCable --> coreRouter1.in[3];
        router5.in[1] <-- intranetCable <-- coreRouter1.out[3];

        router6.out[0] --> ap6.in[0];
        router6.in[0] <-- ap6.out[0];

        router6.out[1] --> intranetCable --> coreRouter2.in[3];
        router6.in[1] <-- intranetCable <-- coreRouter2.out[3];

        router7.out[0] --> ap7.in[0];
        router7.in[0] <-- ap7.out[0];

        router7.out[1] --> intranetCable --> coreRouter1.in[4];
        router7.in[1] <-- intranetCable <-- coreRouter1.out[4];

        router8.out[0] --> ap8.in[0];
        router8.in[0] <-- ap8.out[0];

        router8.out[1] --> intranetCable --> coreRouter2.in[4];
        router8.in[1] <-- intranetCable <-- coreRouter2.out[4];

        router9.out[0] --> ap9.in[0];
        router9.in[0] <-- ap9.out[0];

        router9.out[1] --> intranetCable --> coreRouter1.in[5];
        router9.in[1] <-- intranetCable <-- coreRouter1.out[5];

        router10.out[0] --> ap10.in[0];
        router10.in[0] <-- ap10.out[0];

        router10.out[1] --> intranetCable --> coreRouter2.in[5];
        router10.in[1] <-- intranetCable <-- coreRouter2.out[5];
endmodule

network fourFAR : FourFAR
endnetwork

