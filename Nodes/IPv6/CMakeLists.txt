SET(NODES_SRCS EtherHub.cc)

SET(NODES_NED_INCLUDES ${TOPDIR}/PHY ${TOPDIR}/IP/DualStack
${TOPDIR}/NetworkInterfaces ${TOPDIR}/NetworkInterfaces/Ethernet
${TOPDIR}/IP/IPv4/MAC_LLC ${TOPDIR}/IP/IPv6/Generic ${TOPDIR}/IP/IPv4/QoS
${TOPDIR}/IP/IPv4/IPProcessing ${TOPDIR}/Transport/TCP ${TOPDIR}/Transport/UDP ${TOPDIR}/Applications/Ping6)


SET(NODES_NED EtherBridge.ned EtherHub.ned Router6.ned TCPNode.ned TCPSvrNode.ned Cables.ned)
SET(NODES_NED ${NODES_NED} UDPNode.ned)

IF(BUILD_MOBILITY)
  SET(NODES_NED_INCLUDES ${NODES_NED_INCLUDES} ${TOPDIR}/NetworkInterfaces/Wireless)
  SET(MOBILE_NODES_NED WirelessAccessPoint.ned 
  WirelessMobileNode.ned DualIntMobileNode.ned)
ENDIF(BUILD_MOBILITY)

IF(BUILD_UML)
  SET(NODES_SRCS ${NODES_SRCS} UmlSwitch.cc)
  SET(UML_NODES_NED UMLClient.ned UMLSwitch.ned)
ENDIF(BUILD_UML)

OPP_WRAP_NEDC(Nodes NODES_NED_INCLUDES ${NODES_NED} ${MOBILE_NODES_NED}
		    ${UML_NODES_NED})

RELEASE_SOURCES("${NODES_SRCS}")
ADD_LIBRARY(Nodes ${NODES_SRCS}  ${GENERATED_NEDC_FILES})

#Dualstack is for basic_consts
INCLUDE_DIRECTORIES(${TOPDIR}/IP/DualStack ${TOPDIR}/IP/IPv6/Generic ${TOPDIR}/NetworkInterfaces/Ethernet ${TOPDIR}/PHY)
IF(BUILD_MOBILITY)
  INCLUDE_DIRECTORIES(${TOPDIR}/World ${TOPDIR}/NetworkInterfaces/Wireless ${TOPDIR}/Util)
ENDIF(BUILD_MOBILITY)
