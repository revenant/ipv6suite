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
	@file WirelessEtherAssociationReceiveMode.h
	@brief Header file for WEAssociationReceiveMode
    

	@author	Steve Woon
          Eric Wu
*/

#ifndef __WIRELESS_ETHER_ASSOCIATION_RECEIVE_MODE_H__
#define __WIRELESS_ETHER_ASSOCIATION_RECEIVE_MODE_H__

#include "WirelessEtherReceiveMode.h"

class WESignalData;
class WirelessEtherModule;

class WEAssociationReceiveMode : public WEReceiveMode
{
	public:
		virtual void handleAuthentication(WirelessEtherModule* mod, WESignalData* signal);
		virtual void handleDeAuthentication(WirelessEtherModule* mod, WESignalData* signal);
		virtual void handleAssociationResponse(WirelessEtherModule* mod, WESignalData* signal);
		virtual void handleReAssociationResponse(WirelessEtherModule* mod, WESignalData* signal);
		virtual void handleAck(WirelessEtherModule* mod, WESignalData* signal);
		
		static WEAssociationReceiveMode* instance();

	protected:
		static WEAssociationReceiveMode* _instance;
		WEAssociationReceiveMode(void) {}
};

#endif // __WIRELESS_ETHER_ASSOCIATION_RECEIVE_MODE_H__
