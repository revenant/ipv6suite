##WirelessEtherNetwork
SET( WirelessEtherNetwork_ned_includes ${TOPDIR}/Nodes ${TOPDIR}/IP/DualStack
${TOPDIR}/NetworkInterfaces ${TOPDIR}/Transport/TCP ${TOPDIR}/Transport/UDP
${TOPDIR}/Applications/Ping6 ${TOPDIR}/IP/IPv4/MAC_LLC ${TOPDIR}/IP/IPv6/Generic
${TOPDIR}/IP/IPv4/QoS ${TOPDIR}/IP/IPv4/IPProcessing ${TOPDIR}/World ${TOPDIR}/PHY ${TOPDIR}/NetworkInterfaces/Wireless)

#ADD_TEST(WirelessEtherNetworkTest WirelessEtherNetwork)
#CREATE_SIMULATION(WirelessEtherNetwork WirelessEtherNetwork_ned_includes "WirelessEtherNetwork")
CREATE_SIM(WirelessEtherNetwork)
