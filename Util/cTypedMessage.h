// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Util/Attic/cTypedMessage.h,v 1.3 2005/02/16 00:41:32 andras Exp $
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
        @file cTypedMessage.h
        @brief Typesafe cMessage class to eliminate use of cPars in cMessages


        @author Johnny Lai
        @date 2.11.01

*/
#include "cmessage.h"

#if !defined CTYPEDMESSAGE_H
#define  CTYPEDMESSAGE_H
template<class T>
class cTypedMessage: public cMessage
{
public:
  cTypedMessage(const T& data):_data(data){};
  virtual cTypedMessage* dup() const;
  T& data() { return _data; }
private:
  T _data;
};

template<class T>
class cTypedMessage<T*>: public cMessage
{
public:
  explicit cTypedMessage(const T*& data):_data(data){};
  virtual cTypedMessage* dup() const;
  virtual ~cTypedMessage();
  cTypedMessage& operator=(const cTypedMessage& src);
  T* data() { return _data; }
  T* release()
    {
      T* temp = _data;
      _data = 0;
      return temp;
    }

private:
  T* _data;
};

template<class T>
cTypedMessage<T>* cTypedMessage<T>::dup() const
{
  cTypedMessage<T>* dupl = new cTypedMessage(_data);
  //@warning omnetpp does not dup this but we want to
  dupl->setKind(kind());  // XXX yes sure it DOES! --AV
  return dupl;
}

template<class T>
cTypedMessage<T*>* cTypedMessage<T*>::dup() const
{
  cTypedMessage<T>* dupl = new cTypedMessage<T*>(new T(*_data));
 //@warning omnetpp does not dup this but we want to
  dupl->setKind(kind());  // XXX yes sure it DOES! --AV
  return dupl;
}

template<class T>
cTypedMessage<T*>::~cTypedMessage()
{
  delete data;
}

template<class T>
cTypedMessage<T*>& cTypedMessage<T*>::operator=(const cTypedMessage<T*>& src)
{
  // delete data;
  //data = new T(*(src->data));
  //Reuse is probably more efficient
  *data = *(src.data);
  return *this;
}

#endif // CTYPEDMESSAGE_H
