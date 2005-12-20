//
// Copyright (C) 2005 Eric Wu
// Copyright (C) 2004 Andras Varga
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
//

#ifndef __DYNAMICBRLOADER_H__
#define __DYNAMICBRLOADER_H__

#include <omnetpp.h>

class DynamicBRLoader : public cSimpleModule
{
  public:
    Module_Class_Members(DynamicBRLoader, cSimpleModule, 0);

  protected:
    virtual void initialize(int stage);
    virtual void createModule(std::string src) {}
    virtual int numInitStages() const  {return 3;}
    
    int numNodes;
};

class DynamicIPv6CBRLoader : public DynamicBRLoader
{
  public:
    Module_Class_Members(DynamicIPv6CBRLoader, DynamicBRLoader, 0);

  protected:
    virtual void initialize(int stage);
    virtual void createModule(std::string src);
    
};


#endif

