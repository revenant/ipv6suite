##PHY
SET(PHYLAYER_SRCS PHYSimple.cc)

SET(WIRELESS_ENV_SRCS PHYWirelessSignal.cc Entity.cc MobileEntity.cc MobileBaseStation.cc MobilityHandler.cc MobilityStatic.cc MobilityRandomWalk.cc MobilityRandomWP.cc MobilityRandomPattern.cc SingletonRandomPattern.cc)

OPP_WRAP_NEDC(PHY . PhysicalLayer.ned MobilityManager.ned)

IF(BUILD_MOBILITY)
  SET(PHYLAYER_SRCS ${PHYLAYER_SRCS} ${WIRELESS_ENV_SRCS})
  INCLUDE_DIRECTORIES(	${CMAKE_CURRENT_SOURCE_DIR}
			${TOPBINDIR}/NetworkInterfaces 
			${TOPDIRONE}/NetworkInterfaces/Wireless 
			${TOPDIRONE}/NetworkInterfaces/Ethernet)
ENDIF(BUILD_MOBILITY)

RELEASE_SOURCES("${PHYLAYER_SRCS}")
ADD_LIBRARY(PHY ${PHYLAYER_SRCS}  ${GENERATED_NEDC_FILES})

INCLUDE_DIRECTORIES(${TOPDIRONE}/NetworkInterfaces ${TOPDIRONE}/Util ${TOPDIRONE}/Util/adHocSim/h ${TOPDIRONE}/World ${TOPDIRONE}/IP/DualStack ${TOPDIRONE}/IP/IPv6/Generic)
