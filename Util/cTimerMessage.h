// -*- C++ -*-
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
        @file cTimerMessage.h
        @brief Encapsulate timer messages and the behaviour that goes with
    them when expired.

        @author Johnny Lai
        @date 2.11.01

*/

#ifndef __CMESSAGE_H
#include <cmessage.h>
#endif

#if !defined OPP_VERSION || OPP_VERSION < 3
#ifndef __CMODULE_H
#include <cmodule.h>
#endif
#else
#ifndef __CSIMPLEMODULE_H
#include <csimplemodule.h>
#endif
#endif

#ifndef CASSERT
#define CASSERT
#include <cassert>
#endif

#ifndef FUNCTIONAL
#define FUNCTIONAL
#include <functional>
#endif

#ifndef BOOST_UTILITY_HPP_INCLUDED
#include <boost/utility.hpp>
#endif

#ifndef BOOST_CAST_HPP_INCLUDED
#include <boost/cast.hpp>
#endif

#if !defined BOOST_NONCOPYABLE_HPP_INCLUDED
#include <boost/noncopyable.hpp>
#endif

#if !defined CTIMERMESSAGE_H
#define CTIMERMESSAGE_H

using boost::polymorphic_downcast;

/**
   @class cTimerMessage
   @brief Base class to timer messages.

   Convenience handle to timer messages so that once received and identified via
   (cMessage::isSelfMessage) can invoke callFunc to handle expiry of message.

   @note setOwner function removed. Use OPP_Global::ContextSwitcher when
   appropiate, i.e. during creation of self timer message in another module or
   rescheduling of message that belongs to another module @see PrefixExpiryTimer
   AddressExpiryTimer and RouterExpiryTimer in RoutingTable6 which are managed
   from NDStateHost.
*/
class cTimerMessage: public cMessage, boost::noncopyable
{
 public:
  virtual ~cTimerMessage()
    {
      //Remove these warnings by cancelling messages to be deleted
      //is currently in (cMessageHeap)simulation.scheduled-events, it cannot be
      //deleted.
      //if (isScheduled())
      //cancel();
//      printf("%s in %s at %f is deleted \n", name(), mod->fullPath(), mod->simTime());
    }

  /**
     @brief operator() as a function name looks funny for pointers i.e.
     g->operator()() so give it a name instead

  */
  virtual void callFunc()=0;

  ///Checks timer and cancels if necessary before recheduling to arrivalTime
  void reschedule(simtime_t arrivalTime)
    {
      if(isScheduled())
        cancel();

      //Cannot use this as ownerp changes whenever something else is inserted oh
      //well
      //cSimpleModule* mod = polymorphic_downcast<cSimpleModule*> (ownerp);

      //setOwner(mod);
      //Sometimes it loses plot about who its owner is.
      mod->scheduleAt(arrivalTime, this);
    }

  ///schedule msg for arrival at interval seconds from now
  void rescheduleDelay(simtime_t interval)
    {
      //setOwner(mod);
      reschedule(mod->simTime()+interval);
    }

  void cancel()
    {
      assert(isScheduled());
      mod->cancelEvent(this);
    }

  ///Time remaing till message arrive/timeout expires/callback invoked
  simtime_t remainingTime() const
    {
      //Timer not set yet so message has not been sent
      if (sendingTime() <= 0)
        return 0;

      simtime_t rem = arrivalTime() - mod->simTime();

      //Should only be called when callback has not been invoked yet
      assert(rem >= 0 );

      //well can actually be called during callback when diagnostic output of
      //this class is displayed.
      return rem;
    }

  ///Time elapsed since timer message was triggered/sent
  simtime_t elapsedTime() const
    {
      //Timer not set yet so message has not been sent
      if (sendingTime() <= 0)
        return 0;

      simtime_t elapsed = mod->simTime() - sendingTime();

      assert(elapsed >= 0);

      return elapsed;
    }

 protected:
  cTimerMessage(int message_id, cSimpleModule* module = 0,
                const char* name = NULL):
    cMessage(name),
    mod(boost::polymorphic_downcast<cSimpleModule*> (simulation.contextModule()))
    {
//      if (module != 0)
//        setOwner(mod);
      setKind(message_id);
    }

  cSimpleModule* module() const
    {
      assert(mod != 0);
      return mod;
    }

protected: // needed for cOutVector purposes

  cSimpleModule* mod;
};

/**
   @class cTTimerMessage

   @brief Associate a member function of T as a call back when this message is
   sent with scheduleAt.

   T does not have to be a cSimpleModule.  This object has to have an owner of
   type cSimpleModule for rescheduleTimer to work.

   @deprecated Use cTimerMessageCB
*/
template<class R, class T>
class cTTimerMessage:public cTimerMessage, std::mem_fun_t<R, T>
{
 public:
  cTTimerMessage(int message_id, T* const self, R (T::*f)(),
                 const char* name = NULL)
    : cTimerMessage(message_id, 0, name), std::mem_fun_t<R, T>(f), obj(self)
    {}

  ~cTTimerMessage()
    {}

  virtual void callFunc()
    {
      (*this)(obj);
    };

 private:

  T* obj;
};

///One shot timer to call member function of cSimpleModule
template<class T>
cTimerMessage* createTmrMsg(
  int message_id, T* const module, void (T::*f)(),
  simtime_t arrivalTime, const char* name = NULL)
{
  cTimerMessage* msg = new cTTimerMessage<void, T>(message_id, module, f, name);
  //msg->setOwner(module);
  module->scheduleAt(arrivalTime, msg);
  return msg;
}

///One shot timer to call a member fuction of a non cSimpleModule
template<class T, class C>
cTimerMessage* createTmrMsg(int message_id, T* const module,
                            C* const obj, void (C::*f)(),
                            simtime_t arrivalTime, const char* name = NULL)
{
  cTimerMessage* msg = new cTTimerMessage<void, C>(message_id, obj, f, name);
  //msg->setOwner(module);
  module->scheduleAt(arrivalTime, msg);
  return msg;
}

/**
   @class cTTimerMessageA

   @brief Provide the same functinality as cTTimerMessage except the member
   function can take a single argument.

   This allows multiple triggers of the timer e.g.  implement retransmission
   timers.

   @deprecated Use cTimerMessageCB
*/
template<class Result, class T, class Arg>
class cTTimerMessageA:public cTimerMessage, std::mem_fun1_t<Result, T, Arg*>
{
 public:
  cTTimerMessageA(int message_id, T* const self, Result (T::*f)(Arg*),
                  Arg* a, bool deleteArg = true, const char* name = NULL)
    :cTimerMessage(message_id, 0, name), std::mem_fun1_t<Result, T, Arg*>(f),
     obj(self), _arg(a), ownArg(deleteArg)
    {
      //_arg->msg = this;
    }

  virtual ~cTTimerMessageA()
    {
      if (ownArg)
        delete _arg;
    }

  virtual void callFunc()
    {
      (*this)(obj, _arg);
    }

  Arg* arg() const
    {
      return _arg;
    }

 private:
  ///Disable copy constructor
  //cTTimerMessageA(const cTTimerMessageA& src);
  //cTTimerMessageA& operator =(const cTTimerMessageA& rhs);
  T* obj;
  Arg* _arg;
  bool ownArg;
};


///timer to call member function of cSimpleModule that accepts an argument
template<class T, class Arg>
cTimerMessage* createTmrMsg(
  const int& message_id, T* const module, void (T::*f)(Arg*), Arg* self,
  simtime_t arrivalTime, bool ownArg = true, const char* name = NULL)
{
  cTimerMessage* msg = new cTTimerMessageA<void, T, Arg>(message_id, module, f,
                                                         self, ownArg, name);
  //msg->setOwner(module);
  module->scheduleAt(arrivalTime, msg);
  return msg;
}

///timer to call a member fuction of a non cSimpleModule that accepts an argument
template<class T, class C, class Arg>
/*cTimerMessage*/
cTTimerMessageA<void, C, Arg>* createTmrMsg(const int& message_id, T* const module, C* const obj,
                            void (C::*f)(Arg*), Arg* tmr, simtime_t arrivalTime,
                            bool ownArg = true, const char* name = NULL)
{
  cTTimerMessageA<void, C, Arg>* msg = new cTTimerMessageA<void, C, Arg>
    (message_id, obj, f, tmr, ownArg, name);
  //msg->setOwner(module);
  module->scheduleAt(arrivalTime, msg);
  return msg;
}


/**
   @class cTTimerMessageAS

   @brief Self scheduling timer.

   Clients inherit their timer objects from this class instead of containing it.
   Similar in effect to cTTimerMessageA but less message i.e eradicates ->msg
   and deletion of arg/msg problem because we are the argument/message so the
   callback function can call delete on us.

   @deprecated Use cTimerMessageCB
*/
template<class Module, class Result, class T, class Arg>
class cTTimerMessageAS:public cTimerMessage, std::mem_fun1_t<Result, T, Arg*>
{
public:
  cTTimerMessageAS(const int& message_id, Module* mod, T* const obj,
                   Result (T::*f)(Arg*), bool pause = true,
                   const simtime_t& alarmTime = 0, const char* name = NULL)
    :cTimerMessage(message_id, mod, name), std::mem_fun1_t<Result, T, Arg*>(f),
     _obj(obj)
    {
      assert(!pause && alarmTime > 0 || pause && alarmTime == 0);

      //setOwner(mod);

      if (!pause && alarmTime > 0)
        reschedule(alarmTime);
    }

  //Shouldn't need to be virtual as we're not gonna hold pointers to subclasses
  //of these by this class signature (whatever that means)
  ~cTTimerMessageAS()
    {}

  virtual void callFunc()
    {
      (*this)(_obj, arg());
    }

  /**
   * @warning why does dynamic cast fail is it RTTI doesnt work or cast is really incorrect?
   *
   */

  Module* msgOwner()
    {
      //return polymorphic_downcast<Module*>(module());
      return static_cast<Module*> (module());
    }

private:

  Arg* arg()
    {
      return polymorphic_downcast<Arg*>(this);
    }

private:

  T* _obj;
};

// ///timer to call a member fuction of a non cSimpleModule that accepts an argument
// template<class Module, class Result, class T, class Arg>
// /*cTimerMessage*/
// cTTimerMessageAS<Module, Result, T, Arg>* createTmrMsg(
//   const int& message_id, T* const module, C* const obj,
//   Result (C::*f)(Arg*), bool pause = true, simtime_t arrivalTime = 0,
//   const char* name = NULL)
// {
//   return new cTTimerMessageAS<Result, C, Arg>*
//     (message_id, module, obj, f, pause, arrivalTime, name);
// }


/**
   @class cTimerMessageAE

   @brief Provide the same functinality as cTTimerMessageA with an added expiry
   function that clients can call.

   @deprecated Experimental please do not use
*/
template<class Result, class T, class Arg, class Expire = cTTimerMessageA<Result, T, Arg> >
class cTTimerMessageAE:public cTTimerMessageA<Result, T, Arg>
{
 public:
  cTTimerMessageAE(const int& message_id, T* const self, Result (T::*f)(Arg),
                   Arg a, const Expire& exp, const char* name = NULL): cTTimerMessageA<Result, T, Arg>(message_id, self, f, a, name), expObj(exp)
    {

    }

  ///Should really provide a predicate argument in ctor that calls expire
  ///when callFunc is invoked to test if expire should be called instead
  void expire()
    {
      expObj();
    }

 virtual void callFunc()
    {
      //Check predicate if true
//       if (pred())
//         (*this)(obj, _arg);
//       else
//         expire();
    }

  ~cTTimerMessageAE()
    {}



 private:
  Expire expObj;
};

#endif //CTIMERMESSAGE_H
