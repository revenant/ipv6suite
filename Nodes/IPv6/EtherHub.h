// EtherHub.h
//
// Simulazione Ethernet RFC 802.3
// Michele Baresi, 1999

#ifndef __ETHER_HUB__
#define __ETHER_HUB__


#include <omnetpp.h>
#include "EtherFrame.h"
#include "MLDv2Record.h"

//#define    PrintResult

extern const int DISTANCE;

class EtherSignalData;

class Repeater :public cSimpleModule
{
  Module_Class_Members(Repeater,cSimpleModule, 0);

  virtual void initialize(void);
  virtual void handleMessage(cMessage* msg);

 protected:
  int from_net_gate_id,to_net_gate_id;

  cMessage *jam_msg,*end_frame_msg;

//  void repeatFrame(EtherSignalData* msg);
  virtual void repeatFrame(EtherSignalData* msg, int orig_gate_id);

#ifdef PrintResult
 private:
  FILE *fpAggregate;
  double Aggregate;
  int countAgg;
  simtime_t stAggregate;

  FILE *fpOccur;    // aggregate
  double Occur;
  int countOccur;
#endif
};

#endif
