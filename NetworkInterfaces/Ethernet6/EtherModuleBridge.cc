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
	@file EtherModuleBridge.cc
	@brief Definition file for EtherModuleBridge

	simple implementation of ethernet interface in Bridge

	@author Eric Wu
*/

#include "sys.h"
#include "debug.h"
#include "EtherModuleBridge.h"

Define_Module_Like( EtherModuleBridge, NetworkInterface );

void EtherModuleBridge::initialize(int stage)
{
  EtherModuleAP::initialize(stage);
}

void EtherModuleBridge::handleMessage(cMessage* msg)
{
  EtherModule::handleMessage(msg);
}

void EtherModuleBridge::finish(void)
{}

bool EtherModuleBridge::receiveData(std::auto_ptr<cMessage> msg)
{
  return EtherModuleAP::receiveData(msg);
}

bool EtherModuleBridge::sendData(EtherFrame* frame)
{
  return EtherModuleAP::sendData(frame);
}

