// -*- C++ -*-
// Copyright (C) 2006 by Johnny Lai
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

   @brief Encapsulate timer messages and the behaviour that goes with them
   when expired.

   @author Johnny Lai
   @date 2.11.01

*/

#if !defined CTIMERMESSAGE_H
#define CTIMERMESSAGE_H

#ifndef CASSERT
#define CASSERT
#include <cassert>
#endif

#if !defined BOOST_NONCOPYABLE_HPP_INCLUDED
#include <boost/noncopyable.hpp>
#endif


#ifndef __CMESSAGE_H
#include <cmessage.h>
#endif 

#ifndef __CSIMPLEMODULE_H
#include "csimplemodule.h"
#endif

/**
   @class cTimerMessage
   @brief Base class to timer messages.

   Convenience handle to timer messages so that once received and identified via
   (cMessage::isSelfMessage) can invoke callFunc to handle expiry of message.

   @note Use OPP_Global::ContextSwitcher when appropiate, i.e. during creation
   of self timer message in another module or rescheduling of message that
   belongs to another module
*/
class cTimerMessage: public cMessage, boost::noncopyable
{
 public:
  virtual ~cTimerMessage()
    {
      //Remove these warnings by cancelling messages to be deleted
      //is currently in (cMessageHeap)simulation.scheduled-events, it cannot be
      //deleted.
      if (isScheduled())
      {
        cancel();
        printf("%s in %s at %f is cancelled in dtor \n", name(), mod->fullPath(), mod->simTime());
      }
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
  cTimerMessage(int message_id, const char* name = NULL):
    cMessage(name),
    mod(check_and_cast<cSimpleModule*> (simulation.contextModule()))
    {
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

#endif //CTIMERMESSAGE_H
