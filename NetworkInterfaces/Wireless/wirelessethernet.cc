// -*- C++ -*-
//
// Copyright (C) 2001 CTIE, Monash University
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

/**
    @file wirelessethernet.cc
    @brief Predefined values

    @author Eric Wu
    @date 17.04.2003

*/

#include "wirelessethernet.h"

const int INVALID_CHANNEL = -1;
const int INVALID_POWER = -1000;
const int MAX_CHANNEL = 14;
const int CW_MIN = 31;

const double PRECERROR = 0.00000001;
const double RXTXTURNAROUND = 0.000005;
const double SLOTTIME = 0.00002;
const double SIFS = 0.00001;
const double DIFS = 0.00005;
const double ACKLENGTH = 112;

const double TU = 0.001024;

double BASE_SPEED = 2 * 1000000;        // 11 * 1000 * 1000 bps

const int MAX_CHANNELS = 16;

const int TMR_PRBENERGYSCAN = 3001;
const int WIRELESS_SELF_AWAITMAC = 3002;  // FIXME rename to BACKOFF
const int WIRELESS_SELF_BACKOFF = 3003;
const int WIRELESS_SELF_AWAITACK = 3004;
const int WIRELESS_SELF_ENDSENDACK = 3005;
const int WIRELESS_SELF_SCHEDULEACK = 3018;
const int TMR_BEACON = 3006;
const int TMR_REMOVEENTRY = 3007;
const int TMR_AUTHTIMEOUT = 3008;
const int TMR_ASSTIMEOUT = 3009;
const int TMR_PRBRESPSCAN = 3010;
const int TMR_STATS = 3015;
const int TMR_CHANNELSCAN = 3017;

const short FL_FRAMECTRL = 2 * 8;
const short FL_DURATIONID = 2 * 8;
const short FL_ADDR1 = 6 * 8;
const short FL_ADDR2 = 6 * 8;
const short FL_ADDR3 = 6 * 8;
const short FL_ADDR4 = 6 * 8;
const short FL_SEQCTRL = 6 * 8;
const short FL_FCS = 4 * 8;
const short FL_TIMESTAMP = 8 * 8;
const short FL_BEACONINT = 2 * 8;
const short FL_CAPINFO = 2 * 8;
const short FL_SSID = 34 * 8;   // MAX VALUE
const short FL_SUPPRATES = 10 * 8;      // MAX VALUE
const short FL_DSCHANNEL = 3 * 8;
const short FL_STATUSCODE = 2 * 8;
const short FL_CURRENTAP = 6 * 8;
const short FL_REASONCODE = 2 * 8;
const short FL_IEHEADER = 2 * 8;
const short FL_SEQNUM = 2 * 8;

const short WE_MAX_PAYLOAD_BYTES = 2312;

const double collOhDurationBE = 7 * SLOTTIME + SIFS;
const double collOhDurationVI = 4 * SLOTTIME + SIFS;
const double collOhDurationVO = 2 * SLOTTIME + SIFS;
const double successOhDurationBE = SIFS + ACKLENGTH / BASE_SPEED + collOhDurationBE;
const double successOhDurationVI = SIFS + ACKLENGTH / BASE_SPEED + collOhDurationVI;
const double successOhDurationVO = SIFS + ACKLENGTH / BASE_SPEED + collOhDurationVO;
