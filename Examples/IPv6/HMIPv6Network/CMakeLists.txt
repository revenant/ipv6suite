##HMIPv6Network
SET( HMIPv6Network_ned_includes ${TOPDIR}/Nodes ${TOPDIR}/IP/DualStack
${TOPDIR}/NetworkInterfaces ${TOPDIR}/Transport/TCP ${TOPDIR}/Transport/UDP
${TOPDIR}/Applications/Ping6 ${TOPDIR}/IP/IPv4/MAC_LLC ${TOPDIR}/IP/IPv6/Generic
${TOPDIR}/IP/IPv4/QoS ${TOPDIR}/IP/IPv4/IPProcessing ${TOPDIR}/World ${TOPDIR}/PHY ${TOPDIR}/NetworkInterfaces/Wireless)
#OPP_WRAP_NEDC(HMIPv6Network HMIPv6Network_ned_includes HMIPv6Network.ned)

#SET_SOURCE_FILES_PROPERTIES(${GENERATED_NEDC_FILES}  GENERATED COMPILE_FLAGS -Wno-unused)

#COPY_MISSING_CONFIG_FILES_FOR_SIMULATION()
#ADD_EXECUTABLE(HMIPv6Network HMIPv6Network_n.cc)
#CREATE_SIM(HMIPv6Network)
SET(HMIPv6SOURCES HMIPv6Network HMIPv6Simple)
CREATE_SIMULATION(HMIPv6Network HMIPv6Network_ned_includes "${HMIPv6SOURCES}")

COPY_MISSING_CONFIG_FILES("HMIPv6Simple.xml;HMIPv6Simple.ini;HMIPv6Sait.ini;HMIPv6Sait.xml;HMIPv6Sait.ned;SaitEHCal.ned;SaitEHCal.xml;SaitEHCal.ini")

ADD_TEST(SimpleHMIP HMIPv6Network -f HMIPv6Simple.ini -r 1)
#Does previous MAP to next MAP forwarding in code (perhaps I should add XML var to toggle this)
ADD_TEST(MapToMapHandover HMIPv6Network -f HMIPv6Sait.ini -r 1)
ADD_TEST(EH1secondPing HMIPv6Network -f SaitEHCal.ini -r 1)
ADD_TEST(EH10msPing HMIPv6Network -f SaitEHCal.ini -r 2)
ADD_TEST(EHUDPStream HMIPv6Network -f SaitEHCal.ini -r 3)
ADD_TEST(EHPingStream HMIPv6Network -f SaitEHCal.ini -r 4)
