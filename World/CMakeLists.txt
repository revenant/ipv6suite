##World

#CMake Bug needs .cc in this case as cmake macro generated _n.cc seems to shadow this
SET(WORLD_SRCS WorldProcessor.cc) 

OPP_WRAP_NEDC(World WORLD_NED_INCLUDES WorldProcessor.ned)

INCLUDE_DIRECTORIES(${TOPDIR}/IP/DualStack ${TOPDIR}/IP/IPv6/Generic
${TOPDIR}/Util/XML ${IPv6Suite_SOURCE_DIR}/Util ${IPv6Suite_SOURCE_DIR}/Util/Loki ${TOPDIR}/PHY ${TOPDIR}/NetworkInterfaces/Wireless ${TOPDIR}/NetworkInterfaces ${TOPDIR}/NetworkInterfaces/Ethernet)

IF(BUILD_MOBILITY)
  INCLUDE_DIRECTORIES(${TOPDIR}/NetworkInterfaces/Wireless)
  INCLUDE_DIRECTORIES(${TOPBINDIR}/NetworkInterfaces)
ENDIF(BUILD_MOBILITY)

RELEASE_SOURCES("${WORLD_SRCS}")
ADD_LIBRARY(World ${WORLD_SRCS}  ${GENERATED_NEDC_FILES})
