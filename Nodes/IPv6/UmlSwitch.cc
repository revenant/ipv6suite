//$Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Nodes/IPv6/Attic/UmlSwitch.cc,v 1.2 2005/02/10 05:43:47 andras Exp $
//
// Uml Switch Client for Omnet++ IPv6
// * Copyright (C) 2002 Greg Daley Monash University, Melbourne, Australia
//
// Code Adapted from that supplied by the following Authors:
// * Copyright (C) 2001 Johnny Lai Monash University, Melbourne, Australia
// * Copyright (C) 2001 Eric Wu    Monash University, Melbourne, Australia
// * Copyright (C) 2001 Lennert Buytenhek (buytenh@gnu.org) and
// * Copyright (C) 2001 by various other people who didn't put their name here.
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
	@file: UmlSwitch.cc
	Purpose: Attempting to connect to uml_switch

*/

//Define BUILD_UML_INTERFACE to include cnpayload.h
#include <cpacket.h>
#include <omnetpp.h>
#include <memory> //auto_ptr
#include <iterator> //ostream_iterator in boost/random

#include "UmlSwitch.h"
#include "boost/random.hpp"


extern "C" {
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>
}

#include "ethernet.h"
#include "MACAddress.h"
#include "EtherFrame.h"
/*#include "EtherState.h"*/

/*#include "PPPFrame.h"*/
#include "opp_utils.h"
#include "IPv6InterfaceData.h"
#include "Messages.h"
#include "IPDatagram.h"

typedef cTypedMessage<LLInterfaceInfo> LLInterfacePkt;

#ifdef TESTIPv6
#undef NDEBUG
#include <cassert>
#include "IPv6Datagram.h"
#endif

#if defined __CN_PAYLOAD_H
Define_Module_Like( UmlSwitchInterface, NetworkInterface );

void UmlSwitchInterface::initialize()
{
  LinkLayerModule::initialize();

  int eth_index = 0;
  unsigned char packmac[ETH_ALEN];
  int fdarray[3];

  for(int i = 0; i <  parentModule()->index(); i++)
  {
    // label interface name according to the link layer
    cModule* mod = OPP_Global::findModuleByName(this, "linkLayers");
    LinkLayerModule* llmodule = 0;
    mod = mod->parentModule()->submodule("linkLayers", i);

    if(mod)
    {
      llmodule = (LinkLayerModule*)(mod->submodule("networkInterface"));

      if(llmodule->getInterfaceType() == PR_ETHERNET)
        eth_index ++;
    }
  }

  // TODO: I am lying about what type of interface, will this cause trouble?

  sprintf(iface_name, "eth%d", eth_index);
  iface_type = PR_ETHERNET;

  static boost::mt19937 rng = boost::mt19937();

  MAC_address addr;

  addr.high = rng() & 0xFFFFFF;
  addr.low = rng() & 0xFFFFFF;

  my_addr.set(addr);

  cout<<"MAC address is "<<(const char*)my_addr<<endl;

  sw_init_args(control_path, data_path);
  // on initialise(ctl,data) do set_new_ctl_path set_new_data_path()

  pack_mac_addr((const char *)my_addr ,(unsigned char *)packmac);
  if(!init_swdriver((char *)control_path,(char *)data_path,
          (unsigned char *)packmac,fdarray)){
     // throw ::??
     return;
  }
  ctlfd = fdarray[0];
  writefd = fdarray[1];
  readfd = fdarray[2];
}

void UmlSwitchInterface::activity()
{
  cMessage *msg;

  while(true)
  {

    msg = receive();


    if (!strcmp(msg->arrivalGate()->name(), "ipOutputQueueIn"))
    {
      struct network_payload *frame = NULL;
      cMessage *nwiIdleMsg = new cMessage();
      nwiIdleMsg->setKind(NWI_IDLE);
      int sentlen;

      // encapsulate IP datagram in Ether frame
      EtherFrame *outFrame = new EtherFrame();

      MACAddress macsrc;
      MACAddress macdest;
      MAC_address addr;

      // TODO Not sure here, may require removal of IPv6 checks
#if !defined TESTIPv6
      LLInterfacePkt* recPkt = static_cast<LLInterfacePkt*>(msg);
#else
      LLInterfacePkt* recPkt = dynamic_cast<LLInterfacePkt*>(msg);
      assert(recPkt != 0);
      IPv6Datagram* dgram = dynamic_cast<IPv6Datagram*>(recPkt->data().dgram);
      assert(dgram != 0);
#endif

      outFrame->encapsulate(recPkt->data().dgram->dup());

      //ev << "contents = " <<outFrame->dumpContents() <<endl;

      // TODO, get dest mac
      if(!strcmp("", outFrame->srcAddrString())){
          macsrc = my_addr;
          ev << "setting srcaddress"<<endl;
          outFrame->setSrcAddress(macsrc);
      }
      else{
           ev << "src  " << outFrame->srcAddrString() << endl;
      }
      //if(!strcmp("",outFrame->srcAddrString())){

         addr.high = 0xFFFFFF;
         addr.low = 0xFFFFFF;
         macdest.set(addr);
         outFrame->setDestAddress(macdest);
      //}
      //else{
      //     ev << "dest " <<outFrame->srcAddrString() << endl;
      //}


      frame = outFrame->networkOrder();

      delete recPkt->data().dgram;
      delete recPkt;

      ev << frame->get_dump()<< endl;

      wait(delay);
      while ( (sentlen  = ::send(writefd,frame->get_start(), frame->len(), MSG_DONTWAIT)) < 0){
           if(errno != EINTR){
               ev << " errno = " << errno << " : " << strerror(errno) << endl;
               break;
           }
      }
      ev << sentlen << " octets  sent to fd " << writefd << endl;
      send(outFrame, "physicalOut");
      send(nwiIdleMsg, "ipOutputQueueOut");

    }
#if 0
    else // from Network
    {
      std::auto_ptr<PPPFrame> recFrame(static_cast<PPPFrame*>(msg));

      // decapsulate IP datagram
      if (recFrame->protocol() == PPP_PROT_IP)
      {
        // break off for /all/ bit errors
        if (recFrame->hasBitError())
        {
          ev << "\n+ PPPLink of " << fullPath()
             << " receive error: Error in transmission.\n";
          continue;
        }

        IPDatagram *ipdatagram =
          static_cast<IPDatagram*> (recFrame->decapsulate());

        wait(delay);
        send(ipdatagram, "ipInputQueueOut");
      } else // print error message if it's not an IP datagram
      {
        ev << "\n+ PPPLink of " << fullPath()
           << " receive error: Not IP protocol.\n";
      }
    }
#endif
  }
}


struct sockaddr_un *UmlSwitchInterface::new_addr(void *name, int len){
   struct sockaddr_un *sun;

   sun = (struct sockaddr_un *)malloc(sizeof(struct sockaddr_un));
   if(sun == NULL){
      fprintf(stderr, "new_addr: allocation of sockaddr_un failed\n");
      return(NULL);
   }
   sun->sun_family = AF_UNIX;
   memcpy(sun->sun_path, name, len);
   return(sun);
}


int UmlSwitchInterface::sw_unix_connect(struct sockaddr_un *unixsock, int type){
    int fd;

    if( (fd = socket(PF_UNIX,type,0)) < 0 ){
           fprintf (stderr, "socket() failed\n");
           return -1;
    }

    if(connect(fd, (struct sockaddr *) unixsock,
                   sizeof(struct sockaddr_un)) < 0){
           fprintf(stderr,"sw_unix_connect() : failed, errno = %d\n",
           errno);
           fprintf(stderr,"error:  %s\n", strerror(errno));
           return -1;
    }
    return fd;
}




int UmlSwitchInterface::sw_data_connect(char *data_path){
    struct sockaddr_un unix_path;

    memset(&unix_path, 0,sizeof (struct sockaddr_un ));

    unix_path.sun_family = AF_UNIX;
    strncpy(unix_path.sun_path,data_path, UNIX_MAX_PATH-1);

        printf ("data:  %s\n", data_path);
        printf ("path:  %s\n", unix_path.sun_path);

    return sw_unix_connect(&unix_path, SOCK_DGRAM);
}

int UmlSwitchInterface::sw_control_connect(char *control_path){
    struct sockaddr_un unix_path;

    memset(&unix_path, 0,sizeof (struct sockaddr_un ));

    unix_path.sun_family = AF_UNIX;
    strncpy(unix_path.sun_path,control_path, UNIX_MAX_PATH-1);

       printf ("ctl :  %s\n", control_path);
       printf ("path:  %s\n", unix_path.sun_path);

    return sw_unix_connect(&unix_path, SOCK_STREAM);
}

struct sockaddr_un *UmlSwitchInterface::gen_local_addr(void){
    struct sockaddr_un *local_addr;
    struct timeval tv;
    struct {
             char zero;
             int pid;
             int usecs;
    } name;

    name.zero = 0;
    name.pid = getpid();
    gettimeofday(&tv, NULL);
    name.usecs = tv.tv_usec;

    local_addr = new_addr(&name, sizeof(name));

    return local_addr;
}




void UmlSwitchInterface::sw_init_args(char *control_path, char *data_path){

     set_new_data_path(UNIX_SOCKET_CONTROL_PATH_DFL,control_path);
     set_new_control_path(UNIX_SOCKET_DATA_PATH_DFL,data_path);
        printf("%s\n", control_path);
        printf("%s\n", data_path);
}


int UmlSwitchInterface::set_new_data_path(const char *newval, char * dest){
    return (strncpy(dest,newval,UNIX_MAX_PATH -1)? 0 : -1);
}


int UmlSwitchInterface::set_new_control_path(const char *newval, char * dest){
    return (strncpy(dest, newval, UNIX_MAX_PATH -1 )? 0 : -1);
}




int UmlSwitchInterface::send_control_request(int fd,
          struct sockaddr_un *unix_sock, unsigned char *addr){
    int n;
    struct request req;

    req.type = REQ_NEW_CONTROL;
    memcpy(req.u.new_control.addr, addr, sizeof(req.u.new_control.addr));
    req.u.new_control.name = *unix_sock;
    n = write(fd, &req, sizeof(req));

    if(n != sizeof(req)){
         fprintf(stderr, "daemon_open : control setup request returned %d, "
                   "errno = %d\n", n, errno);
         n = -ENOTCONN;
         return n;
    }

    return 0;
}




int UmlSwitchInterface::pack_mac_addr(const char *straddr,
      unsigned char *pack){
    int i, j;
    unsigned char val;
    char ch;

   if(!(straddr && pack )|| (strlen(straddr) != 17))
        return -1;

   for(i = 0; i < 6 ; i ++){
      pack[i] = 0;
      for( j = 0;  j < 2; j++){
         ch = straddr[ (3 * i )+j];
         switch(ch){
             case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                   val = (ch - 'a' )  + 0x0a;
                   break;
             case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                   val = (ch - 'A' )  + 0x0a;
                   break;
             case '0': case '1': case '2': case '3': case '4':
             case '5': case '6': case '7': case '8': case '9':
                   val = (ch - '0' );
                   break;
             default:
                   return -1;
                   break;
         }
         if (j){
             pack[i] <<= 4;
         }
         pack[i] |= val;
      }
   }
   return 0;
}

void UmlSwitchInterface::initfds(int fds[]){
    int i = 0;
    for(i = 0 ; i < 3 ; i++){
      fds[i] = -1;
    }
}

int *UmlSwitchInterface::init_swdriver(char *control_path, char *data_path,
                unsigned char *macaddr, int fds[]){
    struct sockaddr_un *local_addr;

    if(!fds){
         return NULL;
    }

    initfds(fds);

    if(!(  local_addr = gen_local_addr()) ){
        /* all automatic now */
        fprintf(stderr,"unable to create local address\n");
        return NULL;
    }

    if((fds[0] = sw_control_connect(control_path)) < 0 ){
       initfds(fds);
       fprintf(stderr,"unable to attach to switch: %s\n", control_path);
       return NULL;
    }
    ev << "controlfd: " << fds[0] << endl;
    if(send_control_request(fds[0], local_addr, macaddr) < 0){
       close(fds[0]);
       initfds(fds);
       fprintf(stderr,"unable to send control message to: %s\n", control_path);
       return NULL;
    }

    if((fds[1] = sw_data_connect(data_path)) < 0 ){
       fprintf(stderr,"unable to attach to switch: %s\n", data_path);
       close(fds[0]);
       initfds(fds);
       return NULL;
    }

    ev << "writefd: " << fds[1] << endl;

    if( (fds[2] = socket(PF_UNIX,SOCK_DGRAM,0)) < 0 ){
       fprintf(stderr,"socket() failed\n");
       close(fds[0]); close(fds[1]);
       initfds(fds);
       return NULL;
    }

    if(bind(fds[2], (struct sockaddr *) local_addr,
                   sizeof(struct sockaddr_un)) < 0){
       fprintf(stderr,"bind() : receive bind failed, errno = %d\n", errno);
       fprintf(stderr,"error:  %s\n", strerror(errno));
       close(fds[0]); close(fds[1]);close(fds[2]);
       initfds(fds);
       return NULL;
    }

    ev << "readfd: " << fds[2] << endl;
    return fds;
}
#endif // __CN_PAYLOAD_H
