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
    @file wirelessethernet.h
    @brief Predefined values

    @author Eric Wu
    @date 17.04.2003

*/

#ifndef __WIRELESS_ETH_MISC_H
#define __WIRELESS_ETH_MISC_H

#include <sstream>
#include <iomanip>
#include "WEthernet.h"

// constants

extern const int INVALID_CHANNEL; // represent an invalid channel
extern const int INVALID_POWER;   // represent an invalid power

extern const int MAX_CHANNEL; // maximum number of frequency channels

extern const int CW_MIN;

// predefined constants in 802.11 standard

extern const double TU;    // Time Unit

extern const double PRECERROR;
extern const double RXTXTURNAROUND; // 5 us?; time to switch from rx to tx state
extern const double SLOTTIME; // 15 us; a slot time
extern const double SIFS; // 20 us; Short Interframe Space
extern const double DIFS; // 50 us; Distributed Interframe Space
extern const double ACKLENGTH; // 112 bits
extern const int MAX_CHANNELS; // 16 frequency bands in wireless LAN

// specific 802.11 message kind, message name
extern const int TMR_PRBENERGYSCAN;
extern const int WIRELESS_SELF_AWAITMAC;
extern const int WIRELESS_SELF_BACKOFF;
extern const int WIRELESS_SELF_AWAITACK;
extern const int WIRELESS_SELF_ENDSENDACK;
extern const int WIRELESS_SELF_SCHEDULEACK;
extern const int TMR_BEACON;
extern const int TMR_REMOVEENTRY;
extern const int TMR_AUTHTIMEOUT;
extern const int TMR_ASSTIMEOUT;
extern const int TMR_PRBRESPSCAN;
extern const int TMR_STATS;
extern const int TMR_CHANNELSCAN;

// FieldLength
extern const short FL_FRAMECTRL;
extern const short FL_DURATIONID;
extern const short FL_ADDR1;
extern const short FL_ADDR2;
extern const short FL_ADDR3;
extern const short FL_ADDR4;
extern const short FL_SEQCTRL;
extern const short FL_FCS;
extern const short FL_TIMESTAMP;
extern const short FL_BEACONINT;
extern const short FL_CAPINFO;
extern const short FL_SSID;  //MAX VALUE
extern const short FL_SUPPRATES; //MAX VALUE
extern const short FL_DSCHANNEL;
extern const short FL_STATUSCODE;
extern const short FL_CURRENTAP;
extern const short FL_REASONCODE;
extern const short FL_IEHEADER;
extern const short FL_SEQNUM;

extern const short WE_MAX_PAYLOAD_BYTES;

extern const double collOhDurationBE;
extern const double collOhDurationVI;
extern const double collOhDurationVO;

extern const int BASE_SPEED;

//Receive mode
enum ReceiveMode
{
  RM_UNSPECIFIED = 0,
  RM_ACTIVESCAN = 1,
  RM_PASSIVESCAN = 2,
  RM_AUTHENTICATION = 3,
  RM_ASSOCIATION = 4,
  RM_DATA = 5,
  RM_ACK = 6,
  RM_MONITOR = 7,
  RM_ASSRSP_ACKWAIT = 8,
  RM_AUTHRSP_ACKWAIT = 9
};

//QoS queues
enum
{
    AC_BK = 0,
    AC_BE = 1,
    AC_VI = 2,
    AC_VO = 3
};

inline std::string formatTime(simtime_t t) {
    std::stringstream out;
    out << std::fixed << std::showpoint << std::setprecision(12) << t;
    return out.str();
}

inline std::string currentTime() {
    return formatTime(simulation.simTime());
}

#endif //__WIRELESS_ETH_MISC_H
