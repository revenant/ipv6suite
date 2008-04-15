// -*- C++ -*-
// Copyright (C) 2008 Johnny Lai
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


/**
 * @file   RTPVoip.cc
 * @author Johnny Lai
 * @date   16 Apr 2008
 *
 * @brief  Implementation of RTPVoip
 *
 * @todo
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include "RTPVoip.h"

Define_Module(RTP);

int RTPVoip::numInitStages() const
{
  return 2;
}

void RTPVoip::initialize(int stageNo)
{
}

void RTPVoip::initialize()
{
}

void RTPVoip::finish()
{
}

void RTPVoip::handleMessage(cMessage* msg)
{
}

///For non omnetpp csimplemodule derived classes
RTPVoip::RTPVoip()
{
}

RTPVoip::~RTPVoip()
{
}
