SET(VIDEOSTREAM_SRCS UDPVideoStream.cc UDPVideoStreamSvr.cc)
#Does not work. We cannot have a define like as well as another simple ned for
#extra ned parameters
#  OPP_WRAP_NEDC(VideoStream VideoStream_NED_INCLUDES UDPVideoStream.ned)

RELEASE_SOURCES("${VIDEOSTREAM_SRCS}")
ADD_LIBRARY(VideoStream ${VIDEOSTREAM_SRCS} ${GENERATED_NEDC_FILES})
INCLUDE_DIRECTORIES(${TOPDIR}/Util ${TOPDIR}/IP/DualStack 
    ${TOPDIR}/IP/IPv6/Generic ${TOPDIR}/Transport/UDP
    ${PROJECT_BINARY_DIR}/Transport/UDP)
