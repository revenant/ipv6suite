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

#ifndef __ETH_MISC_H
#define __ETH_MISC_H

// timer message ID
extern const int TRANSMIT_SENDDATA;
extern const int TRANSMIT_JAM;
extern const int SELF_BACKOFF; // for WaitBackoff
extern const int SELF_INTERFRAMEGAP;

//extern const int SELF_BACKOFF_BJ; // for WaitBackoffJam
//extern const int SELF_BACKOFF_RB; // for WaitReceiveBackoff

// speed
extern const int BANDWIDTH; // Mbps

// predefined MAC addresses
extern const char* ETH_BROADCAST_ADDRESS;

// Jam data lenth
extern const int JAM_LENGTH;

extern const int INTER_FRAME_GAP;

// minimum Frame size
extern const int MIN_FRAMESIZE; //

// constants
extern const unsigned int MAX_RETRY;
extern const double SLOT_TIME; // sec

extern const int WIRELESS_AP_NOTIFY_MAC;

extern const int MAC_BRIDGE_REGISTER;



#endif //__ETH_MISC_H
