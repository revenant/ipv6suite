IF(OUTOFSOURCE)
#create symlinks in subdirs to executable in ${PROJECT_BINARY_DIR}
  ADD_CUSTOM_TARGET( linkfiles ALL ${RUBY_EXECUTABLE} opplinkSubdirs.rb
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Research/Networks)
  ADD_DEPENDENCIES(linkfiles ${INET})
ENDIF(OUTOFSOURCE)

SUBDIRS(EthNetwork TestNetwork TunnelNet MIPv6Network HMIPv6Network Baseline)

#SUBDIRS(WirelessEtherNetwork WirelessEtherNetwork2 WirelessTest WirelessEtherNetworkDual)
