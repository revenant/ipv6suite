//
// Copyright 2004 Monash University, Australia
//
// This library is free software, you can redistribute it and/or modify
// it under  the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation;
// either version 2 of the License, or any later version.
// The library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//

#ifndef __VBRSRCMODEL_H_
#define __VBRSRCMODEL_H_

#include <omnetpp.h>
#include "BRSrcModel.h"

/**
 * Single-connection VBR application.
 */
class VBRSrcModel : public BRSrcModel
{
 public:
  Module_Class_Members(VBRSrcModel, BRSrcModel, 0);
  
  virtual void initialize();
  virtual void handleMessage(cMessage* msg);
  
 protected:
  virtual void sendPacket();
  
  unsigned long pixPerFrame; //pixels/frame
  unsigned long frameRate;   //frames/second
  
  double a;
  double b;
  double normalMean;
  double previousBitRate;

 private:
};

#endif


