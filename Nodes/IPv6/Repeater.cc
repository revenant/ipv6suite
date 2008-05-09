// Repeater.cc
//
// Simulazione Ethernet RFC 802.3
// Michele Baresi, 1999

#include <boost/cast.hpp>
#include "Repeater.h"
#include "ethernet6.h"
#include "EtherSignal_m.h"

const int DISTANCE = 100;

Define_Module( Repeater );

void Repeater::initialize(void)
{
  from_net_gate_id = findGate("from_net",0);
  to_net_gate_id = findGate("to_net",0);
#ifdef PrintResult
  Aggregate = 0;
  stAggregate = 0;
  fpAggregate = fopen("Aggregate.txt","w+");
  fpOccur = fopen("Occur.txt","w+");
  countAgg = 0;
  countOccur = 0;

  scheduleAt(1,new cMessage("BriefSum"));
//  scheduleAt(10.5,new cMessage("SendDummy"));
#endif
}


void Repeater::handleMessage(cMessage* msg)
{
#ifdef PrintResult    // by Wally
  if(msg->name() == string("SendDummy"))
  {
    cout << "SendDummy" << endl;
    Aggregate += 20000;
    scheduleAt(simTime()+1,new cMessage("SendDummy"));
    delete msg;
    return;
  }
  else if (msg->name() == string("BriefSum"))
  {
    if(simTime()<MLDv2_EndTime)
    {
      countAgg++;
      fprintf(fpAggregate,"Aggregate(%d)=%lf;\n",(int)floor(simTime()),Aggregate/1000);
//      fprintf(fpAggregate,"AggregateT(%d)=%lf;\n",countAgg,simTime());
      Aggregate = 0;
      scheduleAt(simTime()+1,new cMessage("BriefSum"));
    }
    else
    {
      cout << "close file at " << simTime() << endl;
      fclose(fpAggregate);
      fpAggregate = NULL;
      fclose(fpOccur);
      fpOccur = NULL;
    }
    delete msg;
    return;
  }
#endif
  repeatFrame(static_cast<EtherSignalData*>(msg), msg->arrivalGateId());
#ifdef PrintResult    // by Wally
  if(msg->length()&&fpOccur)
  {
    Occur = msg->length();
    Aggregate += Occur;
    countOccur++;
//    fprintf(fpOccur,"Occur(%d)=%lf;\n",countOccur,Occur/1000);
//    fprintf(fpOccur,"OccurT(%d)=%lf;\n",countOccur,simTime());
  }
//  cout << "Frame length:" << msg->length() << endl;
#endif
  delete msg;
}

void Repeater::repeatFrame(EtherSignalData* recFrame,int orig_gate_id)
{
  int nosend_gate_id = orig_gate_id - from_net_gate_id + to_net_gate_id;

  for (int i=to_net_gate_id; i< to_net_gate_id + gate("to_net")->size();i++)
  {
    if (i != nosend_gate_id)
      send((cMessage *)recFrame->dup(), i);
  }
}
