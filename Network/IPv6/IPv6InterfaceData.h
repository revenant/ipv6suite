// -*- C++ -*-
//
// Copyright (C) 2001, 2004 CTIE, Monash University
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
    @file IPv6InterfaceData.h

    @brief Abstract base classes to transfer arbitrary data between
    OSI layers

    @author Johnny Lai
    @date 18.9.01
    @test

*/

#ifndef __IPv6INTERFACEDATA_H__
#define __IPv6INTERFACEDATA_H__

#include "IPv6InterfacePacket.h"

/**
   @class IPv6InterfaceData

   Template derived from IPv6InterfacePacket.  It's purpose is to
   encapsulate arbitrary data from upper layers to IP Layer as
   a template parameter.  Datatype can be struct, normal
   object or a pointer to an object.  A partial specialization exists
   for pointer.

   Prerequisites for Datatype
     Copy constructor, destructor

   Prerequisites for pointers:
     length() required to send across network
     Pack/unPack() to allow streaming of object to/from a byte array

     @deprecated Whole InterfaceData will be removed soon as omnetpp 3 provides
     better mechanism.
 */

template<class DataType>
class IPv6InterfaceData: public IPv6InterfacePacket
{
public:
  IPv6InterfaceData(const DataType& data, const char* src = 0, const char* dest = 0);
  IPv6InterfaceData(const  DataType& data, const ipv6_addr& src, const ipv6_addr& dest);
  IPv6InterfaceData(const IPv6InterfaceData& src);
  IPv6InterfaceData& operator=(const IPv6InterfaceData& ip);

  virtual IPv6InterfaceData* dup() const { return new IPv6InterfaceData(*this); }
  virtual const char* className() const { return "IPv6InterfaceData"; }
  virtual std::string info();

      ///Insert new data and assign ownership of data to this object
  void insertData(const DataType& data);
      ///Obtain a temporary reference to DataType for use or duplication
  DataType& data() const;
      ///Remove ownership of DataType from this object and return it.
  DataType removeData();

private:
  mutable DataType _data;
};

/**
   @class IPv6InterfaceData<void*>

   Provide common implmentation for all template types using partial
   template specialisation.

   Actually this isn't really as useful as it could be since void * can
   point to anything and not just objects so in effect all this serves
   is a common pointer storage
 */
template<>
class IPv6InterfaceData<void*>: public IPv6InterfacePacket
{
protected:

  IPv6InterfaceData(void* obj = 0)
    :p(obj)
    {}

  void* data() const
    {
      return p;
    }

  void* removeData()
    {
    void* retval = p;
    p = 0;
    setLength(0);
    return retval;
    };

  void insertData(void* newObj)
    {
          //setLength(newObj->length());
          //delete p;
      p = newObj;
    }
private:

  mutable void* p;
};

/**
   @class IPv6InterfaceData<T*>

   Partial template speciliation for pointers.  Implemented using
   private inheritance to Base class so that all pointer types
   share same implementation.  Redefine methods to return the correct
   type of pointer even though internally its stored as a void* in
   base class.

   @note Unused. Should use msgc from omnetpp or opp3's interface
   packet replacement mechanism which is far superior.
 */
template<class T>
class IPv6InterfaceData<T*>: private IPv6InterfaceData<void*>
{
public:

  typedef IPv6InterfaceData<void*> Base;
  IPv6InterfaceData(T* obj = 0)
    :Base(obj)
    {
      data()?setLength(data()->length()):setLength(0);
    }

  virtual ~IPv6InterfaceData()
    {
      delete removeData();
    }


  T* data() const
    {
      return static_cast<T*>(Base::data());
    }
  T* removeData()
    {
      return static_cast<T*>(Base::removeData());
    }

  void  insertData(T* obj)
    {
      delete removeData();
      setLength(obj->length());
      Base::insertData(obj);
    }

private:

};

#if !defined EXPORT_KEYWORD_DEFINED
#include "IPv6InterfaceData.cc"
#endif
#endif //__IPv6INTERFACEDATA_H__
