// -*- Ned -*-
// Copyright (C) 2005 Johnny Lai
//
// Do Not Edit, this file is generated by NedFile.rb at Sun Sep 04 01:09:29 EST 2005.
//Edit at your own peril.
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

//
// @file   CampusEHmip.ned


import
    "WorldProcessor",
    "Router6",
    "UDPNode",
    "WorldProcessor",
    "WirelessAccessPoint",
    "WirelessMobileNode";

//Do not modify as it does nothing if used between ethernet interfaces. 
//Look at ethernet.cc for BANDWIDTH constant and alter that.
channel CampusEHmipIntranetCable
    datarate 100e6
    delay 10e-3
endchannel

channel CampusEHmipInternetCable
    datarate 1000e6
    delay 100e-3
endchannel


module CampusEHmip
  submodules:
    worldProcessor: WorldProcessor;
      display: "i=bwgen_s";
    mn1: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=180,720;i=laptop3";
    mn2: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=217,991;i=laptop3";
    mn3: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=192,1006;i=laptop3";
    mn4: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=179,995;i=laptop3";
    mn5: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=153,753;i=laptop3";
    mn6: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=202,495;i=laptop3";
    mn7: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=156,401;i=laptop3";
    mn8: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=256,141;i=laptop3";
    mn9: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=272,374;i=laptop3";
    mn10: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=144,595;i=laptop3";
    mn11: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=234,957;i=laptop3";
    mn12: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=176,598;i=laptop3";
    mn13: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=184,504;i=laptop3";
    mn14: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=263,982;i=laptop3";
    mn15: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=75,444;i=laptop3";
    mn16: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=79,780;i=laptop3";
    mn17: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=64,477;i=laptop3";
    mn18: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=243,1113;i=laptop3";
    mn19: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=231,231;i=laptop3";
    mn20: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=251,1019;i=laptop3";
    mn21: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=275,591;i=laptop3";
    mn22: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=235,944;i=laptop3";
    mn23: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=161,642;i=laptop3";
    mn24: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=231,813;i=laptop3";
    mn25: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=86,858;i=laptop3";
    mn26: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=200,708;i=laptop3";
    mn27: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=91,660;i=laptop3";
    mn28: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=267,899;i=laptop3";
    mn29: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=174,194;i=laptop3";
    mn30: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=151,591;i=laptop3";
    mn31: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=118,281;i=laptop3";
    mn32: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=230,875;i=laptop3";
    mn33: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=160,313;i=laptop3";
    mn34: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=185,226;i=laptop3";
    mn35: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=64,430;i=laptop3";
    mn36: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=195,241;i=laptop3";
    mn37: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=194,320;i=laptop3";
    mn38: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=195,497;i=laptop3";
    mn39: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=267,1054;i=laptop3";
    mn40: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=210,565;i=laptop3";
    mn41: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=139,742;i=laptop3";
    mn42: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=156,1054;i=laptop3";
    mn43: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=213,187;i=laptop3";
    mn44: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=73,1127;i=laptop3";
    mn45: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=206,785;i=laptop3";
    mn46: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=207,264;i=laptop3";
    mn47: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=106,466;i=laptop3";
    mn48: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=88,890;i=laptop3";
    mn49: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=129,736;i=laptop3";
    mn50: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=140,431;i=laptop3";
    cr1: Router6;
        gatesizes:
            in[4],
            out[4];
        display: "i=router";

    ar2: Router6;
        gatesizes:
            in[6],
            out[6];
        display: "i=router";

    ar3: Router6;
        gatesizes:
            in[5],
            out[5];
        display: "i=router";

    ar4: Router6;
        gatesizes:
            in[4],
            out[4];
        display: "i=router";

    ar5: Router6;
        gatesizes:
            in[4],
            out[4];
        display: "i=router";

    ar6: Router6;
        gatesizes:
            in[6],
            out[6];
        display: "i=router";

    ar7: Router6;
        gatesizes:
            in[5],
            out[5];
        display: "i=router";

    ar8: Router6;
        gatesizes:
            in[4],
            out[4];
        display: "i=router";

    ar9: Router6;
        gatesizes:
            in[4],
            out[4];
        display: "i=router";

    cn10: UDPNode;
        gatesizes:
            in[1],
            out[1];
        display: "i=pc";

    ha11: Router6;
        gatesizes:
            in[1],
            out[1];
        display: "i=router";

    ap12: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=150,650";

    ap13: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=50,650";

    ap14: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=250,650";

    ap15: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=100,750";

    ap16: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=100,850";

    ap17: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=100,950";

    ap18: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=200,750";

    ap19: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=200,850";

    ap20: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=200,950";

    ap21: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=100,1050";

    ap22: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=150,1150";

    ap23: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=200,1050";

    ap24: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=150,550";

    ap25: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=50,550";

    ap26: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=250,550";

    ap27: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=100,450";

    ap28: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=100,350";

    ap29: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=100,250";

    ap30: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=200,450";

    ap31: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=200,350";

    ap32: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=200,250";

    ap33: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=100,150";

    ap34: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=150,50";

    ap35: AccessPoint;
        gatesizes:
            in[1],
            out[1];
        display: "i=switch1_s;p=200,150";

 connections nocheck:
    ap15.out[0] --> CampusEHmipIntranetCable --> ar6.in[0];
    ap15.in[0] <-- CampusEHmipIntranetCable <-- ar6.out[0];
    ap13.out[0] --> CampusEHmipIntranetCable --> ar5.in[0];
    ap13.in[0] <-- CampusEHmipIntranetCable <-- ar5.out[0];
    ar5.out[1] --> CampusEHmipIntranetCable --> ap14.in[0];
    ar5.in[1] <-- CampusEHmipIntranetCable <-- ap14.out[0];
    ar5.out[2] --> CampusEHmipIntranetCable --> ar2.in[0];
    ar5.in[2] <-- CampusEHmipIntranetCable <-- ar2.out[0];
    ar5.out[3] --> CampusEHmipIntranetCable --> ap12.in[0];
    ar5.in[3] <-- CampusEHmipIntranetCable <-- ap12.out[0];
    ap18.out[0] --> CampusEHmipIntranetCable --> ar7.in[0];
    ap18.in[0] <-- CampusEHmipIntranetCable <-- ar7.out[0];
    ap16.out[0] --> CampusEHmipIntranetCable --> ar6.in[1];
    ap16.in[0] <-- CampusEHmipIntranetCable <-- ar6.out[1];
    ha11.out[0] --> CampusEHmipInternetCable --> cr1.in[0];
    ha11.in[0] <-- CampusEHmipInternetCable <-- cr1.out[0];
    ap21.out[0] --> CampusEHmipIntranetCable --> ar2.in[1];
    ap21.in[0] <-- CampusEHmipIntranetCable <-- ar2.out[1];
    ap19.out[0] --> CampusEHmipIntranetCable --> ar7.in[1];
    ap19.in[0] <-- CampusEHmipIntranetCable <-- ar7.out[1];
    ar6.out[2] --> CampusEHmipIntranetCable --> ap17.in[0];
    ar6.in[2] <-- CampusEHmipIntranetCable <-- ap17.out[0];
    ar6.out[3] --> CampusEHmipIntranetCable --> cr1.in[1];
    ar6.in[3] <-- CampusEHmipIntranetCable <-- cr1.out[1];
    ar6.out[4] --> CampusEHmipIntranetCable --> ar7.in[2];
    ar6.in[4] <-- CampusEHmipIntranetCable <-- ar7.out[2];
    ar6.out[5] --> CampusEHmipIntranetCable --> ar8.in[0];
    ar6.in[5] <-- CampusEHmipIntranetCable <-- ar8.out[0];
    ap24.out[0] --> CampusEHmipIntranetCable --> ar8.in[1];
    ap24.in[0] <-- CampusEHmipIntranetCable <-- ar8.out[1];
    ap22.out[0] --> CampusEHmipIntranetCable --> ar2.in[2];
    ap22.in[0] <-- CampusEHmipIntranetCable <-- ar2.out[2];
    cr1.out[2] --> CampusEHmipIntranetCable --> ar2.in[3];
    cr1.in[2] <-- CampusEHmipIntranetCable <-- ar2.out[3];
    cr1.out[3] --> CampusEHmipInternetCable --> cn10.in[0];
    cr1.in[3] <-- CampusEHmipInternetCable <-- cn10.out[0];
    ap27.out[0] --> CampusEHmipIntranetCable --> ar3.in[0];
    ap27.in[0] <-- CampusEHmipIntranetCable <-- ar3.out[0];
    ap25.out[0] --> CampusEHmipIntranetCable --> ar8.in[2];
    ap25.in[0] <-- CampusEHmipIntranetCable <-- ar8.out[2];
    ap20.out[0] --> CampusEHmipIntranetCable --> ar7.in[3];
    ap20.in[0] <-- CampusEHmipIntranetCable <-- ar7.out[3];
    ar7.out[4] --> CampusEHmipIntranetCable --> ar9.in[0];
    ar7.in[4] <-- CampusEHmipIntranetCable <-- ar9.out[0];
    ap30.out[0] --> CampusEHmipIntranetCable --> ar9.in[1];
    ap30.in[0] <-- CampusEHmipIntranetCable <-- ar9.out[1];
    ap28.out[0] --> CampusEHmipIntranetCable --> ar3.in[1];
    ap28.in[0] <-- CampusEHmipIntranetCable <-- ar3.out[1];
    ap23.out[0] --> CampusEHmipIntranetCable --> ar2.in[4];
    ap23.in[0] <-- CampusEHmipIntranetCable <-- ar2.out[4];
    ar2.out[5] --> CampusEHmipIntranetCable --> ar3.in[2];
    ar2.in[5] <-- CampusEHmipIntranetCable <-- ar3.out[2];
    ap33.out[0] --> CampusEHmipIntranetCable --> ar4.in[0];
    ap33.in[0] <-- CampusEHmipIntranetCable <-- ar4.out[0];
    ap31.out[0] --> CampusEHmipIntranetCable --> ar9.in[2];
    ap31.in[0] <-- CampusEHmipIntranetCable <-- ar9.out[2];
    ap26.out[0] --> CampusEHmipIntranetCable --> ar8.in[3];
    ap26.in[0] <-- CampusEHmipIntranetCable <-- ar8.out[3];
    ap34.out[0] --> CampusEHmipIntranetCable --> ar4.in[1];
    ap34.in[0] <-- CampusEHmipIntranetCable <-- ar4.out[1];
    ap29.out[0] --> CampusEHmipIntranetCable --> ar3.in[3];
    ap29.in[0] <-- CampusEHmipIntranetCable <-- ar3.out[3];
    ar3.out[4] --> CampusEHmipIntranetCable --> ar4.in[2];
    ar3.in[4] <-- CampusEHmipIntranetCable <-- ar4.out[2];
    ap32.out[0] --> CampusEHmipIntranetCable --> ar9.in[3];
    ap32.in[0] <-- CampusEHmipIntranetCable <-- ar9.out[3];
    ap35.out[0] --> CampusEHmipIntranetCable --> ar4.in[3];
    ap35.in[0] <-- CampusEHmipIntranetCable <-- ar4.out[3];
  display: "p=1,1;b=220,1080";
endmodule

network campusEHmipNet: CampusEHmip
endnetwork
