// -*- C++ -*-
// Copyright (C) 2006 
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
 * @file   RTCPPacket.cc
 * @author 
 * @date   29 Jul 2006
 *
 * @brief  Implementation of RTCPPacket
 *
 * @todo
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include "RTCPPacket.h"
#include "RTPPacket.h"

//Register_Class(RTPPacket);
Register_Class(RTCPPacket);

unsigned int RTCPReports::reportBlocksArraySize() const
{
  //limit imposed by rfc 3550
  assert(blocks.size() < 31); 
  return blocks.size();
}

RTCPReportBlock& RTCPReports::reportBlocks(unsigned int k)
{
  assert(k < blocks.size());
  if (k >= blocks.size())
    opp_error("index of report blocks %d is greater than size %d", k, blocks.size());
  return blocks[k];
}

void RTCPReports::setReportBlocks(unsigned int k, const RTCPReportBlock& reportBlocks_var)
{
  assert(k < blocks.size());
  blocks[k] = reportBlocks_var;
}

void RTCPReports::addBlock(const RTCPReportBlock& b)
{
  setByteLength(byteLength() + 6*4);
  blocks.push_back(b);
}

const char * RTCPSDES::cname() const
{
  return _cname.c_str();
}

void RTCPSDES::setCname(const char * cname_var)
{
  setByteLength(byteLength() - _cname.size());
  _cname = cname_var;
  setByteLength(byteLength() + _cname.size() + ((_cname.size()%4)?
					       (4 - _cname.size()%4):0));
}
