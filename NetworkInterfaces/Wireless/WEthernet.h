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
//

/**
    @file ethernet.h
    @brief Predefined values

    @author Eric Wu
    @date 29.10.2002

*/

#ifndef __WETHERNET_H
#define __WETHERNET_H

// timer message ID
extern const int WE_TRANSMIT_SENDDATA;
extern const int WE_SELF_BACKOFF; // for WaitBackoff
extern const int WE_SELF_INTERFRAMEGAP;

// speed
extern const int WE_BANDWIDTH; // Mbps

// predefined MAC addresses
extern const char* WE_BROADCAST_ADDRESS; // XXX merge with Eth?

// minimum Frame size
extern const int WE_MIN_FRAMESIZE; //

// constants
extern const int WE_AP_NOTIFY_MAC;

extern const int WE_MAC_BRIDGE_REGISTER;

// XXX FIXME temporarily thrown in here -- find a proper place for it! --AV
#include "WirelessEtherSignal_m.h"
inline WESignalData *encapsulateIntoWESignalData(cMessage *msg) {
    WESignalData *signal = new WESignalData(msg->name());
    signal->encapsulate(msg);
    signal->setLength(signal->length() * 8); // convert into bits
    return signal;
}

#define WEBASICFRAME_IN(wesignal)  check_and_cast<WirelessEtherBasicFrame*>((wesignal)->encapsulatedMsg())

#endif //__ETH_MISC_H
