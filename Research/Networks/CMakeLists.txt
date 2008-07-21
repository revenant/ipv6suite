IF(OUTOFSOURCE)
#create symlinks in subdirs to executable in ${PROJECT_BINARY_DIR}
  ADD_CUSTOM_TARGET( linkfiles ALL ${RUBY_EXECUTABLE} opplinkSubdirs.rb
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Research/Networks)
  ADD_DEPENDENCIES(linkfiles ${INET})
ENDIF(OUTOFSOURCE)

ADD_CUSTOM_TARGET( unhold ALL echo unholding jobs held during build
  COMMENT "Resuming jobs paused during build"
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  VERBATIM)
ADD_DEPENDENCIES(unhold ${INET})

SUBDIRS(EthNetwork TestNetwork TunnelNet MIPv6Network HMIPv6Network Baseline)

#SUBDIRS(WirelessEtherNetwork WirelessEtherNetwork2 WirelessTest WirelessEtherNetworkDual)
