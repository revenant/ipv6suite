// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/ICMPv6Combine.h,v 1.1 2005/02/09 06:15:57 andras Exp $
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
    @file ICMPv6Combine.h
    Simple Ned module ICMPv6Combine definition
	@author Johnny Lai
*/

#ifndef __ICMPv6COMBINE_H__
#define __ICMPv6COMBINE_H__

class ICMPv6Combine: public cSimpleModule
{
public:
  Module_Class_Members(ICMPv6Combine, cSimpleModule, 0);
  virtual void handleMessage(cMessage* msg);
};

#endif //__ICMPv6COMBINE_H__

