##LargeTestNetwork
SET( LargeTestNetwork_ned_includes ${TOPDIR}/NetComponents/Nodes ${TOPDIR}/Util/DualStack
${TOPDIR}/NetworkInterfaces ${TOPDIR}/Transport/TCP ${TOPDIR}/Transport/UDP
${TOPDIR}/Applications/Ping6 ${TOPDIR}/IPv4/MAC_LLC ${TOPDIR}/IPv6/IPv6Processing
${TOPDIR}/IPv4/QoS ${TOPDIR}/IPv4/IPProcessing ${TOPDIR}/World ${TOPDIR}/NetComponents/Networks ${TOPDIR}/PHY)
#OPP_WRAP_NEDC(LargeTestNetwork LargeTestNetwork_ned_includes
#LargeTestNetwork.ned ../../NetComponents/Networks/Ethernet.ned)

#SET_SOURCE_FILES_PROPERTIES(${GENERATED_NEDC_FILES}  GENERATED COMPILE_FLAGS -Wno-unused)

#RELEASE_FILES(LargeTestNetwork.ned)
#COPY_MISSING_CONFIG_FILES("omnetpp.ini")
#ADD_EXECUTABLE(LargeTestNetwork LargeTestNetwork_n.cc Ethernet_n.cc)
CREATE_SIM(LargeTestNetwork)
