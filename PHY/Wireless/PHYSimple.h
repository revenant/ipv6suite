// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/PHY/Wireless/Attic/PHYSimple.h,v 1.2 2005/02/10 06:26:21 andras Exp $
//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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
    @file PHYSimple.h
    @brief Header file for PHYSimple

    Responsibilities:
        - simple physical layer which passes data between layers

    @author Eric Wu
*/

#ifndef __PHY_SIMPLE_MODULE_H__
#define __PHY_SIMPLE_MODULE_H__

#include <list>
#include <omnetpp.h>

class PHYSimple : public cSimpleModule
{
public:
  Module_Class_Members(PHYSimple, cSimpleModule, 0);

  virtual void initialize(void);
  virtual void handleMessage(cMessage* msg);
  virtual void finish();
};

#endif //__PHY_SIMPLE_MODULE_H__

