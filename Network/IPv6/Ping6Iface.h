// -*- C++ -*-
//
// Copyright Copyright (C) 2001 Johnny Lai
// Monash University, Melbourne, Australia
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
   @file Ping6Iface.h

   @brief Store all Application layer to Transport/Network layer interface
   message types.
*/

/**
   @class echo_int_info
   Structure for upper layers to send ping information to IP layer.
   Define another structure if custom data is to be sent across.
*/
struct echo_int_info
{
  echo_int_info()
    :custom_data("PING6_REQ"), _length(sizeof(custom_data)){};

    unsigned int length() const
    {
      return _length;
    }

  void setLength(unsigned int len)
    {
      _length = len;
    }

  short id;
  unsigned long seqNo;
  char* custom_data;
  int _length;
  unsigned char hopLimit;
  double sendingTime;
};

