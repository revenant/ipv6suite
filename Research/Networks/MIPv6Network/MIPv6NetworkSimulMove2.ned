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

module MIPv6NetworkSimulMove2
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=540,88;i=misc/sun";
        mn1: WirelessIPv6TestNode;
            parameters:
                brModelType = "IPv6CBRSrcModel",
                IPForward = false;
            gatesizes:
                wlin[1],
                wlout[1];
            display: "p=110,55;i=laptop3";
        mn2: WirelessIPv6TestNode;
            parameters:
                brModelType = "IPv6CBRSrcModel",
                IPForward = false;
            gatesizes:
                wlin[1],
                wlout[1];
            display: "p=110,421;i=laptop3";
        coreRouter1: Router6;
            gatesizes:
                in[11],
                out[11];
            display: "p=475,220;i=router3";
        coreRouter2: Router6;
            gatesizes:
                in[11],
                out[11];
            display: "p=475,426;i=router3";
        router1: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=95,100;i=router2";
        router2: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=185,386;i=router2";
        router3: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=185,180;i=router2";
        router4: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=185,466;i=router2";
        router5: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=185,260;i=router2";
        router6: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=95,386;i=router2";
        router7: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=185,100;i=router2";
        router8: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=275,386;i=router2";
        router9: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=275,180;i=router2";
        router10: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=275,466;i=router2";
        router11: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=275,260;i=router2";
        router12: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=185,546;i=router2";
        router13: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=275,100;i=router2";
        router14: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=365,386;i=router2";
        router15: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=365,180;i=router2";
        router16: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=365,466;i=router2";
        router17: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=365,260;i=router2";
        router18: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=275,546;i=router2";
        router19: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=365,100;i=router2";
        router20: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=365,546;i=router2";
        ap1: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=150,60;i=switch1_s";
        ap2: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=230,426;i=switch1_s";
        ap3: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=230,140;i=switch1_s";
        ap4: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=230,506;i=switch1_s";
        ap5: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=230,220;i=switch1_s";
        ap6: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=150,426;i=switch1_s";
        ap7: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=230,60;i=switch1_s";
        ap8: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=320,426;i=switch1_s";
        ap9: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=320,140;i=switch1_s";
        ap10: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=320,506;i=switch1_s";
        ap11: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=320,220;i=switch1_s";
        ap12: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=230,586;i=switch1_s";
        ap13: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=320,60;i=switch1_s";
        ap14: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=410,426;i=switch1_s";
        ap15: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=410,140;i=switch1_s";
        ap16: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=410,506;i=switch1_s";
        ap17: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=410,220;i=switch1_s";
        ap18: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=320,586;i=switch1_s";
        ap19: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=410,60;i=switch1_s";
        ap20: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=410,586;i=switch1_s";
        loadGenerator: DynamicIPv6CBRLoader;
            display: "p=612,164;i=block/users";
        configurator6: FlatNetworkConfigurator6;
            parameters:
                moduleTypes = "Router6 MobileNode UDPNode WirelessIPv6TestNode",
                nonIPModuleTypes = "AccessPoint ";
            display: "p=612,92;i=block/cogwheel";
    connections nocheck:
        coreRouter1.out[0] --> internetCable --> coreRouter2.in[0];
        coreRouter1.in[0] <-- internetCable <-- coreRouter2.out[0];

        router1.out[0] --> ap1.in[0];
        router1.in[0] <-- ap1.out[0];

        router1.out[1] --> homeLink --> coreRouter1.in[1];
        router1.in[1] <-- homeLink <-- coreRouter1.out[1];

        router2.out[0] --> ap2.in[0];
        router2.in[0] <-- ap2.out[0];

        router2.out[1] --> intranetCable --> coreRouter2.in[1];
        router2.in[1] <-- intranetCable <-- coreRouter2.out[1];

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

        router6.out[1] --> homeLink --> coreRouter2.in[3];
        router6.in[1] <-- homeLink <-- coreRouter2.out[3];

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

        router11.out[0] --> ap11.in[0];
        router11.in[0] <-- ap11.out[0];

        router11.out[1] --> intranetCable --> coreRouter1.in[6];
        router11.in[1] <-- intranetCable <-- coreRouter1.out[6];

        router12.out[0] --> ap12.in[0];
        router12.in[0] <-- ap12.out[0];

        router12.out[1] --> intranetCable --> coreRouter2.in[6];
        router12.in[1] <-- intranetCable <-- coreRouter2.out[6];

        router13.out[0] --> ap13.in[0];
        router13.in[0] <-- ap13.out[0];

        router13.out[1] --> intranetCable --> coreRouter1.in[7];
        router13.in[1] <-- intranetCable <-- coreRouter1.out[7];

        router14.out[0] --> ap14.in[0];
        router14.in[0] <-- ap14.out[0];

        router14.out[1] --> intranetCable --> coreRouter2.in[7];
        router14.in[1] <-- intranetCable <-- coreRouter2.out[7];

        router15.out[0] --> ap15.in[0];
        router15.in[0] <-- ap15.out[0];

        router15.out[1] --> intranetCable --> coreRouter1.in[8];
        router15.in[1] <-- intranetCable <-- coreRouter1.out[8];

        router16.out[0] --> ap16.in[0];
        router16.in[0] <-- ap16.out[0];

        router16.out[1] --> intranetCable --> coreRouter2.in[8];
        router16.in[1] <-- intranetCable <-- coreRouter2.out[8];

        router17.out[0] --> ap17.in[0];
        router17.in[0] <-- ap17.out[0];

        router17.out[1] --> intranetCable --> coreRouter1.in[9];
        router17.in[1] <-- intranetCable <-- coreRouter1.out[9];

        router18.out[0] --> ap18.in[0];
        router18.in[0] <-- ap18.out[0];

        router18.out[1] --> intranetCable --> coreRouter2.in[9];
        router18.in[1] <-- intranetCable <-- coreRouter2.out[9];

        router19.out[0] --> ap19.in[0];
        router19.in[0] <-- ap19.out[0];

        router19.out[1] --> intranetCable --> coreRouter1.in[10];
        router19.in[1] <-- intranetCable <-- coreRouter1.out[10];

        router20.out[0] --> ap20.in[0];
        router20.in[0] <-- ap20.out[0];

        router20.out[1] --> intranetCable --> coreRouter2.in[10];
        router20.in[1] <-- intranetCable <-- coreRouter2.out[10];
endmodule

network mipv6NetworkSimulMove2 : MIPv6NetworkSimulMove2
endnetwork

