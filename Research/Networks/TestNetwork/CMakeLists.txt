##TestNetwork
SET( TestNetwork_ned_includes ${TOPDIR}/Nodes ${TOPDIR}/IP/DualStack
${TOPDIR}/NetworkInterfaces ${TOPDIR}/Transport/TCP ${TOPDIR}/Transport/UDP
${TOPDIR}/Applications/Ping6 ${TOPDIR}/IP/IPv4/MAC_LLC ${TOPDIR}/IP/IPv6/Generic
${TOPDIR}/IP/IPv4/QoS ${TOPDIR}/IP/IPv4/IPProcessing ${TOPDIR}/World ${TOPDIR}/PHY)

SET(TestSources TestNetwork)
IF(ALPHA_ABSTRACT_UDP)
  SET(TestSources ${TestSources} UDPNetwork)
ENDIF(ALPHA_ABSTRACT_UDP)
CREATE_SIMULATION(TestNetwork TestNetwork_ned_includes "${TestSources}")
COPY_MISSING_CONFIG_FILES("XMLTEST.xml")

#Unmaintained
IF(BUILD_UNITTESTS)
  ADD_TEST(XMLParsingUnitTests TestNetwork -r 3)
ELSE(BUILD_UNITTESTS)
  ADD_TEST(OldTCPNode TestNetwork -r 1)
ENDIF(BUILD_UNITTESTS)

ADD_TEST(PingTestWithOldTCPNode TestNetwork -r 2)
ADD_TEST(VideoStream TestNetwork -r 4)
