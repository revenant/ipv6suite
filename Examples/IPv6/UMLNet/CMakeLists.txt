##UMLNet
SET( UMLNet_ned_includes ${TOPDIR}/Nodes ${TOPDIR}/IP/DualStack
${TOPDIR}/NetworkInterfaces ${TOPDIR}/Transport/TCP ${TOPDIR}/Transport/UDP
${TOPDIR}/Applications/Ping6 ${TOPDIR}/IP/IPv4/MAC_LLC ${TOPDIR}/IP/IPv6/Generic
${TOPDIR}/IP/IPv4/QoS ${TOPDIR}/IP/IPv4/IPProcessing ${TOPDIR}/World ${TOPDIR}/PHY)
#OPP_WRAP_NEDC(UMLNet UMLNet_ned_includes UMLNet.ned )
#
#SET_SOURCE_FILES_PROPERTIES(${GENERATED_NEDC_FILES}  GENERATED COMPILE_FLAGS -Wno-unused)
#
#COPY_MISSING_CONFIG_FILES("network.xml;omnetpp.ini")
#ADD_EXECUTABLE(UMLNet UMLNet_n.cc)

#CREATE_SIMULATION(UMLNet UMLNet_ned_includes "UMLNet")
CREATE_SIM(UMLNet)
