INCLUDE(${CMAKEFILES_PATH}/LinkLibraries.cmake)

RELEASE_FILES("dummy")

IF(CUSTOM_BUILD)
  IF(${MakeExample} MATCHES "^do$")
    #Convert space separated to ; sep for use in foreach
    STRING(REGEX REPLACE "[ ]" ";" RESULT "${EXAMPLE_DIRS}")
    FOREACH(dir ${RESULT})
      SUBDIRS(${dir})
    ENDFOREACH(dir)
  ENDIF(${MakeExample} MATCHES "^do$")
ELSE(CUSTOM_BUILD)

##Examples
SUBDIRS(EthNetwork MelbourneNetwork PingNetwork
  TestNetwork TunnelNet) #3HostLAN SingleRouter 2RouterNet)

IF(BUILD_MOBILITY)
  SUBDIRS(WirelessEtherNetwork WirelessEtherNetwork2 MIPv6Network WirelessTest WirelessEtherNetworkDual)
ENDIF(BUILD_MOBILITY)

IF(BUILD_MOBILITY AND BUILD_HMIP)
  SUBDIRS(HMIPv6Network)
ENDIF(BUILD_MOBILITY AND BUILD_HMIP)

IF(BUILD_UML)
  SUBDIRS(UMLNet)
ENDIF(BUILD_UML)
ENDIF(CUSTOM_BUILD)
