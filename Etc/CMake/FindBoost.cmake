# -*- CMAKE -*-
FIND_PATH(BOOSTROOT boost/utility.hpp 
		    $ENV{BOOSTROOT} 
		    /usr/include
		    /usr/local/include
		    /opt
		    /opt/include
		    ${PROJECT_SOURCE_DIR}/../boost
		    DOC "Path to Boost directory")

FIND_PATH(TEST_BOOST_PATH boost/utility.hpp
	  ${BOOSTROOT}
	  /usr/include
	  /usr/local/include
	  /opt/include
	  ../boost_1_29_0
	  ../boost_1_28_0
	  ../boost_1_27_0
	  ../boost
)

IF(NOT TEST_BOOST_PATH)
  MESSAGE(SEND_ERROR "Please set the option BOOSTROOT to correct value")
ELSE(NOT TEST_BOOST_PATH)
  IF(BOOSTROOT)
    IF(TEST_BOOST_PATH MATCHES ${BOOSTROOT})
    ELSE(TEST_BOOST_PATH MATCHES ${BOOSTROOT})
      MESSAGE("Option BOOSTROOT=${BOOSTROOT} is incorrect, changed to ${TEST_BOOST_PATH}")
      SET(BOOSTROOT ${TEST_BOOST_PATH} CACHE INTERNAL "Path to boost directory")  
    ENDIF(TEST_BOOST_PATH MATCHES ${BOOSTROOT})
  ELSE(BOOSTROOT)
    MESSAGE(SEND_ERROR "Please set BOOSTROOT to ${TEST_BOOST_PATH}")
  ENDIF(BOOSTROOT)
ENDIF(NOT TEST_BOOST_PATH)
MARK_AS_ADVANCED(FORCE TEST_BOOST_PATH)

IF(CMAKE_CXX_COMPILER MATCHES "icpc")
  SET(BOOST_LIB_SUFFIX "il")
ELSE(CMAKE_CXX_COMPILER MATCHES "icpc")
  #Assume it is gcc
  SET(BOOST_LIB_SUFFIX "gcc")
ENDIF(CMAKE_CXX_COMPILER MATCHES "icpc")

FIND_LIBRARY(BOOST_LIB_SIGNALS 
    NAMES boost_signals-${BOOST_LIB_SUFFIX} boost_signals 
    PATHS ${BOOSTROOT}/../lib /usr/lib
    DOC "Signals library which replaced Loki for timer messages")

IF(USE_XMLWRAPP)
  FIND_LIBRARY(BOOST_LIB_REGEX 
    NAMES boost_regex-${BOOST_LIB_SUFFIX} boost_regex 
    PATHS ${BOOSTROOT}/../lib 
    DOC "Regular expressions. Boost >=1.31 has compiler suffix for library names")
ENDIF(USE_XMLWRAPP)
