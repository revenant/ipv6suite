// -*- C++ -*-
// Copyright (C) 2004 Johnny Lai
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
 * @file cTimerMessageCB.h
 * @author Johnny Lai
 * @date 20 May 2004
 *
 * @brief Definition of class cTimerMessageCB which supersedes all other
 * callback timers
 *
 * Transferred from cTTimerMessageCB.h. cTimerMessageCB was created sometime in
 * Jul/Aug of 2002.
 * 
 */

#ifndef CTIMERMESSAGECB_H
#define CTIMERMESSAGECB_H 1

#ifndef CTIMERMESSAGE_H
#include "cTimerMessage.h"
#endif //CTIMERMESSAGE_H

#ifndef FUNCTOR_INC_
#include <Loki/Functor.h>
#endif //FUNCTOR_INC_

#ifndef HIERARCHYGENERATORS_INC_
#include <Loki/HierarchyGenerators.h> //Tuple
#endif // HIERARCHYGENERATORS_INC_

namespace Loki
{

  /**
   * @class cTimerMessageCB
   *
   * @brief A generic cTimerMessage class implemented using Loki::Functor for
   * the callback
   *
   * Loki's Functor class template is much more flexible than the previous
   * callback mechanism which required different class template for different
   * func signatures.  The approach taken by Loki is for the compiler to
   * generate the required number of arguments and and deduce the correct
   * specialisation from that. Refer to Modern C++ Design for details
   *
   * Template parameters are:
   * - R is the return type of the callback function
   * - TList is a TypeList representing all the arguments in the callback function (no arguments by default)
   * - ThreadingModel is by default for single threaded use.
   * @note Use this in preference to every other callback timer class
   * @see  MIPv6MStateMobileNode::scheduleSendBU for an example on how to use this class
   */

  template < typename R, class TList = NullType,
             template <class> class ThreadingModel = DEFAULT_THREADING>
  class cTimerMessageCB:public cTimerMessage,
                        public Functor<R, TList, ThreadingModel>
  {
  public:
    typedef typename Functor<R, TList, ThreadingModel>::ParmList ParmList;

    //@name constructors, destructors and operators
    //@{
    /*
      ctor for member functions
      @param mesage_id has to be unique within the simpleModule 
      @param module is the simple module to invoke this self message's memFn from
      @param p  is pointer to object to invoke member function on
      @param memFn is the pointer to member function i.e. &Class::memberfunc
      @param name is cObject's data member for debug purposes
      @note name is required arg as a default param of 0 causes ambiguous
      overload compn error on gcc2.96
    */
    template <class PtrObj, typename MemFn>
    cTimerMessageCB(int message_id, cSimpleModule* module, 
                 const PtrObj& p, MemFn memFn, const char* name)
      :cTimerMessage(message_id, module, name), Functor<R, TList, ThreadingModel>(p, memFn)
      {}
    
    /*
      ctor for global functions
      @param mesage_id has to be unique within the simpleModule 
      @param module is the simple module to invoke this self message's memFn from
      @param fun is the global function pointer 
      @param name is cObject's data member for debug purposes
     */
    template <typename Fun>
    cTimerMessageCB(int message_id, cSimpleModule* module, 
                 Fun fun, const char* name)
      :cTimerMessage(message_id, module, name), Functor<R, TList, ThreadingModel>(fun)
      {}

    //@}

    virtual void callFunc()
      {
        compileTimeDispatch(Int2Type< Loki::TL::Length<ParmList>::value >());
      }

    virtual R callFuncRet()
      {
        return compileTimeDispatch(Int2Type< Loki::TL::Length<ParmList>::value >());
      }

    /*
      @brief Argument list (needs to be filled in if TList is not NullType)
      @see MIPv6NDStateHost::returnHome() for usage example
    */
    Loki::Tuple< ParmList> args;

  private:

    R compileTimeDispatch(Int2Type<0>)
      {
        return (*this)();
      }
    
    R compileTimeDispatch(Int2Type<1>)
      {
        return (*this)(Field<0>(args));
      }
    
    R compileTimeDispatch(Int2Type<2>)
      {
        return (*this)(Field<0>(args), Field<1>(args));
      }
    
    R compileTimeDispatch(Int2Type<3>)
      {
        return (*this)(Field<0>(args), Field<1>(args), Field<2>(args));
      }

    R compileTimeDispatch(Int2Type<4>)
      {
        return (*this)(Field<0>(args), Field<1>(args), Field<2>(args), Field<3>(args));
      }
  };

};



#endif /* CTIMERMESSAGECB_H */

