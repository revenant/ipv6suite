// -*- C++ -*-
// Copyright (C) 2001 Monash University, Melbourne, Australia
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

/*
	@file WirelessEtherPScanReceiveMode.h
	@brief Header file for WEPScanReceiveMode
    

	@author	Steve Woon
          Eric Wu
*/

#ifndef __WIRELESS_ETHER_PSCAN_RECEIVE_MODE_H__
#define __WIRELESS_ETHER_PSCAN_RECEIVE_MODE_H__

#include "WirelessEtherReceiveMode.h"

class WESignalData;
class WirelessEtherModule;

class WEPScanReceiveMode : public WEReceiveMode
{
	public:
		virtual void handleBeacon(WirelessEtherModule* mod, WESignalData* signal);
		static WEPScanReceiveMode* instance();
	
	protected:
		static WEPScanReceiveMode* _instance;
		WEPScanReceiveMode(void) {}
};

#endif // __WIRELESS_ETHER_PSCAN_RECEIVE_MODE_H__
