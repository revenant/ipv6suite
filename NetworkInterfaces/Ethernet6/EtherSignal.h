/*
// -*- C++ -*-
//
// Eric Wu
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

/ *
    @file EtherSignal.h
    @brief Ethernet control signal (also for simulation wise)
    @author Eric Wu
* /


#ifndef __ETHER_SIGNAL_H
#define __EHTER_SIGNAL_H

#include <string>

#include <omnetpp.h>
#include "ethernet.h"

class EtherFrame6;

class EtherSignal : public cMessage
{
public:
  EtherSignal(EtherSignalType type = EST_None);
  EtherSignal(const EtherSignal& p);

  // assignment operator
  EtherSignal& operator=(const EtherSignal& p);
  virtual EtherSignal *dup() const { return new EtherSignal(*this); }

  // info functions
  virtual const char *className() const { return "EtherSignal"; }
  virtual std::string info() {return std::string();}
  virtual void writeContents(std::ostream& os) {}

  void setSrcModPathName(std::string n) { srcModPathName = n; }
  std::string getSrcModPathName(void) { return srcModPathName; }

  const EtherSignalType& type()
    {
      return _type;
    }
 protected:
  EtherSignalType _type;

 private:
  std::string srcModPathName; // for the matching purpose
};

class EtherSignalData : public EtherSignal
{
 public:
  EtherSignalData(EtherFrame6* frame = 0);
  EtherSignalData(const EtherSignalData &p);
  ~EtherSignalData();

  // assignment operator
  EtherSignalData& operator=(const EtherSignalData& p);
  virtual EtherSignalData *dup() const { return new EtherSignalData(*this); }

  // info functions
  virtual const char *className() const { return "EtherSignalData"; }
  virtual std::string info() {return std::string();}
  virtual void writeContents(std::ostream& os) {}

  EtherFrame6* data();

 private:
  EtherFrame6* _frame;
};

class EtherSignalJam : public EtherSignal
{
 public:
  EtherSignalJam(void);
  EtherSignalJam(const EtherSignalJam& p);

  // assignment operator
  EtherSignalJam& operator=(const EtherSignalJam& p);
  virtual EtherSignalJam *dup() const { return new EtherSignalJam(*this); }

  // info functions
  virtual const char *className() const { return "EtherSignalJam"; }
  virtual std::string info() {return std::string();}
  virtual void writeContents(std::ostream& os) {}
};

class EtherSignalJamEnd : public EtherSignal
{
 public:
  EtherSignalJamEnd(void);
  EtherSignalJamEnd(const EtherSignalJamEnd& p);

  // assignment operator
  EtherSignalJamEnd& operator=(const EtherSignalJamEnd& p);
  virtual EtherSignalJamEnd *dup() const { return new EtherSignalJamEnd(*this); }

  // info functions
  virtual const char *className() const { return "EtherSignalJamEnd"; }
  virtual std::string info() {return std::string();}
  virtual void writeContents(std::ostream& os) {}
};

class EtherSignalIdle : public EtherSignal
{
 public:
  EtherSignalIdle(void);
  EtherSignalIdle(const EtherSignalIdle& p);

  // assignment operator
  EtherSignalIdle& operator=(const EtherSignalIdle& p);
  virtual EtherSignalIdle *dup() const { return new EtherSignalIdle(*this); }

  // info functions
  virtual const char *className() const { return "EtherSignalIdle"; }
  virtual std::string info() {return std::string();}
  virtual void writeContents(std::ostream& os) {}
};

#endif
*/
