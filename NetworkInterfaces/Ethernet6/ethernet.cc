// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/NetworkInterfaces/Ethernet6/ethernet.cc,v 1.2 2005/02/10 05:59:32 andras Exp $
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
    @file ethernet.cc
    @brief Predefined values

    @author Eric Wu
    @date 29.10.2002

*/

#include "ethernet.h"

// timer message ID
const int TRANSMIT_SENDDATA = 1001;
const int TRANSMIT_JAM = 1002;
const int SELF_BACKOFF = 2001;
const int SELF_INTERFRAMEGAP = 2002;

// speed
const int BANDWIDTH = 10000000; // 100 Mbps
//const int BANDWIDTH = 500000; // 500kbps

const char* ETH_BROADCAST_ADDRESS="ff:ff:ff:ff:ff:ff";

const int JAM_LENGTH = 4; // bytes

const int MIN_FRAMESIZE = 512; // bits

const unsigned int MAX_RETRY = 15;

const double SLOT_TIME = (double) MIN_FRAMESIZE / BANDWIDTH; // 512 bit times

const int INTER_FRAME_GAP = 12; // bytes

const int WIRELESS_AP_NOTIFY_MAC = -4001;

const int MAC_BRIDGE_REGISTER = -1000;
