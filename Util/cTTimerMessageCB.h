// -*- C++ -*-
//
// Copyright (C) 2002, 2004 CTIE, Monash University
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
 * @file cTTimerMessageCB.h
 * @author Johnny Lai
 * @date 08 May 2002
 *
 * @brief cTimerMessage subclasses that allows modification of callback function
 * during runtime
 *
 *
 * @note Use cTimerMessageCB which supersedes all other cTTimer* classes. The
 * others are all deprecated and will be replaced in future by cTimerMessageCB.
 */

#ifndef CTTIMERMESSAGECB_H
#define CTTIMERMESSAGECB_H

#ifndef CTIMERMESSAGE_H
#include "cTimerMessage.h"
#endif //CTIMERMESSAGE_H

/**
 * @class TFunctorBaseA
 *
 * @brief Abstract base class for the callback mechanism.  Defines a functor
 * accepting a cTimerMessage as an argument.
 *
 * This is not as flexible as we would like i.e you still need to have other
 * TFunctor bases for different function signatures.
 */

template<class Arg>
class TFunctorBaseA
{
public:

  ///The function to invoke the callback with
  virtual void operator()(Arg* msg) = 0;

  ///convenience function that concrete classes override (may actually be unused
  ///:( )
  virtual bool operator==(const TFunctorBaseA& rhs) const = 0;

  ///Has to be virtual because clients contain pointers to TFunctors not the concrete subclasses
  virtual ~TFunctorBaseA() = 0;
};

template<class Arg>
TFunctorBaseA<Arg>::~TFunctorBaseA()
{}

/**
 * @class cTTimerMessageCBA<Arg, Result>
 * @brief General template for callbacks involving timer messages
 */

template<class Arg, class Result>
class cTTimerMessageCBA:public cTimerMessage
{
public:
  cTTimerMessageCBA(const int& message_id, cSimpleModule* module,
                    TFunctorBaseA<Arg>* cb, Arg* arg = 0,
                    bool deleteArg = true, const char* name = NULL,
                    bool deleteFunctor = false)
    :cTimerMessage(message_id, module, name), _functor(cb), _arg(arg), ownArg(deleteArg), functorDelete(deleteFunctor)
    {}

  ~cTTimerMessageCBA()
    {
      if (functorDelete)
        delete _functor;
      if (ownArg)
        delete _arg;
    }

  ///Don't delete this unless policy of deleteArg is set to false
  Arg* arg() { return _arg; }

  /**
   * Use the global function makeCallback to create the correct concrete class.
   *
   */
  void setFunctor(TFunctorBaseA<Arg>* callback)
    {
      if (functorDelete)
        delete _functor;
      _functor = callback;
    }

  const TFunctorBaseA<Arg>* functor() { return _functor; }

  virtual void callFunc() { res = (*_functor)(_arg); }

  const Result& result() { return res; };

private:
  TFunctorBaseA<Arg>* _functor;
  Arg* _arg;
  Result res;
  bool ownArg;
  bool functorDelete;
};

/**
 * @class cTTimerMessageCBA<Arg, void>
 *
 * @brief partial specialization of cTTimerMessage<Arg, Result> with no return
 * in callback
 * @deprecated Use cTimerMessageCB
 */

template<class Arg>
class cTTimerMessageCBA<Arg, void>:public cTimerMessage
{
public:
  cTTimerMessageCBA(const int& message_id, cSimpleModule* module,
                    TFunctorBaseA<Arg>* cb, Arg* arg = 0,
                    bool deleteArg = true, const char* name = NULL,
                    bool deleteFunctor = false)

    :cTimerMessage(message_id, module, name), _functor(cb), _arg(arg), ownArg(deleteArg), functorDelete(deleteFunctor)
    {}
  ~cTTimerMessageCBA()
    {
      if (functorDelete)
        delete _functor;
      if (ownArg)
        delete _arg;
    }

  ///Don't delete this unless policy of deleteArg is set to false
  Arg* arg() { return _arg; }

  virtual void callFunc() { (*_functor)(_arg); }

  /**
   * Use the global function makeCallback to create the correct concrete class.
   *
   */
  void setFunctor(TFunctorBaseA<Arg>* callback)
    {
      if (functorDelete)
        delete _functor;
      _functor = callback;
    }

  const TFunctorBaseA<Arg>* functor() { return _functor; }

  void setArg(Arg* arg)
    {
      if (ownArg)
        delete _arg;
      _arg = arg;
    }

private:
  TFunctorBaseA<Arg>* _functor;
  Arg* _arg;
  bool ownArg;
  bool functorDelete;
};

/**
 * @class cTTimerMessageCBA<cTimerMessage, void>
 *
 * @brief full specialization of cTTimerMessage<cTimerMessage, Result> with no
 * return in callback
 *
 * Used for self timers i.e. timers that pass themselves as arguments to callback
 * @deprecated Use cTimerMessageCB
 */

template<>
class cTTimerMessageCBA<cTimerMessage, void>:public cTimerMessage
{
public:
  cTTimerMessageCBA(const int& message_id, cSimpleModule* module,
                    TFunctorBaseA<cTimerMessage>* cb,
                    const char* name = NULL, bool deleteFunctor = true)

    :cTimerMessage(message_id, module, name), _functor(cb),
     functorDelete(deleteFunctor)
    {}
  ~cTTimerMessageCBA()
    {
      if (functorDelete)
        delete _functor;
    }

  virtual void callFunc() { (*_functor)(this); }

  /**
   * Use the global function makeCallback to create the correct concrete class.
   *
   */
  void setFunctor(TFunctorBaseA<cTimerMessage>* callback)
    {
      if (functorDelete)
        delete _functor;
      _functor = callback;
    }

  const TFunctorBaseA<cTimerMessage>* functor() { return _functor; }

private:
  TFunctorBaseA<cTimerMessage>* _functor;
  bool functorDelete;
};

/**
 * @class cTTimerMessageCBA<void, void>
 *
 * @brief Full specialization of cTTimerMessageCBA for a callback function that
 * does not accept or return anything
 *
 * The callback function can be changed during runtime by calling setFunctor and
 * using makeCallback for whichever function you want to set as the callback.

 * @warning However functor is deleted by default so its different from the
 * other variants.

 * @todo reduce cut and paste of common member funcs
 * @deprecated Use cTimerMessageCB
 */
template <>
class cTTimerMessageCBA<void, void>: public cTimerMessage
{
public:

  cTTimerMessageCBA(const int& message_id, cSimpleModule* module,
                   TFunctorBaseA<void>* f = 0, const char* name = NULL)
    :cTimerMessage(message_id, module, name), _functor(f)
    {}

  virtual void callFunc() { (*_functor)(0); }

  ~cTTimerMessageCBA() { delete _functor; }

  /**
   * Use the global function makeCallback to create the correct concrete class.
   *
   */
  void setFunctor(TFunctorBaseA<void>* callback) { delete _functor; _functor = callback; }

  const TFunctorBaseA<void>* functor() { return _functor; }

private:
  TFunctorBaseA<void>* _functor;
};

/**
 * @class TFunctorA<T, Arg, Result>
 * @brief concrete class of TFunctorBaseA
 *
 * It has been intentionally unimplmented as a callback function with a return
 * value signature is not used yet
 */

template<class T, class Arg, class Result>
class TFunctorA:public TFunctorBaseA<Arg>
{
public:

  Result* res;
};

/**
 * @class TFunctorA<T, Arg, void>
 * @brief specialation of TFunctorA for callback functions returning void
 */

template <class T, class Arg>
class TFunctorA<T, Arg, void>:public TFunctorBaseA<Arg>
{
public:
  TFunctorA(T* obj, void (T::*fpt)(Arg*))
    :functor(obj), mfptr(fpt)
    {}
  virtual void operator()(Arg* msg)
    {
      (*functor.*mfptr)(msg);
    }
  virtual bool operator==(const TFunctorBaseA<Arg>& rhs) const
    {
      const TFunctorA& rhsvoid = static_cast<const TFunctorA&>(rhs);
      return mfptr == rhsvoid.mfptr && functor == rhsvoid.functor;
    }

private:
  T* functor;
  void (T::*mfptr)(Arg*);
};

/**
 * @class TFunctor<T, void, void>
 *
 * @brief specialisation for member functions that do not take arguments or
 * return.
 *
 * This is too repetitive.  However since void members don't make sense ( even
 * if they do the function signature would be rejected by compiler) this is the
 * only way out.
 *
 * @todo reduce cut and paste of common member funcs
 */

template<class T>
class TFunctorA<T, void, void>: public TFunctorBaseA<void>
{
public:
  TFunctorA(T* obj, void (T::*fpt)(void))
    :functor(obj), mfptr(fpt)
    {}

  virtual void operator()(void* msg)
    {
      (*functor.*mfptr)();
    }
  virtual bool operator==(const TFunctorBaseA<void>& rhs) const
    {
      const TFunctorA& rhsvoid = static_cast<const TFunctorA&>(rhs);
      return mfptr == rhsvoid.mfptr && functor == rhsvoid.functor;
    }
private:
  T* functor;
  void (T::*mfptr)(void);
};


/**
 * @class FunctorA<Arg, Result>
 * @brief concrete implementation of callback mechanism for global functions
 */

template<class Arg, class Result>
class FunctorA:public TFunctorBaseA<Arg>
{
public:
  typedef Result (*F)(Arg*);
  FunctorA(F fptr)
    :fptr(fptr)
    {}

  virtual void operator()(Arg* msg)
    {
      res = (*fptr)(msg);
    }

  virtual bool operator==(const TFunctorBaseA<Arg>& rhs) const
    {
      const FunctorA& rhsvoid = static_cast<const FunctorA&>(rhs);
      return fptr == rhsvoid.fptr;
    }

private:
  F fptr;
  Result res;
};

/**
 * @class FunctorA<Arg, void>
 *
 * @brief Partial specialisation for global functions that do not return
 */

template<class Arg>
class FunctorA<Arg, void>:public TFunctorBaseA<Arg>
{
  typedef void (*F)(Arg*);
  FunctorA(F fptr)
    :fptr(fptr)
    {}

  virtual void operator()(Arg* msg)
    {
      (*fptr)(msg);
    }

  virtual bool operator==(const TFunctorBaseA<Arg>& rhs) const
    {
      const FunctorA& rhsvoid = static_cast<const FunctorA&>(rhs);
      return fptr == rhsvoid.fptr;
    }
 private:

  F fptr;
};

/**
 * @brief convenience function for cTTimerMessageCBA<Arg, void>
 *
 */

template<class T, class Arg>
TFunctorA<T, Arg, void>* makeCallback(T* const obj, void (T::*mfptr)(Arg*))
{
  return new TFunctorA<T, Arg, void>(obj, mfptr);
}

/**
 * @brief convenience function for cTTimerMessageCBA<void, void>
 *
 */

template<class T>
TFunctorA<T, void, void>* makeCallback(T* const obj, void (T::*mfptr)(void))
{
  return new TFunctorA<T, void, void>(obj, mfptr);
}

#ifndef TIMERCONSTANTS_H
#include "TimerConstants.h"
#endif //TIMERCONSTANTS_H

#endif /* CTTIMERMESSAGECB_H */
