# -*- CMAKE -*-
##---- "Libraries i.e. -L and -l flag settings for building IPv6Suite networks"-
IF(NOT ONE_BIG_EXE)
  SET(IPV6SUITE_LIBS Nodes TCP Interface IPv6Processing XML DualStack Util
    NetworkInterfaces VideoStream Ping6 UDP QoS MAC_LLC IPProcessing World PHY)

  LINK_DIRECTORIES(${IPv6Suite_BINARY_DIR}/lib)

  LINK_LIBRARIES( ${IPV6SUITE_LIBS} )

ENDIF(NOT ONE_BIG_EXE)

IF(USE_XMLWRAPP)
  IF(XMLWRAPP_FOUND)
    LINK_LIBRARIES(${XMLWRAPP_LIBRARY} ${BOOST_LIB_REGEX})
  ENDIF(XMLWRAPP_FOUND)
ENDIF(USE_XMLWRAPP)
IF(USE_XERCES)
  LINK_DIRECTORIES(${XERCESROOT}/lib)
  LINK_LIBRARIES( ${XERCES_LIBRARY} )
ENDIF(USE_XERCES)

IF(BUILD_UNITTESTS)
  IF(CPPUNIT_FOUND)
    LINK_LIBRARIES( ${CPPUNIT_LIBRARY} )
  ENDIF(CPPUNIT_FOUND)
ENDIF(BUILD_UNITTESTS)

IF(LIBCWD_DEBUG)
  IF(LIBCWD_FOUND)
    LINK_LIBRARIES( ${LIBCWD_LIBRARY})
  ENDIF(LIBCWD_FOUND)
ENDIF(LIBCWD_DEBUG)

#SET(SYS_LIBS m)
#LINK_LIBRARIES(${SYS_LIBS})

#-laudiofile for RTP
LINK_LIBRARIES(-lboost_signals)

LINK_LIBRARIES( -lstdc++)
