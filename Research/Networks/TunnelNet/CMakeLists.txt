##TunnelNet
SET( TunnelNet_ned_includes ${TOPDIR}/Nodes ${TOPDIR}/IP/DualStack
${TOPDIR}/NetworkInterfaces ${TOPDIR}/Transport/TCP ${TOPDIR}/Transport/UDP
${TOPDIR}/Applications/Ping6 ${TOPDIR}/IP/IPv4/MAC_LLC ${TOPDIR}/IP/IPv6/Generic
${TOPDIR}/IP/IPv4/QoS ${TOPDIR}/IP/IPv4/IPProcessing ${TOPDIR}/World ${TOPDIR}/PHY)

SET(TUNNEL_SOURCES TunnelNet PrefixedTunnelNet)
CREATE_SIMULATION(TunnelNet TunnelNet_ned_includes "${TUNNEL_SOURCES}")
COPY_MISSING_CONFIG_FILES(PrefixedTunnelNet.xml)

ADD_TEST(TunnelNetR1 TunnelNet -r 1)
ADD_TEST(TunnelNetR2 TunnelNet -r 2)

##---------------------------Development---s------------------------------------
#perl -i -pwe 's|SUBDIRS(Examples)|#SUBDIRS(Examples)|' CMakeLists.txt\;
ADD_CUSTOM_TARGET(top cd ${PROJECT_BINARY_DIR}\; make)
