// -*- C++ -*-
// Copyright (C) 2002, 2003, 2004 CTIE, Monash University
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
 * @file MIPv6CDSMobileNode.h
 * @author Johnny Lai
 * @date 05 May 2002
 * @brief Conceptual Data Structures for Mobile Node and the interface to it
 */

#ifndef MIPV6CDSMOBILENODE_H
#define MIPV6CDSMOBILENODE_H 1

#include "MIPv6CDS.h"

#ifndef MAP
#define MAP
#include <map>
#endif //MAP

#ifndef LIST
#define LIST
#include <list>
#endif //LIST

#ifndef IPV6ADDRESS_H
#include "IPv6Address.h"
#endif //IPV6ADDRESS_H


class InterfaceEntry;
class IPv6Mobility;


namespace MobileIPv6
{

  class MIPv6RouterEntry;
  class bu_entry;


/**
 * @class MIPv6CDSMobileNode
 *
 * @brief Interface to the CDS of a MobileNode and some implementation specific
 * data structures are added too
 *
 * detailed description
 */

  class MIPv6CDSMobileNode: public MIPv6CDS
  {

    friend class MIPv6NDStateHost;
    friend class MIPv6MStateMobileNode;

  public:

    //@name constructors, destructors and operators
    //@{
    MIPv6CDSMobileNode(size_t interfaceCount);

    ~MIPv6CDSMobileNode();
    //@}

    ///@name Home Agents/MIPv6 Routers  List
    //@{

    boost::shared_ptr<MIPv6RouterEntry>& primaryHA() const
      {
      //   if (!halEmpty())
//         {
//           boost::weak_ptr<MIPv6RouterEntry> ha = *(mrl.begin());
//           //We will not add any routers to the rtr list until we add our very
//           //first homeAgent
//           assert(ha->isHomeAgent() == true);
//           return ha;
//         }
//         return boost::weak_ptr<MIPv6RouterEntry>();

        //The previous implementation was too restrictive as it prevented
        //foreign HA from been added to the list if the primary HA has not been
        //added yet (startup in foreign network case)

        return _primaryHA;
      }

    /**
     * We need a separate defaultRouter variable because defaultRouter will
     * automatically return reachable routers and does not notify us of this
     * change.
     *
     */
    //This could have been implemented as the last entry in the ha/router list
    //but then we'd have to insert new routers into the hal just one before the
    //last position and swap them around if it becomes the new " default
    //router"
    boost::shared_ptr<MIPv6RouterEntry>& currentRouter() const
      {
        return _currentRouter;
      }

    /**
       @brief Returns the last known MIPv6 router

       L2 trigger and low ping intervals (20-70ms) causes conceptual sending to
       regenerate destination cache entries using defaultRouter that was in the
       previous subnet. Thus we need to store the previous Default router as
       currentRouter() gets set to null when moving to new subnet and no
       router's detected yet(For case when oldRtr and newRtr are known then this
       function is not necessary) . With this variable we are able to update the
       dest cache entries pointing to previousDefaultRtr and redirect them to
       the new router.  This is done in handover from oldRtr=none to newRtr.

       If this updating of dest entries was not done then BUs to CNs may go via
       old default router and route optimised CNs will continue sending to our
       prior access network. And end up with many dropped packets.
    */
    boost::shared_ptr<MIPv6RouterEntry>& previousDefaultRouter() const
      {
        return _previousDefaultRtr;
      }

    /**
       @brief fn to return newly visited router

       In case router has already been visited before e.g. coming back to the
       HA. If this is not done then when it detects movement and has already
       received RA from this "newly" visited router it will not know about this
       and send an RS again. Thus there is an extra unsolicited router
       advertisement delay before it completes handover properly.

       Previously I detected new routers by looking into the mlr in this
       class. However only new routers/HAs get inserted into there so if we ever
       return home or go back to a previous router that we've visited before
       these are not added to the end of mrl. When movement is detected the MN
       thinks that there have been no router advs yet from the default router on
       visited subnet.
     */
    boost::shared_ptr<MIPv6RouterEntry>& potentialNewDefaultRtr() const
      {
        return _potentialNewDefaultRtr;
      }

    ///Return the homeagent with link local address in addr
    boost::shared_ptr<MIPv6RouterEntry> findRouter(const ipv6_addr& ll_addr);

    ///Return the homeagent with the global address in g_addr
    boost::shared_ptr<MIPv6RouterEntry> findHomeAgent(const ipv6_addr& g_addr);

    bool halEmpty() const
      {
        return mrl.empty();
      }

    ///Should remove the corresponding BUL entry too
    bool removeHomeAgent(boost::weak_ptr<MIPv6RouterEntry> ha);
    void insertHomeAgent(MIPv6RouterEntry* ha);

    /**
     * @param ha is either the Homeagent or the foreign MIPv6 router or HA
     *
     * @param primary is true if inserting the Primary HA otherwise it is false
     * for the current Router.
     *
     * This hack is necessary due to the unknown order of setup i.e. whether the
     * primary home agent is discovered first or an MIPv6 Router or HA on
     * foreign network
     */
    void setHomeAgent(boost::shared_ptr<MIPv6RouterEntry> ha, bool primary);


    //@}

    ///@name MIPv6 state
    //@{

    /**
     * Looks to me like you can only have one primary home agent per node
     * instead of per interface just like one default router per node not per
     * interface
     *
     */

    const ipv6_prefix& homePrefix() const;

    /**
     * There can be many home addr but they usually mean previous care of addr.
     * Another possibility is many global on-link prefixes on home subnet so
     * many homeAddr from that.  However we will only register one home addr so
     * far for simplicity's sake.  Although if nodes contact us on those other
     * prefixes we should still be reachable on them (how would forwarding of
     * these other prefixes be implemented?).
     */

    //Return a cached version of the home addr and update it if needed later on
    //for cases when home addr statically configured but no home agent
    const ipv6_addr& homeAddr() const { return _homeAddr.prefix; }

    /**
     * @return homeAddr corresponding to ha
     *
     * Usually the returned home addr is the primary home address.  However when
     * forwarding from previous care of addr the home addr is the previous care
     * of addr (this should have been sent as a BU to ha)
     *
     *
     */
    const ipv6_addr& homeAddr(boost::shared_ptr<MIPv6RouterEntry> ha) const;

    /**
     * There is only one current care of addr and that is the one registred with
     * primary HA. Even previous HA in BUL will all be updated with this coa
     * even though the home address in that binding could be previous care of
     * addr.  (This careOfAddr is semantically different from the Using Multiple
     * Care of Address in 10.20 of draft as that refers more to the binding of
     * that addr to this node)
     *
     */

    const ipv6_addr& careOfAddr(bool old = true) const;

    //@}

    ///@name Binding Update List
    //@{
    bool bulEmpty() const { return bul.empty(); }
    void addBU(bu_entry* bu);
    ///return the bul entry or 0 if no BUs were sent to destAddr
    bu_entry* findBU(const ipv6_addr& destAddr) const;
    bool removeBU(const ipv6_addr& addr);
    //@}

    bool awayFromHome() const { return away; }
    void setAwayFromHome(bool notAtHome);
    bool movementDetected() const { return moved; }
    void setMovementDetected(bool move) { moved = move; }

    /**
     * @param re the MIPv6 router to form coa from
     * @param ie the interface to retrieve Interface ID from
     */

    ipv6_addr formCareOfAddress(boost::weak_ptr<MIPv6RouterEntry> re,
                                InterfaceEntry *ie) const;

    ///Returns the preferred lifetime management function to be called at every
    ///period
    virtual TFunctorBaseA<cTimerMessage>* setupLifetimeManagement();

    ///lifetime for registrations with HAs for previous coa forwarding purposes
    simtime_t pcoaLifetime(){ return _pcoaLifetime; }

    bool eagerHandover() { return eagerHO; }
    void setEagerHandover(bool eager) { eagerHO = eager; }

  protected:

    void expireLifetimes(cTimerMessage* tmr);
    void setFutureCoa(const ipv6_addr& ncoa);

  private:

    typedef std::list<boost::shared_ptr<bu_entry> > BindingUpdateList;
    typedef BindingUpdateList::iterator BULI;

    typedef std::list<boost::shared_ptr<MIPv6RouterEntry> > MIPv6RouterList;
    typedef MIPv6RouterList::iterator MRLI;

    bool findRouter(const ipv6_addr& ll_addr, MRLI& it);

  private:

    /**
     * @brief Mobile Routers List - routers with MIPv6 support i.e. global address in RA.
     *
     * Should only contain MIPv6 routers i.e. routers with global prefix
     * advertised including HAs.
     *
     * Serves as the Home Agent's list too for the entries that return true for
     * isHomeAgent.  Required to perform handovers which does not always depend on HA's
     * existing on every foreign network only that there be a global prefix
     * available on every foreign network to create a care of addr to register
     * with Home HA.
     *
     * mrl stores only new routers and is not a history list of visited routers
     * @todo implement such a history list as you can not insert the same pointer
     * into mrl it justs removes the original shared_ptr. Need a weak_ptr list
     * for this.
     */

    MIPv6RouterList mrl;

    /**
     * List of CN and HA to which a binding has been sent to recently. The
     * primaryHA should always be the first one in this list
     */

    mutable BindingUpdateList bul;

    ipv6_prefix _homeAddr;
    ///Needed because the careOfAddr() uses BUL and can contain a pcoa as we may
    ///not have sent a BU yet due to DAD of ncoa.
    ipv6_addr   futureCoa;

    ///Maintains running state of whether we are away from home
    bool away;
    ///flag for movementDetected func
    bool moved;

    simtime_t _pcoaLifetime;

    mutable boost::shared_ptr<MIPv6RouterEntry> _primaryHA;
    mutable boost::shared_ptr<MIPv6RouterEntry> _currentRouter;
    mutable boost::shared_ptr<MIPv6RouterEntry> _previousDefaultRtr;
    mutable boost::shared_ptr<MIPv6RouterEntry> _potentialNewDefaultRtr;

    /**
       @brief when true will handover to new router regardless of movement detection

       Read from XML conf. Does not affect HMIP operation since HMIP relies on
       RA from routers to trigger MAP handover anyway.

       @note It will mistakenly handover to new router on same subnet if both
       advertise so this option is best only for one adv. router per subnet.
    */
    bool eagerHO;

    ///@name MIPv6MStateMobileNode members transported here to allow multiple MNs to work
    //@{

    //@}
  };

} //namespace MobileIPv6

#endif /* MIPV6CDSMOBILENODE_H */
