// EtherBridge.cc
//

#include <boost/cast.hpp>

#include <string>
#include <cassert>

#include "EtherBridge.h"
#include "ethernet.h"
#include "EtherSignal.h"
#include "EtherFrame6.h"
#include "MACAddress6.h"

Define_Module( EtherRelayUnit );

void EtherRelayUnit::initialize(void)
{
  entryTimeout = par("entryTimeout").doubleValue();
  entryRemovalTimerMsg = 0;
}

void EtherRelayUnit::handleMessage(cMessage* msg)
{
  if ( std::string(msg->name()) == "PROTOCOL_NOTIFIER")
  {
    delete msg;
    return;
  }

  if ( msg->isSelfMessage())
  {
    removeEntry();
    return;
  }

  EtherFrame6* frame =
    check_and_cast<EtherFrame6*>(msg);
  assert(frame);

  int chkOutputPort = outputPortByAddress(frame->destAddrString());

  if ( chkOutputPort != -1 )
  {
    EtherSignalData* signal = new EtherSignalData(frame->dup());

    cMessage* internalNotifier = new cMessage;
    internalNotifier->setKind(MK_PACKET);
    internalNotifier->encapsulate(signal);

    // redirect the frame to the current network that the mobile station
    // locates
    send(internalNotifier, gate("ethOut", chkOutputPort));

    return;
  }

  chkOutputPort = outputPortByAddress(frame->srcAddrString());

  if ( chkOutputPort != -1 )
  {
    for (int i = 0; i < gate("ethOut", chkOutputPort)->size(); i ++)
    {
      if ( chkOutputPort != i )
      {
        EtherSignalData* signal = new EtherSignalData(frame->dup());

        cMessage* internalNotifier = new cMessage;
        internalNotifier->setKind(MK_PACKET);
        internalNotifier->encapsulate(signal);

        send(internalNotifier, gate("ethOut", i));
      }
    }
    return;
  }

  cMessage *dataPkt = frame->decapsulate();

  if (dataPkt->kind() == MAC_BRIDGE_REGISTER )
  {
    const char* msAddr = static_cast<cPar*>(dataPkt->parList().get(0))->stringValue();

    // update the database
    updateDB(msAddr, msg->arrivalGate()->index());
  }
  delete dataPkt;

  delete frame;
}

void EtherRelayUnit::updateDB(const char* address, int outputPort)
{
  bool isFound =  false;

  for (MACEntryIt it = filteringDB.begin(); it != filteringDB.end(); it++)
  {
    if ( (*it)->address == std::string(address) )
    {
      (*it)->outputPort = outputPort;
      (*it)->expireTime = simTime() + entryTimeout;
      isFound = true;
      break;
    }
  }

  if ( isFound == false )
  {
    MACEntry* entry = new MACEntry;
    entry->address = std::string(address);
    entry->outputPort = outputPort;
    entry->expireTime = simTime() + entryTimeout;

    bool isFirstEntry = (filteringDB.size() ? false : true);
    if ( isFirstEntry )
    {
      entryRemovalTimerMsg = new cMessage();
      scheduleAt(simTime() + entryTimeout, entryRemovalTimerMsg);
    }

    filteringDB.push_back(entry);
  }
}

int EtherRelayUnit::outputPortByAddress(const char* address)
{
  for (MACEntryIt it = filteringDB.begin(); it != filteringDB.end(); it++)
  {
    if ( (*it)->address == std::string(address) )
    {
      return (*it)->outputPort;
    }
  }

  return -1;
}

void EtherRelayUnit::removeEntry(void)
{
  for (MACEntryIt it = filteringDB.begin(); it != filteringDB.end(); it++)
  {
    if ( (*it)->expireTime <= simTime() )
    {
      delete (*it);
      it = filteringDB.erase(it);
      it--;
    }
  }

  assert(entryRemovalTimerMsg);

  if (!filteringDB.size())
  {
    delete entryRemovalTimerMsg;
    return;
  }

  scheduleAt(nextExpireTime(), entryRemovalTimerMsg);
}

double EtherRelayUnit::nextExpireTime(void)
{
  double nextExpire = (*(filteringDB.begin()))->expireTime - simTime();

  for (MACEntryIt it = filteringDB.begin(); it != filteringDB.end(); it++)
  {
    if ( nextExpire > ((*it)->expireTime - simTime()) )
      nextExpire = (*it)->expireTime - simTime();
  }

  return simTime() + nextExpire;
}
