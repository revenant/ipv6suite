//
// Copyright (C) 2001, 2002 CTIE, Monash University
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
 *  @file MLDv2Message.h
 *  @brief MLDv2 manage Query and Report Message
 *
 *  @author Wally Chen
 *
 *  @date    28/11/2002
 *
 */
#include <vector>
#include "ICMPv6Message.h"
#include "ipv6_addr.h"

class MLDv2Message: public ICMPv6Message
{
 public:
  MLDv2Message(const ICMPv6Type type, size_t size);
  virtual MLDv2Message& operator=(const MLDv2Message & rhs);
  MLDv2Message* duplicate() const
    { return new MLDv2Message(*this); }

  void setMaxRspCode(unsigned int maxDelay);  // Maximum Response Code
  unsigned int MaxRspCode();
  void setNoMAR(unsigned int num);    // Number of Multicast Address Record
  unsigned int NoMAR();
  void setMAR(char *src, int offset, int size);

  virtual MLDv2Message *dup()const
  {
    return new MLDv2Message(*this);
  }

  virtual const char* classname() const
  {
    return "MLDv2Message";
  }

// Function for General Query
  const ipv6_addr MA() const;
  const bool S() const;
  const char QRV() const;
  const char QQIC() const;
  const short int NS() const;
  void setMA(ipv6_addr _info);
  void setS_Flag(bool S);
  void setQRV(char QRVcode);
  void setQQIC(char _info);
  void setNS(short int _info);

// Function for Option Field Processing
  void setOpt(char* src, int offset, size_t size);
  void getOpt(char* dest, int offset, size_t size);
  const ipv6_addr getIPv6addr(int offset);
  int optLength();

 private:
  std::vector<char> _opt;
};
