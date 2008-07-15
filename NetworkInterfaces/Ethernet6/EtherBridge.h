// EtherBridge.h

#ifndef __ETHER_SWITCH__
#define __ETHER_SWITCH__

#include <omnetpp.h>
#include <list>



class EtherSignalData;

struct MACEntry
{
  std::string address;
  int outputPort;
  simtime_t expireTime;
};

class EtherRelayUnit :public cSimpleModule
{
  Module_Class_Members(EtherRelayUnit,cSimpleModule, 0);

  virtual void initialize(void);
  virtual void handleMessage(cMessage* msg);

 protected:

  void updateDB(const char* address, int outputPort);
  int outputPortByAddress(const char* address);

  void removeEntry(void);
  double nextExpireTime(void);

 protected:
  double advInterval;
  double entryTimeout;

  std::list<MACEntry*> filteringDB;
  cMessage* entryRemovalTimerMsg;
};

typedef std::list<MACEntry*>::iterator MACEntryIt;

#endif
