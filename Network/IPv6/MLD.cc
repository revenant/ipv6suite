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
 *  @file MLD.cc
 *  @brief Manage requests to the routing table and the interface table
 *
 *  @author Chainarong Kit-Opas
 *
 *  @date    28/11/2002
 *
 */

#include "MLD.h"

#include <stdlib.h>
#include <time.h>

#include <memory>
#include <string>
#include <cassert>
#include "math.h"

#include <boost/cast.hpp>


#include <cpacket.h>

#include "ipv6_addr.h"
#include "RoutingTable6.h"
#include "IPv6Datagram.h"
#include "IPv6InterfacePacket.h"
#include "opp_utils.h"
#include "MLDMessage.h"
#include "MLDv2Message.h"
#include "MLDIface.h"
#include "cTimerMessageCB.h"
#include "IPv6CDS.h"

typedef Loki::cTimerMessageCB<void, TYPELIST_1(ipv6_addr) > MLDListenIntTmr;

Define_Module( MLD );

void MLD::initialize()
{
  srand((unsigned)time(NULL));

  rt = static_cast<RoutingTable6*>(OPP_Global::findModuleByName(this,"routingTable6"));
  assert(rt!=0);
  reportCounter = 0;
  masqueryCounter = 0;
  genqueryCounter = 0;

/*  MLD_version = 2;

  MLDv2_QI = 125;
  MLDv2_RV = 2;
  MLDv2_QRI = 10;
  MLDv2_MALI = MLDv2_RV * MLDv2_QI + MLDv2_QRI;
  MLDv2_LLQI = 1;
  MLDv2_LLQC = MLDv2_RV;
  MLDv2_LLQT = MLDv2_LLQI * MLDv2_LLQC;*/
  CheckTimerInterval = 1;
  RouterType = IS_IN;

  LSTsize = 0;
  LStable = new MLDv2Record;
  MASSQtable = NULL;
  ReportTable = NULL;

// Create client1 MLD Record
  if(strncmp(OPP_Global::findNetNodeModule(this)->name(),"client",6)==0)
  {
    int i,j;
    ipv6_addr bufMA;
    ipv6_addr bufSA;
    char bufAddr[40];

//    Group = par("Group");
//    Source = par("Source");
/*
    // Generate constant Groups with constant Sources
    for(i=0;i<Group;i++)
    {
      sprintf(bufAddr,"ff3e:0:0:0:0:%d:0:1980",i+1);
      bufMA = c_ipv6_addr(bufAddr);
      rt->joinMulticastGroup(bufMA);
      LStable->addMA(bufMA,IS_IN);
      for(j=0;j<Source;j++)
      {
        sprintf(bufAddr,"fec0:0:0:0:0:%d:%d:1002",i+1,j+1);
        bufSA = c_ipv6_addr(bufAddr);
        LStable->addSA(bufMA,bufSA,IS_IN);
      }
    }
  */

    // Generate constant Groups with random Sources
    srand((unsigned)OPP_Global::findNetNodeModule(this));
    for(i=0;i<Group;i++)
    {
      if(((float)rand()/RAND_MAX)>((float)1/2))
      {
        sprintf(bufAddr,"ff3e:0:0:0:0:%d:0:1980",i+1);
        bufMA = c_ipv6_addr(bufAddr);
        rt->joinMulticastGroup(bufMA);
        LStable->addMA(bufMA,IS_IN);
        for(j=0;j<Source;j++)
        {
          if(((float)rand()/RAND_MAX)>((float)1/2))
          {
            sprintf(bufAddr,"fec0:0:0:0:0:%d:%d:1002",i+1,j+1);
            bufSA = c_ipv6_addr(bufAddr);
            LStable->addSA(bufMA,bufSA,IS_IN);
          }
        }
      }
    }
  }

  if(strcmp(OPP_Global::findNetNodeModule(this)->name(),"command")==0)
  {
    kbs = 20.0;
//      cout << "kbs:" << kbs << endl;
    SampleingRate = (double)1/16;
//      cout << "SampleingRate:" << SampleingRate << endl;
    AppDataSize = (int)(SampleingRate*kbs*1000/8);
//      cout << "AppDataSize:" << AppDataSize << endl;
//    scheduleAt(MLDv2_StartTime,new cMessage("SendDummy"));
  }

#if MLDV2
  scheduleAt(MLDv2_StartTime,new cMessage("Start"));
  scheduleAt(MLDv2_EndTime,new cMessage("End"));
#endif //MLDV2

}

void MLD::handleMessage(cMessage* msg)
{
  //static const string MLDIn("MLDIn");
  //sendDirect function from UDPVideoStreamCnt.cc when node starts/stop listening
  if(!msg->isSelfMessage())
  {
    if(MLD_version==2)
    {
        IPv6Datagram* dgram = boost::polymorphic_downcast<IPv6Datagram*>(msg);
//        cout << "extension length:" << dgram->extensionLength() << endl;
//        cout << "srcAddress:" << dgram->srcAddress() << endl;
//        cout << "destAddress:" << dgram->destAddress() << endl;

        MLDv2Message * mldmsg = boost::polymorphic_downcast<MLDv2Message *>(dgram->decapsulate());
//        cout << "optLength():" << mldmsg->optLength() << endl;

//    dumpMAL();

      if(!rt->isRouter())
      {
        if(mldmsg->type()==ICMPv6_MLD_QUERY)
        {
          cout << endl << "=====" << OPP_Global::findNetNodeModule(this)->name() << "=====" << endl;
          cout << "[MLDv2]Receive packet at:" << simTime() << endl;
          cout << "Type: " << mldmsg->type() << ", NS:" << mldmsg->NS() << endl;
          if(mldmsg->MA()==c_ipv6_addr(0))    // General Query
          {
            cout << "receive General Query" << endl;
            if(simTime()<300) // for block GQ after 20 senconds
            {
              simtime_t Delay = ((simtime_t)rand()/RAND_MAX)*(mldmsg->MaxRspCode()/1000);

//              cout << "Delay:" << Delay << endl;
              scheduleAt(simTime()+Delay,new cMessage("GQreport"));
            }
          }
          else if(mldmsg->NS()==0)    // Multicast Address Specific Query
          {
            cout << "receive Multicast Address Specific Query" << endl;
            cout << "MLDv2ReportMASQ()" << endl;
            MLDv2ReportMASQ();
          }
          else            // Multicast Address and Source Specific Query
          {
            cout << "receive Multicast Address and Source Specific Query" << endl;
            cout << "Multicast Address:" << ipv6_addr_toString(mldmsg->MA()) << endl;
            if(simTime()<300) // for block MASSQ after n senconds
            {
              simtime_t Delay = ((simtime_t)rand()/RAND_MAX)*(mldmsg->MaxRspCode()/1000);

//              cout << "Delay:" << Delay << endl;
              ProcessMASSQ(mldmsg);
              scheduleAt(simTime()+Delay,new cMessage("MLDv2ReportMASSQ"));
            }
          }
          }
        //MLDv2NodeParsing(dgram);
      }
      else if(mldmsg->type()==ICMPv6_MLDv2_REPORT)    // for router
      {
        cout << endl << "=====" << OPP_Global::findNetNodeModule(this)->name() << "=====" << endl;
        cout << "MLD length:" << mldmsg->length() << endl;
        cout << "[MLDv2]Receive packet" << ", Type: " << mldmsg->type() << ", at:" << simTime() << endl;
//        cout << "srcAddress:" << dgram->srcAddress() << endl;
        cout << "MLDv2RouterParsingMsg()" << endl;
        MLDv2RouterParsingMsg(mldmsg,dgram->srcAddress());
      }
      delete msg;
      return;
    }
    else if(MLD_version==1)
    {
      cout << "[MLDv1]processing packet" << endl;
      //From custom UDP application bad method of starting via sendDirect
      if(msg->arrivalGate()->name()== string("ReportMessage"))
      {

        sendReport(c_ipv6_addr(msg->name()));
        startReportTimer(c_ipv6_addr(msg->name()));
        delete msg;
        return;
      }
      else if(msg->arrivalGate()->name()== string("DoneMessage"))
      {
        sendDone(c_ipv6_addr(msg->name()));
        delete msg;
        return;
      }

      if(!rt->isRouter())
      {
        processNodeMLDMessages(msg);
      }
      else
      {
        processRouterMLDMessages(msg);
      }
      delete msg;
      return;
    }
  }

  LSTsize = LStable->sizeofMAL();

  if(msg->name() == string("Start"))
  {
    if (rt->isRouter())
    {
//      cout << OPP_Global::findNetNodeModule(this)->name() << "[MLDv2]Start, router processing" << endl;
      if(MLD_version==2)
      {
        MASSQtable = new MLDv2Record;
        scheduleAt(simTime()+MLDv2_QI,new cMessage("GeneralQuery"));
        scheduleAt(simTime()+CheckTimerInterval,new cMessage("CheckTimerExpire"));
        MLDv2GenQuery();
        //sendGenQuery();
      }
      else
      {
        scheduleAt(simTime()+MLDv2_QI,new cMessage("GeneralQuery"));
        sendGenQuery();
      }

      ipv6_addr temp = c_ipv6_addr("FF02:0:0:0:0:0:0:16");
      rt->joinMulticastGroup(c_ipv6_addr("FF02:0:0:0:0:0:0:16"));

//      sendGenQuery(static_cast<cTimerMessage*>(0));
//      tmrs.push_back(new cTTimerMessageS<MLD, void, MLD>(Tmr_SendGenQuery, this, this, &MLD::sendGenQuery, queryInt, false, simTime()+queryInt, "General Query"));
    }
    else
    {
      ReportTable = new MLDv2Record;
      if(strcmp(OPP_Global::findNetNodeModule(this)->name(),"command")==0)
      {
//        scheduleAt(25,new cMessage("Join"));
        scheduleAt(MLDv2_EndTime-1,new cMessage("Block"));
      }
//      simtime_t tempT = MLDv2_StartTime + ((simtime_t)rand()/RAND_MAX)*MLDv2_QI;
//      scheduleAt(tempT,new cMessage("Join"));
//      cout << "Join at " << tempT << endl;
//      tempT = MLDv2_StartTime + 10 + ((simtime_t)rand()/RAND_MAX)*MLDv2_QI;
//      scheduleAt(tempT,new cMessage("Block"));
//      cout << "Block at " << tempT << endl;
    }
  }
  else if(msg->name() == string("GeneralQuery"))
  {
    if (rt->isRouter())
    {
      if(MLD_version==2)
      {
        MLDv2GenQuery();
        scheduleAt(simTime()+MLDv2_QI,new cMessage("GeneralQuery"));
        //sendGenQuery();
      }
      else
      {
        scheduleAt(simTime()+MLDv2_QI,new cMessage("GeneralQuery"));
        sendGenQuery();
      }
    }
  }
  else if(msg->name() == string("SendDummy"))
  {
    SendDummy();
    scheduleAt(simTime()+SampleingRate,new cMessage("SendDummy"));
  }
  else if(msg->name() == string("Join"))
  {
//    SendJoin();
    SendRandJoin();
  }
  else if(msg->name() == string("Block"))
  {
//    SendBlock();
    SendAllBlock();
  }
  else if(msg->name() == string("CheckTimerExpire"))
  {
//    cout << "[self]CheckTimerExpire" << endl;
    scheduleAt(simTime()+CheckTimerInterval,new cMessage("CheckTimerExpire"));
    MLDv2CheckTimer();
  }
  else if(msg->name() == string("End"))
  {
    LStable->destroyMAL();
  }
  else if(LSTsize)
  {
    if(msg->name() == string("GQreport"))
    {
      if(LStable->MAR())
      {
//        cout << "[self]GQreport" << endl;
        MLDv2ReportGQ();
      }
    }
    else if(msg->name() == string("MLDv2ReportMASSQ"))
    {
      if(ReportTable->MAR())
      {
        cout << endl << "=====" << OPP_Global::findNetNodeModule(this)->name() << "=====" << endl;
//        cout << "[self]MLDv2ReportMASSQ" << endl;
        MLDv2ReportMASSQ();
      }
    }
    else if(msg->name() == string("QueueMASSQ"))
    {
      if(MASSQtable->MAR())
      {
        cout << endl << "=====" << OPP_Global::findNetNodeModule(this)->name() << "=====" << endl;
        cout << "[self]MLDv2QueueMASSQ(), at" << simTime() << endl;
        MLDv2QueueMASSQ();
      }
    }
  }
  else
  {
//    cout << "[self]else message" << endl;
//    (static_cast<cTimerMessage *> (msg))->callFunc();
  }
  delete msg;
}

void MLD::processNodeMLDMessages(cMessage* msg)

{
  // This is a node
  IPv6Datagram* dgram = boost::polymorphic_cast<IPv6Datagram*>(msg);
  MLDMessage* mldmsg = boost::polymorphic_cast<MLDMessage*>(dgram->encapsulatedMsg());

  switch(mldmsg->type())
  {
  case ICMPv6_MLD_QUERY:
  {
    if(mldmsg->showDelay()==lastlistenerInt)
    {
      masqueryCounter++;
      //this is MAS Query
      if(rt->localDeliver(mldmsg->showAddress()))
      {
        //usually a listener is required to send MAS immediately. No Timers required.
        sendReport(mldmsg->showAddress());
      }

    }
    else
    {
      //this is GEN Query
      genqueryCounter++;

      //must find all timers running and reset the timer if current value > new delay
      //#####################
      //add new find_reset_tmr function to specifically find running timer that's not IPv6_ADDR_UNSPECIFIED
      cout<<"\n NODE receives GEN Query @ "<<simTime()<<endl;
      cTimerMessage* runningTmr = resetTmr(Tmr_Report);

      if(runningTmr /*&&runningTmr->remainingTime()>mldmsg->showDelay()&&runningTmr->isScheduled()*/)
      {
        unsigned int rand_no = (unsigned int)abs((int)(normal( ( (mldmsg->showDelay())/2 ) , 4)));
        cout<<"random number gen= "<<rand_no<<endl;

        runningTmr->rescheduleDelay(rand_no);
        //runningTmr->rescheduleDelay(mldmsg->showDelay());

        cout<<"\n***"<<rt->nodeName()<<" reschedules timer due to GEN Query\n"<<endl;
      }
      //this program only find one running timer for multicast addr
    }
  }
  break;
  case ICMPv6_MLD_REPORT:
  {

    cout<<rt->nodeName()<<" receives MLDreport and looking to stop timer"<<endl;

    if(rt->localDeliver(mldmsg->showAddress()))
    {
      cTimerMessage* reportTmr = findTmr(Tmr_Report, mldmsg->showAddress());

      // node may be idle listener (no timer running) so if receive report it should stop timer

      // remove this functionality means that there will be duplicate report on the link.
      if(reportTmr&&reportTmr->isScheduled())
      {
        reportTmr->cancel(); // removed for MIPv6 testcase
        //need to leave trace of timer in case need to restart it
        //reportTmr->setOwner(0);
        //delete reportTmr;
        //tmrs.remove(reportTmr);
        cout<< "\n***"<<rt->nodeName()<<"NODE cancel timer\n"<<endl; // removed for MIPv6 testcase
      }
    }
  }
  default:
  assert(false);
  break;
  }
}

void MLD::processRouterMLDMessages(cMessage* msg)
{
  //this is a router

  IPv6Datagram* dgram = boost::polymorphic_downcast<IPv6Datagram*>(msg);
  MLDMessage* mldmsg = boost::polymorphic_downcast<MLDMessage*>(dgram->encapsulatedMsg());

  switch(mldmsg->type())
  {
  case ICMPv6_MLD_REPORT:
    cout<<rt->nodeName()<<" receives REPORT @ "<<simTime()<<"to " <<mldmsg->showAddress()<<"\n"<<endl;
    reportCounter++;

    //check if entry already in RtTable
    if (rt->cds->neighbour(mldmsg->showAddress()).lock())
    {
      cTimerMessage* mul_listener_Tmr = findTmr(Tmr_MulticastListenerInterval, mldmsg->showAddress());
      if(mul_listener_Tmr)
      {
        mul_listener_Tmr->cancel();
        mul_listener_Tmr->rescheduleDelay(mul_listener_Int);
        cout<<rt->nodeName()<<" reschedules delay querytmr \n"<<endl;
      }
    }
    else
      // add entry into RtTable
    {
      rt->cds->insertNeighbourEntry(new IPv6NeighbourDiscovery::NeighbourEntry(mldmsg->showAddress(), dgram->inputPort()));
      MLDListenIntTmr* mliTmr = new MLDListenIntTmr(Tmr_MulticastListenerInterval, this, this, &MLD::removeRtEntry, "MLD Listener Interval Timer");
      Loki::Field<0>(mliTmr->args) = mldmsg->showAddress();
      tmrs.push_back(mliTmr);
      //       tmrs.push_back(createTmrMsg(Tmr_MulticastListenerInterval, this, &MLD::removeRtEntry, new IPv6Address(mldmsg->showAddress()), simTime()+mul_listener_Int, true, "Mul Listener Interval Timer"));
      cout<<rt->nodeName()<<" adds entry into routing table at "<<simTime()<<"\n"<<endl;
    }
    break;
  case ICMPv6_MLD_DONE:
    cout<<rt->nodeName()<<" receives DONE @ "<<simTime() <<endl;

    if (rt->cds->neighbour(mldmsg->showAddress()).lock().get())
    {
      sendMASQuery(mldmsg->showAddress());

      cTimerMessage* masTmr = findTmr(Tmr_MulticastListenerInterval, mldmsg->showAddress());
      if(masTmr)
      {
        masTmr->cancel();
        masTmr->rescheduleDelay(lastlistenerInt);
      }
    }
    break;
    case ICMPv6_MLD_QUERY:
    //it is assumed that router do not receive MLD Query from any source
    break;
  default:
    cerr<<"Unknown message arrived "<<msg->name()<<" "<<msg->className()<<endl;
    assert(false);
    break;
  }
}

void MLD::startReportTimer(const ipv6_addr& addr, unsigned int responseInterval)
{
    //tmrs.push_back(createTmrMsg(Tmr_Report, this,  &MLD::sendReport, addr, simTime()+responseInterval, true, "Report message"));
    cout<<"In "<<rt->nodeName()<<" , a report timer created for "<<addr<<endl;
}

void MLD::removeRtEntry(const ipv6_addr& addr)
{
//I think it is unnecessary to put timer in tmr list if we only use it once. Other than to delete it. But easier if we just pass timer as arg of function anyway.

  //find and remove entry off the routing table
  cTimerMessage* mul_listener_Tmr = findTmr(Tmr_MulticastListenerInterval, addr);
  assert(mul_listener_Tmr);
  if(mul_listener_Tmr)
  {
    if (mul_listener_Tmr->isScheduled())
      mul_listener_Tmr->cancel();
    delete mul_listener_Tmr;
    tmrs.remove(mul_listener_Tmr);
    cout<<rt->nodeName()<<" removes Routing Entry at "<<simTime()<<"\n"<<endl;
  }
  rt->cds->removeNeighbourEntry(addr);
}


void MLD::sendOwnReportMsg(const ipv6_addr& addr)
{
  cMessage * msg = new cMessage(ipv6_addr_toString(addr).c_str());
  msg->setKind(131);
  scheduleAt(simTime(), msg);
}

void MLD::sendReport(const ipv6_addr& addr)
{
  //only called by timer expired(callback fn) or when just started listening. thus no timer needed reschedule nor started
  MLDMessage* mldmsg = new MLDMessage(ICMPv6_MLD_REPORT);
  mldmsg->setDelay(0);//query response interval
  mldmsg->setAddress(addr);

  // sendInterfacePacket(mldmsg, c_ipv6_addr(ALL_ROUTERS_LINK_ADDRESS),IPv6_ADDR_UNSPECIFIED,1);

  sendInterfacePacket(mldmsg, addr, IPv6_ADDR_UNSPECIFIED,1);

  reportCounter++;
  cout<<"####REPORT funct called @ "<<simTime()<<endl;
  //report msg should have addr as its header file but it doesn't work in this so use ALL Router link add for now
  //NOPE that was fixed.temporarily in localDeliver() in RoutingTable6.cc
}

void MLD::sendDone(const ipv6_addr& addr)
{
  MLDMessage* mldmsg = new MLDMessage(ICMPv6_MLD_DONE);
  mldmsg->setDelay(0);//query response interval
  mldmsg->setAddress(addr);
  sendInterfacePacket(mldmsg, c_ipv6_addr(ALL_ROUTERS_LINK_ADDRESS) ,IPv6_ADDR_UNSPECIFIED,1);
}

void MLD::finish()
{
  delete LStable;
  LStable = 0;
  recordScalar("MLDReport", reportCounter);
  recordScalar("MASQuery", masqueryCounter);
  recordScalar("GenQuery on LinkA", genqueryCounter);
}


//void MLD::sendGenQuery(cTimerMessage* queryTmr)
void MLD::sendGenQuery()
{
  cout << "[MLDv2]sendGenQuery(), at simTime:" << simTime() << endl;
/*  cout << "[MLD] isRouter, " << queryTmr << ", at simTime:" << simTime() << endl;
  if (!queryTmr)
  {
    queryTmr = new Loki::cTimerMessageCB<void, TYPELIST_1(cTimerMessage*) >(Tmr_SendGenQuery, this, this, &MLD::sendGenQuery, "General Query");
    tmrs.push_back(queryTmr);
  }
  else if (queryTmr->isScheduled())
    queryTmr->cancel();
  queryTmr->rescheduleDelay(queryInt);
*/
  MLDMessage* mldmsg = new MLDMessage(ICMPv6_MLD_QUERY);
/*  mldmsg->setDelay(responseInt);//query response interval
  mldmsg->setAddress(IPv6_ADDR_UNSPECIFIED);
  mldmsg->setOptInfo(MAX_RESPONES_CODE);
  mldmsg->setS_Flag(false);
  mldmsg->setQRV(2);
  mldmsg->setQQIC(125);
  mldmsg->setNS(0);
    cout << "Type: " << mldmsg->type() << endl;
    cout << "QRV:" << mldmsg->QRV() << endl;
    cout << "QQIC:" << mldmsg->QQIC() << endl;
    cout << "NS:" << mldmsg->NS() << endl;
*/
  sendInterfacePacket(mldmsg, c_ipv6_addr(ALL_NODES_LINK_ADDRESS),IPv6_ADDR_UNSPECIFIED,1);
}

void MLD::sendMASQuery(const ipv6_addr& addr)
{
  MLDMessage* mldmsg = new MLDMessage(ICMPv6_MLD_QUERY);
  mldmsg->setDelay(lastlistenerInt);//query response interval
  mldmsg->setAddress(addr);//general query multicast_addr field
//  mldmsg->setS_Flag(false);
//  mldmsg->setQRV(2);
//  mldmsg->setQQIC(125);
//  mldmsg->setNS(0);

  sendInterfacePacket(mldmsg, addr,IPv6_ADDR_UNSPECIFIED,1);
}

void MLD::sendInterfacePacket(MLDMessage *msg, const ipv6_addr& dest, const ipv6_addr& src, size_t hopLimit)
{
//  cout << "[MLD]sendInterfacePacket =>" << endl;
//  cout << "showAddress:" << msg->showAddress() << endl;
  IPv6InterfacePacket* interfacePacket = new IPv6InterfacePacket;
//  IPv6Datagram* dgram = new IPv6Datagram;

  assert(dest != IPv6_ADDR_UNSPECIFIED);

  interfacePacket->encapsulate(msg);
  interfacePacket->setDestAddr(dest);
  interfacePacket->setSrcAddr(src);
  interfacePacket->setProtocol(IP_PROT_IPv6_ICMP);

  if(hopLimit != 0)
    interfacePacket->setTimeToLive(hopLimit);
  send(interfacePacket, "MLDOut");
}

cTimerMessage* MLD::findTmr(int MLDTimerType, const ipv6_addr& multicast_addr)
{
  for(TimerMsgs::iterator it = tmrs.begin(); it!=tmrs.end(); it++)
  {

    if((*it)->kind() == MLDTimerType)
    {

      switch(MLDTimerType)
      {

        case Tmr_SendGenQuery:
          return *it;
          break;
        case Tmr_Report:
          if ((*(boost::polymorphic_downcast<ReportTmr*>(*it))->arg()) == multicast_addr)
            return *it;
          continue;
          break;
        case Tmr_MulticastListenerInterval:
          if ((*(boost::polymorphic_downcast<ReportTmr*>(*it))->arg()) == multicast_addr)
            return *it;
          break;

        default:
          assert(false);
          break;
      }
    }
  }
  return (cTimerMessage*)0;
}

cTimerMessage* MLD::resetTmr(int MLDTimerType)
{
  for(TimerMsgs::iterator it = tmrs.begin(); it!=tmrs.end(); it++)
  {

    if((*it)->kind() == MLDTimerType)
    {
      if ((*(boost::polymorphic_downcast<ReportTmr*>(*it))->arg()) != IPv6_ADDR_UNSPECIFIED)
      {
        return *it;
        break;
      }
    }
  }
  return (cTimerMessage*)0;
}
// ==================== MLDv2 function by Wally ====================

void MLD::MLDv2GenQuery()
{
  MLDv2Message *GQmsg= new MLDv2Message(ICMPv6_MLD_QUERY,MLDv2_GQ_LEN);

  cout << endl << OPP_Global::findNetNodeModule(this)->name() << " send MLDv2GenQuery(), at simTime:" << simTime() << endl;
  cout << "_NMAR:" << LStable->NMAR() << endl;
  LStable->dumpMAL();

  GQmsg->setLength(MLDv2_HEADER+MLDv2_GQ_LEN);

  GQmsg->setMaxRspCode(MAX_RESPONES_CODE);//query response interval
  GQmsg->setMA(c_ipv6_addr(0));
  GQmsg->setS_Flag(false);
  GQmsg->setQRV(robustVar);
  GQmsg->setQQIC(queryInt);
  GQmsg->setNS(0);

  MLDv2sendIPdgram(GQmsg, c_ipv6_addr(ALL_NODES_LINK_ADDRESS),IPv6_ADDR_UNSPECIFIED,1);
}

void MLD::MLDv2MASQ()
{
  cout << endl << "MLDv2MASQ(), at simTime:" << simTime() << endl;
}

void MLD::MLDv2MASSQ(ipv6_addr MA, SARecord_t* headSAR)
{
  SARecord_t* ptrSAR=headSAR;
  int count=0;

  while(ptrSAR)
  {
    count++;
    ptrSAR = ptrSAR->Next;
  }

  MLDv2Message *MASSQmsg= new MLDv2Message(ICMPv6_MLD_QUERY,MLDv2_GQ_LEN+(sizeof(ipv6_addr)*count));

  MASSQmsg->setLength(MLDv2_HEADER+MLDv2_GQ_LEN+(sizeof(ipv6_addr)*count));

  MASSQmsg->setMaxRspCode(LLQI_RESPONES_CODE);//query response interval
  MASSQmsg->setMA(MA);
  MASSQmsg->setS_Flag(false);
  MASSQmsg->setQRV(MLDv2_RV);
  MASSQmsg->setQQIC(MLDv2_LLQI);
  MASSQmsg->setNS(count);

  cout << "MLDv2MASSQ()==>, NS:" << MASSQmsg->NS() << ", at " << simTime() << endl;

  int offset = MLDv2_GQ_LEN;

  ptrSAR = headSAR;
  while(ptrSAR)
  {
    MASSQmsg->setOpt((char*)&ptrSAR->SA,offset,sizeof(ipv6_addr));
    offset += sizeof(ipv6_addr);
//    cout << ipv6_addr_toString(ptrSAR->SA) << endl;
    ptrSAR = ptrSAR->Next;
  }

  // notice! destination should be Multicast Address exactly!
  MLDv2sendIPdgram(MASSQmsg, c_ipv6_addr(ALL_NODES_LINK_ADDRESS),IPv6_ADDR_UNSPECIFIED,1);
}

void MLD::MLDv2ReportGQ()
{
  int PayloadSize=LStable->sizeofMAL();
  MLDv2Message *RptGQmsg = new MLDv2Message(ICMPv6_MLDv2_REPORT,PayloadSize);
  int offset=0;
  MARecord_t* ptrMAR=LStable->MAR();
  SARecord_t* ptrSAR=NULL;

  cout << endl << "MLDv2ReportGQ(), at simTime:" << simTime() << endl;
  cout << "rptMsg size: " << MLDv2_HEADER+PayloadSize << endl;
  RptGQmsg->setLength(MLDv2_HEADER+PayloadSize);
  RptGQmsg->setNoMAR(LStable->NMAR());

  while(ptrMAR)
  {
    RptGQmsg->setOpt((char*)ptrMAR,offset,MLDv2_MAR_HEADER);
    cout << " NS:" << ptrMAR->NS;
    offset += MLDv2_MAR_HEADER;

    if(ptrMAR->type==IS_IN)
      ptrSAR = ptrMAR->inSAL;
    else if(ptrMAR->type==IS_EX)
      ptrSAR = ptrMAR->exSAL;

    while(ptrSAR)
    {
      RptGQmsg->setOpt((char*)&ptrSAR->SA,offset,sizeof(ipv6_addr));
      offset += sizeof(ipv6_addr);
      ptrSAR = ptrSAR->Next;
    }

    ptrMAR = ptrMAR->Next;
  }
  cout << endl;
  MLDv2sendIPdgram(RptGQmsg, c_ipv6_addr("FF02:0:0:0:0:0:0:16"),IPv6_ADDR_UNSPECIFIED,1);
}

void MLD::MLDv2ReportMASQ()
{
}

void MLD::ProcessMASSQ(MLDv2Message* mldmsg)
{
  int NScount=mldmsg->NS();
  ipv6_addr McastAddr=mldmsg->MA();
  ipv6_addr tempAddr;
  int SrcOffset = MLDv2_GQ_LEN;
  SARecord_t* tempSAR=NULL;
  int i;

  // count NS and duplicate Source List in tempMsg
  if(LStable->searchMAR(McastAddr))
  {
    ReportTable->addMA(McastAddr,IS_IN);
    for(i=0;i<NScount;i++)
    {
      mldmsg->getOpt((char*)&tempAddr,SrcOffset,sizeof(ipv6_addr));
      SrcOffset += sizeof(ipv6_addr);
      tempSAR=LStable->searchSA(McastAddr,tempAddr,IS_IN);
      if(tempSAR)
      {
        ReportTable->addSA(McastAddr,tempAddr,IS_IN);
      }
    }
  }
}

void MLD::MLDv2ReportMASSQ()
{
  int PayloadSize=ReportTable->sizeofMAL();
  MLDv2Message *RptMASSQ = new MLDv2Message(ICMPv6_MLDv2_REPORT,PayloadSize);
  int offset=0;
  short int MARcount=0;
  MARecord_t* ptrMAR=ReportTable->MAR();
  SARecord_t* ptrSAR=NULL;

  cout << "MLDv2ReportMASSQ(), at simTime:" << simTime() << endl;
  cout << "rptMsg size: " << MLDv2_HEADER+PayloadSize;
  RptMASSQ->setLength(MLDv2_HEADER+PayloadSize);
  RptMASSQ->setNoMAR(ReportTable->NMAR());

  while(ptrMAR)
  {
    RptMASSQ->setOpt((char*)ptrMAR,offset,MLDv2_MAR_HEADER);
    cout << ", NS:" << ptrMAR->NS;
    offset += MLDv2_MAR_HEADER;

    if(ptrMAR->type==IS_IN)
      ptrSAR = ptrMAR->inSAL;
    else if(ptrMAR->type==IS_EX)
      ptrSAR = ptrMAR->exSAL;

    while(ptrSAR)
    {
      RptMASSQ->setOpt((char*)&ptrSAR->SA,offset,sizeof(ipv6_addr));
      offset += sizeof(ipv6_addr);
      ptrSAR = ptrSAR->Next;
    }
    MARcount++;
    ptrMAR = ptrMAR->Next;
  }
  cout << "MARcount:" << MARcount << endl;
  RptMASSQ->setNoMAR(MARcount);
  ReportTable->destroyMAL();
  MLDv2sendIPdgram(RptMASSQ, c_ipv6_addr("FF02:0:0:0:0:0:0:16"),IPv6_ADDR_UNSPECIFIED,1);
}

void MLD::MLDv2ChangeReport(MLDv2Record* headChange)
{
  MARecord_t* ptrMAR=headChange->MAR();
  short int count=headChange->NMAR();
  int Size=headChange->sizeofMAL();

  MLDv2Message *ChangeMsg= new MLDv2Message(ICMPv6_MLDv2_REPORT,MLDv2_HEADER+Size);

  ChangeMsg->setLength(MLDv2_HEADER+Size);

  // set Header of Multicast Listener Record
  ChangeMsg->setNoMAR(count);
  cout << "MLD::MLDv2ChangeReport(), Size:" << Size << endl;

  int offset=0;
  SARecord_t* ptrSAR=NULL;

  while(ptrMAR)
  {
    count = 0;
    ptrSAR = ptrMAR->inSAL;
    while(ptrSAR)
    {
      count++;
      ptrSAR = ptrSAR->Next;
    }
//    cout << "count:" << count << endl;
    cout << "MA:" << ipv6_addr_toString(ptrMAR->MA) << ", type:" << (int)ptrMAR->type << ", NS:" << count << endl;
    ChangeMsg->setOpt((char*)&ptrMAR->type,offset,sizeof(char));
    offset += (sizeof(char)*2);    // Record Type + Aux Data Len
    ChangeMsg->setOpt((char*)&count,offset,sizeof(short int));
    offset += sizeof(short int);
    ChangeMsg->setOpt((char*)&ptrMAR->MA,offset,sizeof(ipv6_addr));
    offset += sizeof(ipv6_addr);

    ptrSAR = ptrMAR->inSAL;    // This is no related to Record Type
    while(ptrSAR)
    {
      ChangeMsg->setOpt((char*)&ptrSAR->SA,offset,sizeof(ipv6_addr));
      offset += sizeof(ipv6_addr);
//      cout << "SA:" << ipv6_addr_toString(ptrSAR->SA) << endl;
      ptrSAR = ptrSAR->Next;
    }
    ptrMAR = ptrMAR->Next;
  }
  MLDv2sendIPdgram(ChangeMsg, c_ipv6_addr(ALL_NODES_LINK_ADDRESS),IPv6_ADDR_UNSPECIFIED,1);
}

void MLD::MLDv2sendIPdgram(MLDv2Message *msg, const ipv6_addr& dest, const ipv6_addr& src, size_t hopLimit)
{
  IPv6Datagram* dgram = new IPv6Datagram;

  assert(dest != IPv6_ADDR_UNSPECIFIED);

  dgram->encapsulate(msg);
  dgram->setPayloadLength(msg->length());
  dgram->setDestAddress(dest);
  dgram->setSrcAddress(src);
  dgram->setTransportProtocol(IP_PROT_IPv6_ICMP);

  if(hopLimit != 0)
    dgram->setHopLimit(hopLimit);

//  cout << "payloadLength:" << dgram->payloadLength() << endl;
//  cout << "hopLimit:" << dgram->hopLimit() << endl;
//  cout << "transportProtocol:" << dgram->transportProtocol() << endl;
//  cout << "srcAddr:" << dgram->srcAddress() << endl;
//  cout << "destAddr:" << dgram->destAddress() << endl;

  send(dgram, "MLDOut");
}

/*
void MLD::MLDv2NodeParsing(cMessage* Qmsg)
{
  IPv6Datagram* dgram = boost::polymorphic_downcast<IPv6Datagram*>(Qmsg);
    cout << endl << "=====" << OPP_Global::findNetNodeModule(this)->name() << "=====" << endl;
        cout << "[MLDv2]Receive packet at:" << simTime() << endl;
        cout << "payload length:" << dgram->payloadLength() << endl;
        cout << "extension length:" << dgram->extensionLength() << endl;
        cout << "totalLength:" << dgram->totalLength() << endl;
        cout << "transportProtocol:" << dgram->transportProtocol() << endl;
        cout << "srcAddress:" << dgram->srcAddress() << endl;
        cout << "destAddress:" << dgram->destAddress() << endl;

  MLDv2Message * mldmsg = boost::polymorphic_downcast<MLDv2Message *>(dgram->decapsulate());
  cout << "[MLDv2]NodeParsingMsg() ==>" << endl;
  if(mldmsg->type()!=130)
  {
    cout << "[MLDv2]NodeParsingMsg() ==> Not Query Message, Dropping" << endl;
  }
  else
  {
    cout << "[MLDv2]NodeParsingMsg() ==> ICMP type:" << mldmsg->type() << endl;
    if(mldmsg->MA()==0)
    { // General Query

//      IPv6Datagram *dgram = new IPv6Datagram;
    }
    else if(mldmsg->NS()==0)
    { // Multicast Address Specific Query
    }
    else if(mldmsg->NS()==0)
    { // Multicast Address Specific Query
    }
  }

}*/

void MLD::MLDv2RouterParsingMsg(MLDv2Message* mldmsg,ipv6_addr SrcAddr)
{
  if(mldmsg->type()==ICMPv6_MLDv2_REPORT)
  {
    cout << "NoMAR():" << mldmsg->NoMAR() << endl;
    int offset=0;
    MARecord_t bufMAR;
    MARecord_t* ptrMAR=NULL;
    MARecord_t* tempMASSO=NULL;
    SARecord_t* ptrSAR=NULL;
    SARecord_t* MASSQ_SAR=NULL;
    ipv6_addr tempAddr;
    int i,j,k;

    for(i=0;i<(int)mldmsg->NoMAR();i++)
    {
      // get Header of Multicast Address Record
      mldmsg->getOpt((char*)&bufMAR,offset,MLDv2_MAR_HEADER);
      offset += MLDv2_MAR_HEADER;
      cout << "MA:" << ipv6_addr_toString(bufMAR.MA) << endl;
      cout << "bufMAR.NS:" << bufMAR.NS << ", type:" << (int)bufMAR.type << endl;
      ptrMAR = LStable->searchMAR(bufMAR.MA);
      if(ptrMAR)
      {
        cout << "prtMAR->type:" << (int)ptrMAR->type << ", MA:" << ipv6_addr_toString(ptrMAR->MA) << endl;
//      MASSQtable->dumpMAL();
        if(ptrMAR->type == IS_IN)
        {
          switch(bufMAR.type)
          {
            case IS_IN:
            case ALLOW:
              cout << "IS_IN, ALLOW message" << endl;
              for(j=0;j<bufMAR.NS;j++)
              {
                mldmsg->getOpt((char*)&tempAddr,offset,sizeof(ipv6_addr));
                offset += sizeof(ipv6_addr);
                ptrSAR = LStable->searchSA(bufMAR.MA,tempAddr,IS_IN);
                if(ptrSAR)
                {
                  if(MASSQtable->searchSA(bufMAR.MA,tempAddr,IS_IN))
                  {
                    MASSQtable->delSA(bufMAR.MA,tempAddr,IS_IN);
                    tempMASSO=MASSQtable->searchMAR(bufMAR.MA);
                    if(!tempMASSO->inNS)
                    {
                      MASSQtable->delMA(bufMAR.MA);
                    }
                  }
                  ptrSAR->SourceTimer = MLDv2_MALI;
                  if(ptrSAR->countListener<LISTENER_RECORD)
                  {
                    for(k=0;k<LISTENER_RECORD;k++)
                    {
                      if(ptrSAR->Listener[k]==IPv6_ADDR_UNSPECIFIED)
                      {
                        ptrSAR->Listener[k] = SrcAddr;
                        ptrSAR->countListener++;
                        k = LISTENER_RECORD;
                      }
                    }
                  }
                }
                else
                {
                  LStable->addSA(bufMAR.MA,tempAddr,IS_IN);
                  ptrSAR = LStable->searchSA(bufMAR.MA,tempAddr,IS_IN);
                  if(ptrSAR->countListener<LISTENER_RECORD)
                  {
                    for(k=0;k<LISTENER_RECORD;k++)
                    {
                      if(ptrSAR->Listener[k]==IPv6_ADDR_UNSPECIFIED)
                      {
                        ptrSAR->Listener[k] = SrcAddr;
                        ptrSAR->countListener++;
                        k = LISTENER_RECORD;
                      }
                    }
                  }
                }
              }
              break;

            case IS_EX:
              break;

            case BLOCK:
           {
              bool bBlock=false;

              cout << "BLOCK message" << endl;

              for(j=0;j<bufMAR.NS;j++)
              {
                mldmsg->getOpt((char*)&tempAddr,offset,sizeof(ipv6_addr));
//                cout << "SA:" << ipv6_addr_toString(tempAddr) << endl;
                offset += sizeof(ipv6_addr);
                ptrSAR = LStable->searchSA(bufMAR.MA,tempAddr,IS_IN);
                if(ptrSAR)
                {
//                  cout << "Listener List, count:" << (int)ptrSAR->countListener << endl;
                  // maintain Listener List first
                  for(k=0;k<LISTENER_RECORD;k++)
                  {
                    if(ptrSAR->Listener[k]==SrcAddr)
                    {
                      ptrSAR->Listener[k] = IPv6_ADDR_UNSPECIFIED;
                      ptrSAR->countListener--;
                      k = LISTENER_RECORD;
                    }
                  }

                  if(ptrSAR->countListener==0)
                  {
                    if(MASSQtable->searchMAR(bufMAR.MA)==NULL)
                      MASSQtable->addMA(bufMAR.MA,BLOCK);
                    ptrSAR->SourceTimer = MLDv2_LLQT-1;
                    if(MASSQtable->addSA(bufMAR.MA,ptrSAR->SA,IS_IN))
                    {
                      bBlock = true;
                    }
                  }
                }
              }
              if(bBlock)
              {
//              MASSQtable->dumpSAL(bufMAR.MA);
                tempMASSO = MASSQtable->searchMAR(bufMAR.MA);
                tempMASSO->tPrevious = simTime();
                MASSQ_SAR = tempMASSO->inSAL;
                MLDv2MASSQ(bufMAR.MA,MASSQ_SAR);
                scheduleAt(simTime()+MLDv2_LLQI,new cMessage("QueueMASSQ"));
              }
            }
            break;

            case TO_IN:
              break;

            case TO_EX:
              break;

            default:
              break;
          }
        }
        else if(ptrMAR->type == IS_EX)
        {
          switch(bufMAR.type)
          {
            case IS_IN:
            case ALLOW:
              for(j=0;j<bufMAR.NS;j++)
              {
                mldmsg->getOpt((char*)&tempAddr,offset,sizeof(ipv6_addr));
                offset += sizeof(ipv6_addr);
                ptrSAR = LStable->searchSA(bufMAR.MA,tempAddr,IS_EX);
                if(ptrSAR)
                {
                  LStable->delSA(bufMAR.MA,tempAddr,IS_EX);
                }
                ptrSAR = LStable->searchSA(bufMAR.MA,tempAddr,IS_IN);
                if(ptrSAR==NULL)
                {
                  LStable->addSA(bufMAR.MA,tempAddr,IS_IN);
                  ptrSAR = LStable->searchSA(bufMAR.MA,tempAddr,IS_IN);
                  if(ptrSAR->countListener<LISTENER_RECORD)
                  {
                    for(k=0;k<LISTENER_RECORD;k++)
                    {
                      if(ptrSAR->Listener[k]==IPv6_ADDR_UNSPECIFIED)
                      {
                        ptrSAR->Listener[k] = SrcAddr;
                        ptrSAR->countListener++;
                        k = LISTENER_RECORD;
                      }
                    }
                  }
                }
              }
              break;

            case IS_EX:
              ptrMAR->FilterTimer = MLDv2_MALI;
              break;

            case BLOCK:
              break;

            case TO_IN:
              break;

            case TO_EX:
              ptrMAR->FilterTimer = MLDv2_MALI;
              break;

            default:
              break;
          }
        }
        else
        { // Invalid Record type, destroy all record
        }
      }
      else
      {
        LStable->addMA(bufMAR.MA,bufMAR.type);
        if(bufMAR.type == IS_IN)
        {
          switch(bufMAR.type)
          {
            case IS_IN:
            case ALLOW:
              for(j=0;j<bufMAR.NS;j++)
              {
                mldmsg->getOpt((char*)&tempAddr,offset,sizeof(ipv6_addr));
                offset += sizeof(ipv6_addr);
                LStable->addSA(bufMAR.MA,tempAddr,IS_IN);
                ptrSAR = LStable->searchSA(bufMAR.MA,tempAddr,IS_IN);
                if(ptrSAR->countListener<LISTENER_RECORD)
                {
                  for(k=0;k<LISTENER_RECORD;k++)
                  {
                    if(ptrSAR->Listener[k]==IPv6_ADDR_UNSPECIFIED)
                    {
                      ptrSAR->Listener[k] = SrcAddr;
                      ptrSAR->countListener++;
                      k = LISTENER_RECORD;
                    }
                  }
                }
              }
              break;

            case IS_EX:
              break;

            case BLOCK:
              break;

            case TO_IN:
              break;

            case TO_EX:
              break;

            default:
              break;
          }
        }
        else if(bufMAR.type == IS_EX)
        {
          switch(bufMAR.type)
          {
            case IS_IN:
              break;

            case ALLOW:
              break;

            case IS_EX:
              break;

            case BLOCK:
              break;

            case TO_IN:
              break;

            case TO_EX:
              break;

            default:
              break;
          }
        }
      }
    }
  }
}

void MLD::MLDv2CheckTimer()
{
  MARecord_t* ptrMAR=LStable->MAR();
  MARecord_t* tempMAR=NULL;
  SARecord_t* ptrSAR=NULL;
  SARecord_t* tempSAR=NULL;
  bool bMAdelete=false;
  bool bSAdelete=false;

//  cout << endl << "MLDv2CheckTimer(), at " << simTime() << endl;
  while(ptrMAR)
  {
    bMAdelete = false;
    ptrSAR = ptrMAR->inSAL;
    while(ptrSAR)
    {
      if(ptrSAR->SourceTimer)
      {
        ptrSAR->SourceTimer--;
      }
      else
      { // LLQT timer exhaust
        if(ptrMAR->type == IS_IN)
        { // delete the source record
          bSAdelete = true;
          tempSAR = ptrSAR->Next;
          if(MASSQtable)
          {
            MASSQtable->delSA(ptrMAR->MA,ptrSAR->SA,IS_IN);
          }
          LStable->delSA(ptrMAR->MA,ptrSAR->SA,IS_IN);
        }
        else if(ptrMAR->type == IS_EX)
        {
          bSAdelete = true;
          tempSAR = ptrSAR->Next;
          if(MASSQtable)
            MASSQtable->delSA(ptrMAR->MA,ptrSAR->SA,IS_IN);
          LStable->addSA(ptrMAR->MA,ptrSAR->SA,IS_EX);
          LStable->delSA(ptrMAR->MA,ptrSAR->SA,IS_IN);
        }
      }
      if(bSAdelete)
        ptrSAR = tempSAR;
      else
        ptrSAR = ptrSAR->Next;
    }

    if(ptrMAR->type == IS_IN)
    {
      if(ptrMAR->inNS==0)
      {
        bMAdelete = true;
        tempMAR = ptrMAR->Next;
        if(MASSQtable)
        {
          MASSQtable->delMA(ptrMAR->MA);
        }
        LStable->delMA(ptrMAR->MA);
      }
    }
    else if(ptrMAR->type == IS_EX)
    {
      if(ptrMAR->FilterTimer)
      {
        ptrMAR->FilterTimer--;
      }
      else
      { // Filter timer expire
        if(ptrMAR->inNS==0)
        { // Requested List is empty, delete Multicast Address Record.
          bMAdelete = true;
          tempMAR = ptrMAR->Next;
          if(MASSQtable)
          {
            MASSQtable->delMA(ptrMAR->MA);
          }
          LStable->delMA(ptrMAR->MA);
        }
        else
        { // switch to INCLUDE filter mode; Exclude List is deleted.
          ptrMAR->type = IS_IN;
          LStable->destroySAL(ptrMAR,IS_EX);
        }
      }
    }
    if(bMAdelete)
      ptrMAR = tempMAR;
    else
      ptrMAR = ptrMAR->Next;
  }
}

void MLD::MLDv2QueueMASSQ()
{
  MARecord_t* ptrMASSQ=MASSQtable->MAR();
  bool bDelMASSQ=false;

  while(ptrMASSQ)
  {
    bDelMASSQ=false;
    if(simTime() >= MLDv2_LLQI+ptrMASSQ->tPrevious)
    {
      MLDv2MASSQ(ptrMASSQ->MA,ptrMASSQ->inSAL);
      ptrMASSQ->tPrevious = simTime();
      scheduleAt(simTime()+MLDv2_LLQI,new cMessage("QueueMASSQ"));
      return;
    }
    ptrMASSQ = ptrMASSQ->Next;
  }
}


void MLD::SendDummy()
{
  MLDv2Message *DummyMsg= new MLDv2Message((ICMPv6Type)254,AppDataSize);

  DummyMsg->setLength(MLDv2_HEADER+AppDataSize);
  MLDv2sendIPdgram(DummyMsg, c_ipv6_addr("ff3e:0:0:0:0:0:0:1000"),IPv6_ADDR_UNSPECIFIED,1);
}

void MLD::SendJoin()
{
  MLDv2Record* JoinList=new MLDv2Record;
  int i,j;
  ipv6_addr bufMA;
  ipv6_addr bufSA;
  char bufAddr[40];

  cout << endl << OPP_Global::findNetNodeModule(this)->name() << "send Join message" << endl;
  for(i=0;i<1;i++)
  {
    sprintf(bufAddr,"ff3e:0:0:0:0:%d:0:1980",i+1);
    bufMA = c_ipv6_addr(bufAddr);
    JoinList->addMA(bufMA,ALLOW);
//    cout << "MA:" << ipv6_addr_toString(bufMA) << endl;
    for(j=0;j<Source;j++)
    {
      sprintf(bufAddr,"fec0:0:0:0:0:%d:%d:1002",i+1,j+1);
      bufSA = c_ipv6_addr(bufAddr);
//      cout << "SA:" << ipv6_addr_toString(bufSA) << endl;
      JoinList->addSA(bufMA,bufSA,IS_IN);
    }
  }

  MLDv2ChangeReport(JoinList);
  JoinList->destroyMAL();

  delete JoinList;
}

void MLD::SendRandJoin()
{
  MLDv2Record* JoinList=new MLDv2Record;
  int i,j;
  ipv6_addr bufMA;
  ipv6_addr bufSA;
  char bufAddr[40];

  cout << endl << OPP_Global::findNetNodeModule(this)->name() << "SendRandJoine at " << simTime() << endl;
  for(i=0;i<Group;i++)
  {
    if(((float)rand()/RAND_MAX)>((float)1/2))
    {
      sprintf(bufAddr,"ff3e:0:0:0:0:%d:0:1980",i+1);
      bufMA = c_ipv6_addr(bufAddr);
      JoinList->addMA(bufMA,ALLOW);
      LStable->addMA(bufMA,IS_IN);

      for(j=0;j<Source;j++)
      {
        if(((float)rand()/RAND_MAX)>((float)1/2))
        {
          sprintf(bufAddr,"fec0:0:0:0:0:%d:%d:1002",i+1,j+1);
          bufSA = c_ipv6_addr(bufAddr);

          JoinList->addSA(bufMA,bufSA,IS_IN);
          LStable->addSA(bufMA,bufSA,IS_IN);
        }
      }
    }
  }

  MLDv2ChangeReport(JoinList);
  JoinList->destroyMAL();

  delete JoinList;
}

void MLD::SendBlock()
{
  MLDv2Record* BlockList=new MLDv2Record;
  int i,j;
  ipv6_addr bufMA;
  ipv6_addr bufSA;
  char bufAddr[40];

  cout << endl << OPP_Global::findNetNodeModule(this)->name() << " send Leave message at " << simTime() << endl;
  for(i=0;i<Group;i++)
  {
    sprintf(bufAddr,"ff3e:0:0:0:0:%d:0:1980",i+1);
    bufMA = c_ipv6_addr(bufAddr);
//    cout << "MA:" << ipv6_addr_toString(bufMA) << endl;
    BlockList->addMA(bufMA,BLOCK);
    for(j=0;j<Source;j++)
    {
      sprintf(bufAddr,"fec0:0:0:0:0:%d:%d:1002",i+1,j+1);
      bufSA = c_ipv6_addr(bufAddr);
//      cout << "SA:" << ipv6_addr_toString(bufSA) << endl;
      BlockList->addSA(bufMA,bufSA,IS_IN);
    }
  }

//  if(simTime()<41)
//    scheduleAt(simTime()+1,new cMessage("Block"));

  MLDv2ChangeReport(BlockList);
  BlockList->destroyMAL();
  delete BlockList;
}

void MLD::SendAllBlock()
{
  MLDv2Record* BlockList=new MLDv2Record;
//  int i,j;
  MARecord_t* ptrMAR=LStable->MAR();
  SARecord_t* ptrSAR=NULL;

  cout << endl << OPP_Global::findNetNodeModule(this)->name() << " send All Leave message at " << simTime() << endl;

  while(ptrMAR)
  {
    BlockList->addMA(ptrMAR->MA,BLOCK);
    ptrSAR = ptrMAR->inSAL;
    while(ptrSAR)
    {
      BlockList->addSA(ptrMAR->MA,ptrSAR->SA,IS_IN);
      ptrSAR = ptrSAR->Next;
    }
    ptrMAR = ptrMAR->Next;
  }

//  if(simTime()<41)
//    scheduleAt(simTime()+1,new cMessage("Block"));

  MLDv2ChangeReport(BlockList);
  BlockList->destroyMAL();
  delete BlockList;
}
