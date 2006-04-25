//// Copyright (C) 2001, 2002 CTIE, Monash University
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

/**
 *  @file MLDv2Record.cc
 *  @brief Manage requests to the routing table and the interface table
 *
 *  @author Wally Chen
 *
 *  @date    30/11/2004
 *
 */
#include "MLDv2Record.h"

#include <stdlib.h>
#include <memory>
#include <string>
#include <cassert>
#include "math.h"

#include <boost/cast.hpp>


#include <cpacket.h>

#include "ipv6_addr.h"
#include "RoutingTable6.h"
#include "IPv6Datagram.h"
#include "opp_utils.h"
#include "IPv6CDS.h"

//#define    DEBUG_MSG

// MLDv2Recored.cc
MLDv2Record::MLDv2Record()
{
  _NMAR = 0;
  _MAR = NULL;
}

short int MLDv2Record::NMAR()
{
  return(_NMAR);
}

MARecord_t* MLDv2Record::MAR()
{
  return(_MAR);
}

SARecord_t* MLDv2Record::SrcAddrList(MARecord_t* ptrMAR)
{
  if(ptrMAR->type==IS_IN)
    return ptrMAR->inSAL;
  else if(ptrMAR->type==IS_EX)
    return ptrMAR->exSAL;

  return NULL;
}

MARecord_t* MLDv2Record::searchMAR(ipv6_addr MA)
{
  MARecord_t* ptrMAR=NULL;
  ptrMAR = _MAR;
  while(ptrMAR)
  {
    if(ptrMAR->MA==MA)
    {
     return(ptrMAR);
    }
    ptrMAR = ptrMAR->Next;
  }

  return(NULL);
}

bool MLDv2Record::addMA(ipv6_addr MA, char type)
{
  MARecord_t* ptrMAR=searchMAR(MA);

  if(ptrMAR==NULL)
  { // there is no such record before, create a new one.
    MARecord_t* tempMAR = new MARecord_t;
    _NMAR++;
    tempMAR->MA = MA;
    tempMAR->type = type;
    tempMAR->AuxDataLen = 0;
    tempMAR->inNS = 0;
    tempMAR->exNS = 0;
    tempMAR->inSAL = NULL;
    tempMAR->exSAL = NULL;
    tempMAR->Auxiliary = NULL;
    tempMAR->FilterTimer = MLDv2_MALI;
    tempMAR->inSize = MLDv2_MAR_HEADER;
    tempMAR->exSize = MLDv2_MAR_HEADER;
    tempMAR->Next = _MAR;
    _MAR = tempMAR;
#ifdef DEBUG_MSG
    cout << "[MLDv2]addMA(" << ipv6_addr_toString(MA) << "," << (int)type << ")" << endl;
#endif
    return(true);
  }
  return(false);
}

bool MLDv2Record::delMA(ipv6_addr MA)
{
  MARecord_t* tempMAR=NULL;
  MARecord_t* ptrMAR=_MAR;

  if(_MAR==NULL)
  {
    return(false);
  }
  else if(_MAR->MA==MA)
  {
    tempMAR = _MAR;
    _MAR = _MAR->Next;
    destroySAL(tempMAR,IS_IN);
    destroySAL(tempMAR,IS_EX);
    delete tempMAR;
    _NMAR--;
#ifdef DEBUG_MSG
    cout << "[MLDv2]delMA(" << ipv6_addr_toString(MA) << ")" << endl;
#endif
    return(true);
  }
  else
  {
    while(ptrMAR)
    {
      if(ptrMAR->Next==NULL)
      {
        return(false);
      }
      else if(ptrMAR->Next->MA==MA)
      {
       // 2: Normal Hit
        tempMAR = ptrMAR->Next;
        ptrMAR->Next = tempMAR->Next;
        destroySAL(tempMAR,IS_IN);
        destroySAL(tempMAR,IS_EX);
        delete tempMAR;
        _NMAR--;
#ifdef DEBUG_MSG
        cout << "[MLDv2]delMA(" << ipv6_addr_toString(MA) << ")" << endl;
#endif
        return(true);
      }
      else
      {
        ptrMAR = ptrMAR->Next;
      }
    }
  }
#ifdef DEBUG_MSG
  cout << "[MLDv2]delMA(" << ipv6_addr_toString(MA) << "), no match!" << endl;
#endif
  return(false);
}

bool MLDv2Record::setMAtype(ipv6_addr MA, char type)
{
  MARecord_t* ptrMAR=searchMAR(MA);

  if(ptrMAR==NULL)
  { // no such record
//    cout << "[MLDv2]setMAtype(" << ipv6_addr_toString(MA) << "), no match!" << endl;
    return(false);
  }
  else
  {
    ptrMAR->type = type;
  }
  return true;
}


void MLDv2Record::destroyMAL()
{
  MARecord_t* tempMAR=NULL;

#ifdef DEBUG_MSG
  cout << "[MLDv2]destroyMAL()" << endl;
#endif
  while(_MAR!=NULL)
  {
    destroySAL(_MAR,IS_IN);
    destroySAL(_MAR,IS_EX);
    tempMAR = _MAR;
    _MAR = _MAR->Next;
    delete tempMAR;
  }
}

SARecord_t* MLDv2Record::searchSA(ipv6_addr MA, ipv6_addr SA, char type)
{
  MARecord_t* ptrMAR=searchMAR(MA);

  if(ptrMAR==NULL)
  { // no such record
    return(NULL);
  }
  else
  {
    SARecord_t* ptrSA=NULL;
    if(type==IS_IN)
      ptrSA = ptrMAR->inSAL;
    else if(type==IS_EX)
      ptrSA = ptrMAR->exSAL;
    else
      return(NULL);

    while(ptrSA)
    {
      if(ptrSA->SA==SA)
      {
//        cout << "searchSA(" << ipv6_addr_toString(SA) << "): hit!" << endl;
        return(ptrSA);
      }
      ptrSA = ptrSA->Next;
    }
//    cout << "searchSA(" << ipv6_addr_toString(SA) << "): miss!" << endl;
    return(NULL);
  }
  return(NULL);
}

bool MLDv2Record::addSA(ipv6_addr MA, ipv6_addr SA, char type)
{
  MARecord_t* ptrMAR=searchMAR(MA);

  if(ptrMAR==NULL)
  { // no such record
#ifdef DEBUG_MSG
    cout << "[MLDv2]addSA(" << ipv6_addr_toString(MA) << "), no match!" << endl;
#endif
    return(false);
  }
  else
  {
    SARecord_t* ptrSA=searchSA(MA,SA,type);

    if(ptrSA==NULL)
    { // there is no such record before, create a new one.
      SARecord_t* tempSA = new SARecord_t;
      tempSA->SA = SA;
      if(type==IS_IN)
      {
        tempSA->Next = ptrMAR->inSAL;
        tempSA->SourceTimer = MLDv2_MALI;
        tempSA->countListener = 0;
        for(int i=0;i<LISTENER_RECORD;i++)
          tempSA->Listener[i] = IPv6_ADDR_UNSPECIFIED;
        ptrMAR->inSAL = tempSA;
        ptrMAR->inNS++;
        ptrMAR->inSize += 16;
#ifdef DEBUG_MSG
        cout << "[MLDv2]addSA(" << ipv6_addr_toString(SA) << ",INCLUDE)" << endl;
#endif
      }
      else if(type==IS_EX)
      {
        tempSA->Next = ptrMAR->exSAL;
        tempSA->SourceTimer = 0;
        tempSA->countListener = 0;
        for(int i=0;i<LISTENER_RECORD;i++)
          tempSA->Listener[i] = IPv6_ADDR_UNSPECIFIED;
        ptrMAR->exSAL = tempSA;
        ptrMAR->exNS++;
        ptrMAR->exSize += 16;
#ifdef DEBUG_MSG
        cout << "[MLDv2]addSA(" << ipv6_addr_toString(SA) << ",EXCLUDE)" << endl;
#endif
      }
      if(ptrMAR->type==IS_IN)
        ptrMAR->NS = ptrMAR->inNS;
      else if(ptrMAR->type==IS_EX)
        ptrMAR->NS = ptrMAR->exNS;
      return(true);
    }
    return(false);
  }
}

bool MLDv2Record::delSA(ipv6_addr MA, ipv6_addr SA, char type)
{
  MARecord_t* ptrMAR=searchMAR(MA);

  if(ptrMAR==NULL)
  { // no such record
#ifdef DEBUG_MSG
    cout << "[MLDv2]delSA(" << ipv6_addr_toString(MA) << ",SA), no match!" << endl;
#endif
    return(false);
  }
  else
  {
    SARecord_t* tempSA=NULL;
    SARecord_t* ptrSA=NULL;

    if(type==IS_IN)
      ptrSA = ptrMAR->inSAL;
    else if(type==IS_EX)
      ptrSA = ptrMAR->exSAL;

    if(ptrSA==NULL)
    {
#ifdef DEBUG_MSG
      cout << "[MLDv2]delSA(MA," << ipv6_addr_toString(SA) << "), no match!" << endl;
#endif
      return(false);
    }
    else if(ptrSA->SA==SA)
    {
      if(type==IS_IN)
      {
        tempSA = ptrMAR->inSAL;
        ptrMAR->inSAL = ptrMAR->inSAL->Next;
        delete tempSA;
        ptrMAR->inNS--;
        ptrMAR->inSize -= 16;
#ifdef DEBUG_MSG
        cout << "[MLDv2]delSAin(MA," << ipv6_addr_toString(SA) << "), at " << endl;
#endif
      }
      else if(type==IS_EX)
      {
        tempSA = ptrMAR->exSAL;
        ptrMAR->exSAL = ptrMAR->exSAL->Next;
        delete tempSA;
        ptrMAR->exNS--;
        ptrMAR->exSize -= 16;
#ifdef DEBUG_MSG
        cout << "[MLDv2]delSAex(MA," << ipv6_addr_toString(SA) << "), at " << endl;
#endif
      }
      if(ptrMAR->type==IS_IN)
        ptrMAR->NS = ptrMAR->inNS;
      else if(ptrMAR->type==IS_EX)
        ptrMAR->NS = ptrMAR->exNS;
      return(true);
    }
    else
    {
      while(ptrSA)
      {
        if(ptrSA->Next==NULL)
        {
          return(false);
        }
        else if(ptrSA->Next->SA==SA)
        {
         // 2: Normal Hit
          tempSA = ptrSA->Next;
          ptrSA->Next = tempSA->Next;
          delete tempSA;
          if(type==IS_IN)
          {
            ptrMAR->inNS--;
            ptrMAR->inSize -= 16;
#ifdef DEBUG_MSG
            cout << "[MLDv2]delSAin(MA," << ipv6_addr_toString(SA) << "), at " << endl;
#endif
          }
          if(type==IS_EX)
          {
            ptrMAR->exNS--;
            ptrMAR->exSize -= 16;
#ifdef DEBUG_MSG
            cout << "[MLDv2]delSAex(MA," << ipv6_addr_toString(SA) << "), at " << endl;
#endif
          }
          if(ptrMAR->type==IS_IN)
            ptrMAR->NS = ptrMAR->inNS;
          else if(ptrMAR->type==IS_EX)
            ptrMAR->NS = ptrMAR->exNS;
          return(true);
        }
        else
        {
          ptrSA = ptrSA->Next;
        }
      }
    }
#ifdef DEBUG_MSG
    cout << "[MLDv2]delSA(MA," << ipv6_addr_toString(SA) << "), no match!" << endl;
#endif
    return(false);
  }

}

void MLDv2Record::destroySAL(MARecord_t* MAR, char type)
{
  SARecord_t* tempSA=NULL;

  if(type==IS_IN)
  {
    while(MAR->inSAL!=NULL)
    {
      tempSA = MAR->inSAL;
      MAR->inSAL = tempSA->Next;
      delete tempSA;
    }
    MAR->inNS = 0;
    MAR->inSize = 20;
  }
  else if(type==IS_EX)
  {
    while(MAR->exSAL!=NULL)
    {
      tempSA = MAR->exSAL;
      MAR->exSAL = tempSA->Next;
      delete tempSA;
    }
    MAR->exNS = 0;
    MAR->exSize = 20;
  }
  MAR->NS = 0;
}

void MLDv2Record::dumpMAL()
{
  MARecord_t* ptrMAR=_MAR;
  cout << "[MLDv2]dumpMAL()===>" << endl;

  while(ptrMAR)
  {
    cout << "Multicast Address:" << ptrMAR->MA << endl;        // Multicast Address
    cout << "Number of inSource:" << ptrMAR->inNS << endl;        // Number of Sources
    cout << "Number of exSource:" << ptrMAR->exNS << endl;        // Number of Sources
    ptrMAR = ptrMAR->Next;
  }
  cout << "[MLDv2]dumpMAL()===>over" << endl;
}

void MLDv2Record::dumpMAR(ipv6_addr MA)
{
  MARecord_t* ptrMAR=searchMAR(MA);

  cout << "===== [MLDv2]dumpMAR(MA) =====" << endl;
  if(ptrMAR==NULL)
  {
    cout << "none" << endl;
  }
  else
  {
    cout << "type:" << ptrMAR->type << endl;            // Record Type
    cout << "AuxDataLen:" << ptrMAR->AuxDataLen << endl;        // Auxiliary Data Length.
    cout << "Number of Source:" << ptrMAR->NS << endl;        // Number of Sources
    cout << "Multicast Address:" << ptrMAR->MA << endl;        // Multicast Address
    cout << "Source Address List:" << ptrMAR->inSAL << endl;    // Source Address List
    cout << "Auxiliary:" << ptrMAR->Auxiliary << endl;        // Implementations of MLDv2 MUST NOT include any auxiliary data (i.e., MUST set the Aux DATA LEN field to zero) in any transmitted Multicast Address Record, and MUST ignore any such data present in any received Multicast Address Record.
    cout << "FilterTimer:" << ptrMAR->FilterTimer << endl;
    cout << "Next:" << ptrMAR->Next << endl;
  }
}

void MLDv2Record::dumpSAL(ipv6_addr MA)
{
  MARecord_t* ptrMAR=searchMAR(MA);
  SARecord_t* ptrSAR=ptrMAR->inSAL;

  cout << "===== [MLDv2]dumpSAL() =====" << endl;
  cout << "MA:" << ipv6_addr_toString(MA) << endl;        // Source Address
  cout << "inSAL=>" << endl;

  while(ptrSAR)
  {
    cout << "SA:" << ipv6_addr_toString(ptrSAR->SA) << endl;        // Source Address
    ptrSAR = ptrSAR->Next;
  }

  ptrSAR=ptrMAR->exSAL;
  cout << "exSAL=>" << endl;

  while(ptrSAR)
  {
    cout << "SA:" << ipv6_addr_toString(ptrSAR->SA) << endl;        // Source Address
    ptrSAR = ptrSAR->Next;
  }
}

void MLDv2Record::dumpSAR(ipv6_addr MA, ipv6_addr SA, char type)
{
  SARecord_t* ptrSAR=searchSA(MA,SA,type);

  cout << "===== [MLDv2]dumpSAR(" << ipv6_addr_toString(MA) << ipv6_addr_toString(SA) << ","") =====" << endl;
  if(ptrSAR==NULL)
  {
    cout << "none" << endl;
  }
  else
  {
    cout << "Source Address:" << ptrSAR->SA << endl;        // Source Address
    cout << "SourceTimer:" << ptrSAR->SourceTimer << endl;
    cout << "Next:" << ptrSAR->Next << endl;
  }
}


int MLDv2Record::sizeofMAR(MARecord_t* ptrMAR, char type)
{
  if(ptrMAR==NULL)
  {
    ptrMAR->inSize = 0;
    ptrMAR->exSize = 0;
  }
  else
  {
    ptrMAR->inSize = 20+ptrMAR->AuxDataLen+(ptrMAR->inNS*sizeof(ipv6_addr));
    ptrMAR->exSize = 20+ptrMAR->AuxDataLen+(ptrMAR->exNS*sizeof(ipv6_addr));
  }

  if(ptrMAR->type==IS_IN)
    return ptrMAR->inSize;
  else if(ptrMAR->type==IS_EX)
    return ptrMAR->exSize;

  assert(false);
  return -1;
}

int MLDv2Record::sizeofMAL()
{
  MARecord_t* ptrMAR=_MAR;
  int SumSize=0;

  while(ptrMAR)
  {
    if(ptrMAR->type==IS_IN)
    {
      ptrMAR->NS = ptrMAR->inNS;
      SumSize += ptrMAR->inSize;
    }
    else if(ptrMAR->type==IS_EX)
    {
      ptrMAR->NS = ptrMAR->exNS;
      SumSize += sizeofMAR(ptrMAR,IS_IN);
    }
    else
    {
      SumSize += ptrMAR->inSize;
//      cout << "ptrMAR->inSize:" << ptrMAR->inSize << endl;;
    }
    ptrMAR = ptrMAR->Next;
  }

  return SumSize;
}
