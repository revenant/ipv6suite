// -*- C++ -*-
// Copyright (C) 2006 Johnny Lai
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
 * @file cCallbackMessage.h
 * @author Johnny Lai
 * @date 24 Apr 2006
 *
 * @brief Definition of class cCallbackMessage
 *
 */

#ifndef CCALLBACKMESSAGE_H
#define CCALLBACKMESSAGE_H


#ifndef BOOST_FUNCTION_HPP
#define BOOST_FUNCTION_HPP
#include <boost/function.hpp>
#endif

#if !defined CTIMERMESSAGE_H
#include "cTimerMessage.h"
#endif //CTIMERMESSAGE_H

#if !defined IOSTREAM
#define IOSTREAM
#include <iostream>
#endif

typedef boost::function<void ()> Nullary;

/**
 * @class cCallbackMessage
 *
 * @brief Allows binding and rebinding of functions and args to messages
 *
 * Advantage of this over cSignalMessage is that can rebind with different args
 * or different functions while taking up 12 bytes less. However cSignalMessage
 * can invoke multiple callbacks at once with every connect call adding a new
 * observer.
 *
 * All functions can be changed into Nullary signature by using boost::bind and
 * boost::lambda placeholders to store the extra arguments. @see
 * MIPv6NDStateHost::MIPv6NDStateHost where L2 trigger is an example of such a
 * use
 */

class cCallbackMessage: public cTimerMessage, public Nullary
{
 public:
  //@name constructors, destructors and operators
  //@{
  cCallbackMessage(const char* name = NULL, int message_id = 0):
    //module is 0 as it is unused argument
    cTimerMessage(message_id, 0, name), Nullary()
  {}

  ///use Nullary's operator= to rebind new callback or args
  cCallbackMessage& operator=(const Nullary& f)
    {
      (Nullary&)(*this)=f;
      return *this;
    }
  //@}

  virtual void callFunc()
  {
    if (!*this)
      std::cerr<<"Oh oh no function so why the hell r we scheduled "<<name()<<endl;
    else
      (*this)();
  }

 protected:

 private:

};

#endif /* CCALLBACKMESSAGE_H */

