##EthNetwork
SET( EthNetwork_ned_includes ${TOPDIR}/Nodes ${TOPDIR}/IP/DualStack
${TOPDIR}/NetworkInterfaces ${TOPDIR}/Transport/TCP ${TOPDIR}/Transport/UDP
${TOPDIR}/Applications/Ping6 ${TOPDIR}/IP/IPv4/MAC_LLC ${TOPDIR}/IP/IPv6/Generic
${TOPDIR}/IP/IPv4/QoS ${TOPDIR}/IP/IPv4/IPProcessing ${TOPDIR}/World ${TOPDIR}/PHY)
#ADD_TEST(EthNetworkTest EthNetwork) 
CREATE_SIMULATION(EthNetwork EthNetwork_ned_includes "Ethernetwork")
COPY_MISSING_CONFIG_FILES("EthNetwork-debug.xml")

IF (${PROJECT_SOURCE_DIR} MATCHES ${PROJECT_BINARY_DIR})
#Operation not required because binary and source directories are the same
ELSE(${PROJECT_SOURCE_DIR} MATCHES ${PROJECT_BINARY_DIR})
  INCLUDE(${CMAKEFILES_PATH}/CustomTargets.cmake)
  #This should be very first test run so we get updated config files
  ADD_TEST(COPYCONFIG /usr/bin/make OutOfSourceCopyConfig)
ENDIF(${PROJECT_SOURCE_DIR} MATCHES ${PROJECT_BINARY_DIR})

ADD_TEST(EthernetPing EthNetwork -r 3)
