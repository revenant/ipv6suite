// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Util/Topology/DynamicTopologyBuilder.h,v 1.1 2005/02/09 06:15:59 andras Exp $
// Copyright (C) 2002 CTIE, Monash University 
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
 * @file DynamicTopologyBuilder.h
 * @author Johnny Lai
 * @date 19 Nov 2002
 *
 * @brief Dynamic Module Creation of Topology based on XML Routing Table
 *
 */

#ifndef DYNAMICTOPOLOGYBUILDER_H
#define DYNAMICTOPOLOGYBUILDER_H 1

#include <cmodule.h>
#include <macros.h>

/**
 * @class DynamicTopologyBuilder
 *
 * @brief Create modules dynamically from XML configuration
 * @ingroup Prototype
 *
 * detailed description
 */

class DynamicTopologyBuilder: public cSimpleModule
{
public:
  Module_Class_Members(DynamicTopologyBuilder, cSimpleModule, 0);
  
  //@name constructors, destructors and operators
  //@{
  // DynamicTopologyBuilder();

  virtual ~DynamicTopologyBuilder();

  //@}

  //@name omnetpp overrides
  //@{
  void initialize();
  void handleMessage(cMessage *msg);
  void finish();
  //@}
protected:
  
private:
  
};

Define_Module(DynamicTopologyBuilder);


#endif /* DYNAMICTOPOLOGYBUILDER_H */
