//// Copyright (C) 2001, 2002 CTIE, Monash University
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
 *  @file MLDv2Record.h
 *  @brief Manage requests to the routing table and the interface table
 *
 *  @author Wally Chen
 *
 *  @date    30/11/2004
 *
 */

#include <list>

#include <omnetpp.h>
#include "ipv6_addr.h"

namespace
{
  const int MLD_version = 2;
  const int MLDv2_QI = 125;        // Query Interval
  const int MLDv2_RV = 2;        // Robustness Variable
  const int MLDv2_QRI = 10;    // Query Response Interval
  const int MLDv2_MALI = MLDv2_RV * MLDv2_QI + MLDv2_QRI;    // Multicast Address Listening Interval
  const int MLDv2_LLQI = 1;    // Last Listener Query Interval
  const int MLDv2_LLQC = MLDv2_RV;    // Last Listener Query Count
  const int MLDv2_LLQT = MLDv2_LLQI * MLDv2_LLQC;    // Last Listener Query Time

  const char IS_IN = 1;
  const char IS_EX = 2;
  const char TO_IN = 3;
  const char TO_EX = 4;
  const char ALLOW = 5;
  const char BLOCK = 6;

  const int MLDv2_StartTime = 10;
  const int MLDv2_EndTime = 300;
  const int Group=5;
  const int Source=10;

  const unsigned int MLDv2_HEADER = 8;
  const unsigned int MLDv2_GQ_LEN = 20;
  const unsigned int MLDv2_MAR_HEADER = 20;

  const int LISTENER_RECORD = 1;  //XXX was = 0, but compiler said "illegal zero-sized array" below  --AV
}

typedef struct SARecord
{
  ipv6_addr SA;
  int SourceTimer;
  char countListener;
  ipv6_addr Listener[LISTENER_RECORD];
  struct SARecord *Next;
}SARecord_t;

typedef struct MARecord
{
  char type;            // Record Type
  char AuxDataLen;      // Auxiliary Data Length.
  short int NS;         // Number of Sources
  ipv6_addr MA;         // Multicast Address
  int FilterTimer;      // Multicast Address Listening Interval
  short int inNS;       // Number of INCLUDE sources
  short int exNS;       // Number of INCLUDE sources
  int inSize;
  int exSize;
  SARecord_t* inSAL;    // Source Address List
  SARecord_t* exSAL;    // Source Address List
  char* Auxiliary;      // Implementations of MLDv2 MUST NOT include any auxiliary data (i.e., MUST set the Aux DATA LEN field to zero) in any transmitted Multicast Address Record, and MUST ignore any such data present in any received Multicast Address Record.
  simtime_t tPrevious;
  struct MARecord *Next;
}MARecord_t;

class MLDv2Record
{
  public:
  MLDv2Record();

  public:
  short int NMAR();
  MARecord_t* MAR();
  SARecord_t* SrcAddrList(MARecord_t* ptrMAR);

  MARecord_t* searchMAR(ipv6_addr MA);
  bool addMA(ipv6_addr MA, char type);
  bool delMA(ipv6_addr MA);
  bool setMAtype(ipv6_addr MA, char type);
  void destroyMAL();

  SARecord_t* searchSA(ipv6_addr MA, ipv6_addr SA, char type);
  bool addSA(ipv6_addr MA, ipv6_addr SA, char type);
  bool delSA(ipv6_addr MA, ipv6_addr SA, char type);
  void destroySAL(MARecord_t* MAR, char type);

  void dumpMAL();                               // Multicast Address List
  void dumpMAR(ipv6_addr MA);                   // Multicast Address Record
  void dumpSAL(ipv6_addr MA);
  void dumpSAR(ipv6_addr MA, ipv6_addr SA, char type);
  int sizeofMAR(MARecord_t* ptrMAR, char type);
  int sizeofMAL();

  private:
  // MLDv2 Record
  short int _NMAR;      // Number of Multicast Address Record;
  MARecord_t* _MAR;     // tunable parameter
};
