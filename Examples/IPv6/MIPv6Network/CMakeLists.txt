##MIPv6Network
SET( MIPv6Network_ned_includes ${TOPDIR}/Nodes ${TOPDIR}/IP/DualStack
${TOPDIR}/NetworkInterfaces ${TOPDIR}/Transport/TCP ${TOPDIR}/Transport/UDP
${TOPDIR}/Applications/Ping6 ${TOPDIR}/IP/IPv4/MAC_LLC ${TOPDIR}/IP/IPv6/Generic
${TOPDIR}/IP/IPv4/QoS ${TOPDIR}/IP/IPv4/IPProcessing ${TOPDIR}/World
${TOPDIR}/PHY ${TOPDIR}/NetworkInterfaces/Wireless ${TOPDIR}/NetworkInterfaces/Ethernet)

SET(MIPv6SOURCES MIPv6Network)

IF(EWU_MACBRIDGE)
  SET(MIPv6SOURCES ${MIPv6SOURCES} MIPv6MACBridgeNetwork)
ENDIF(EWU_MACBRIDGE)

SET(MIPv6SOURCES ${MIPv6SOURCES} MIPv6NetworkSimulMove MIPv6NetworkSimulMove2 MIPv6FastRANetwork PCOAForwarding)
#STRING(REGEX REPLACE "[ ]" "\\\;" RESULT "${MIPv6SOURCES}")


CREATE_SIMULATION(MIPv6Network MIPv6Network_ned_includes "${MIPv6SOURCES}")
COPY_MISSING_CONFIG_FILES("MIPv6NetworkSimulMove.xml;MIPv6NetworkSimulMove2.xml;MIPv6FastRANetwork.xml;PCOAForwarding.xml;PCOAForwarding.ini;MIPv6Network.ini;MIPv6NetworkSimulMove.ini")

ADD_TEST(FastRANet MIPv6Network -f MIPv6Network.ini -r 3)

ADD_TEST(PCOAFNet MIPv6Network -f PCOAForwarding.ini -r 1)

ADD_TEST(MIPv6NetworkSiumlMov2 MIPv6Network -f MIPv6NetworkSimulMove.ini -r 5)

