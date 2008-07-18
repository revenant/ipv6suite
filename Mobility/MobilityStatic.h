// -*- C++ -*-
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
    @file MobilityStatic.h
    @brief Header file for MobilityStatic

    Responsibilities:
             - mobility handling

    @author Eric Wu, Steve Woon
*/

#ifndef __MOBILITY_STATIC_H__
#define __MOBILITY_STATIC_H__

#include "MobilityHandler.h"

using namespace std;

class MobilityStatic:public MobilityHandler
{
  friend class XMLConfiguration::IPv6XMLParser;
  friend class XMLConfiguration::IPv6XMLWrapManager;
  friend class XMLConfiguration::XMLOmnetParser;

public:
  Module_Class_Members(MobilityStatic, MobilityHandler, 0);

  virtual void initialize(int stage);
  virtual void handleMessage(cMessage* msg);
  virtual void finish();

  virtual int  numInitStages() const  {return 3;}
};

#endif

