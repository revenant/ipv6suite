#LINK_DIRECTORIES(${XERCESROOT}/lib)
#LINK_LIBRARIES( ${XERCES_LIBRARY} )

IF(BUILD_UNITTESTS)
  LINK_LIBRARIES( cppunit )
ENDIF(BUILD_UNITTESTS)

IF(LIBCWD_DEBUG)
  LINK_LIBRARIES( cwd Channels)
ENDIF(LIBCWD_DEBUG)

FIND_PATH(XMLPP libxml++/libxml++.h
		/usr/include/libxml++-1.0
		DOC "Path to libxml++ v0.18 base dir for header file libxml++.h")

FIND_PATH(LIBXML2 libxml/parser.h
		    /usr/include
		    /usr/include/libxml2
		    /usr/local/include
		    DOC "Path to libxml2 base dir for header file parser.h")

INCLUDE_DIRECTORIES(${LIBXML2} ${XMLPP})
SET(IP6DIR ${TOPDIR}/IP/IPv6/Generic)
INCLUDE_DIRECTORIES(${IP6DIR} ${TOPDIR}/Util)

IF(CMAKE_CXX_COMPILER MATCHES "g\\+\\+")
  #Required for gcc 2.96
  ADD_DEFINITIONS(-ftemplate-depth-100)
ENDIF(CMAKE_CXX_COMPILER MATCHES "g\\+\\+")

SET(SYS_LIBS dl m )
LINK_LIBRARIES(${SYS_LIBS})

ADD_EXECUTABLE(XMLTopoGen TopologyGenerator XMLWriterHandler ${IP6DIR}/ipv6_addr.cc)
TARGET_LINK_LIBRARIES(XMLTopoGen xml++-0.1 bgl-viz)

SET(CWD ${IPv6Suite_SOURCE_DIR}/Util/Topology)

#Test dynamic generation of modules
SET( DynamicTopology_ned_includes ${TOPDIR}/NetComponents/Nodes ${TOPDIR}/IP/DualStack
${TOPDIR}/NetworkInterfaces ${TOPDIR}/Transport/TCP ${TOPDIR}/Transport/UDP
${TOPDIR}/Applications/Ping6 ${TOPDIR}/IP/IPv4/MAC_LLC ${TOPDIR}/IP/IPv6/IPv6Processing
${TOPDIR}/IP/IPv4/QoS ${TOPDIR}/IP/IPv4/IPProcessing ${TOPDIR}/World ${CWD})
OPP_WRAP_NEDC(CQN DynamicTopologyBuider_ned_includes DynamicTopologyBuilder.ned)


INCLUDE_DIRECTORIES(${TOPDIR}/Util/World ${TOPDIR}/NetworkInterfaces/Wireless
${TOPDIR}/Util/XML ${TOPDIR}/Util/Loki)

SET(TOPOBUILDER_SRCS DynamicTopologyBuilder DynamicTopologyBuilder_n.cc)

#ADD_DEFINITIONS(-DGRAPHVIZ_GRAPH=GraphvizGraph)
#AUX_SOURCE_DIRECTORY(graphviz TOPOBUILDER_SRCS)

#OPP_WRAP_NEDC(DynamicTopology DynamicTopology_ned_includes DynamicTopology.ned)
#ADD_EXECUTABLE(DynamicTopology DynamicTopology_n.cc ${TOPOBUILDER_SRCS})
#LINK_DIRECTORIES(${XERCESROOT}/lib)
#TARGET_LINK_LIBRARIES(DynamicTopology ${IPV6SUITE_LIBS} ${XERCES_LIBRARY}
#${OPP_LIBRARIES} bgl-viz boost_regex)


#Non IPv6Suite simulations
SET( AntNet_ned_includes ${CWD})
OPP_WRAP_NEDC(AntNetwork AntNet_ned_includes AntNetNodes.ned AntNet.ned)

ADD_EXECUTABLE(AntNetwork  ${TOPOBUILDER_SRCS} gen router sink AntNet_n.cc AntNetNodes_n.cc)
TARGET_LINK_LIBRARIES(AntNetwork ${OPP_LIBRARIES} bgl-viz boost_regex)

SET(CQN_ned_includes ${CWD})
OPP_WRAP_NEDC(CQN CQN_ned_includes ClosedQueueingNetwork.ned)
AUX_SOURCE_DIRECTORY(${TOPDIR}/../cqnf CQN_SRCS)
INCLUDE_DIRECTORIES(${IPv6Suite_SOURCE_DIR}/../cqnf)

ADD_EXECUTABLE(CQN ${TOPOBUILDER_SRCS} ClosedQueueingNetwork_n.cc ${CQN_SRCS}  )
TARGET_LINK_LIBRARIES(CQN ${OPP_LIBRARIES} bgl-viz boost_regex)


SOURCE_GROUP(CXX_SRCS "[^_][.]cc$")
SET_SOURCE_FILES_PROPERTIES(${CXX_SRCS} COMPILE_FLAGS -Wall)

SET_SOURCE_FILES_PROPERTIES(${GENERATED_NEDC_FILES}  GENERATED COMPILE_FLAGS -Wno-unused)
