# -*- CMAKE -*-

#hide to reduce clutter and confusion (BUILD_DOXYGEN is used with Dart)
MARK_AS_ADVANCED(FORCE CMAKE_INSTALL_PREFIX BUILD_TESTING
  CMAKE_BACKWARDS_COMPATIBILITY BUILD_DOXYGEN BUILD_SHARED_LIBS)

IF(CMAKE_C_COMPILER MATCHES "gcc") #or c\\+\\+/g\\+\\+
  OPTION(FORCE_32BIT "Force 32bit binary compatibile output on 64bit architectures (results of sim exactly same as on 32bit archs)" OFF)
  IF(FORCE_32BIT)
    SET(CMAKE_CXX_FLAGS -m32 CACHE STRING "build 32bit" FORCE)
    SET(CMAKE_SHARED_LINKER_FLAGS -m32 CACHE STRING "build 32bit" FORCE)
  ENDIF(FORCE_32BIT)
ENDIF(CMAKE_C_COMPILER MATCHES "gcc")

OPTION(USER_RELEASE "Denotes a public release. On for users and off for developers." ON)
IF(NOT USER_RELEASE)
  OPTION(CUSTOM_BUILD "Builds only user defined Example subdirectories specified in EXAMPLE_DIRS. Use target buildExamples to build" OFF)
  IF(CUSTOM_BUILD)
    SET(EXAMPLE_DIRS "EthNetwork PingNetwork TestNetwork TunnelNet MIPv6Network HMIPv6Network WirelessEtherNetwork WirelessEtherNetworkDual" 
      CACHE STRING "Space separated list of Example subdirs to build")
  ENDIF(CUSTOM_BUILD)

  OPTION(ICC_LIBCWD_DEBUG_HACK "Convert Dout calls to cerr?" OFF)
  MARK_AS_ADVANCED(ICC_LIBCWD_DEBUG_HACK)
  IF(ICC_LIBCWD_DEBUG_HACK)
    SET(DOUT_HACK -D"Dout\(a,b\)=std::cerr<<b<<endl" -D"Debug\(x\)=x")
  ENDIF(ICC_LIBCWD_DEBUG_HACK)
ENDIF(NOT USER_RELEASE)

MARK_AS_ADVANCED(FORCE USER_RELEASE)
OPTION(BUILD_SHARED_LIBS "Build with shared libraries." ON)

OPTION(BUILD_MOBILITY "Build with Mobility support. make clean after toggling this option" OFF)
IF(BUILD_MOBILITY)
  SET(BUILD_MOBILITY_DEFINE "USE_MOBILITY" )
  ADD_DEFINITIONS(-D${BUILD_MOBILITY_DEFINE})
ENDIF(BUILD_MOBILITY)

OPTION(BUILD_HMIP "Build with HMIP support. make clean after toggling this option" OFF)
IF(BUILD_HMIP AND BUILD_MOBILITY)
  SET(BUILD_HMIP_DEFINE "USE_HMIP")
  ADD_DEFINITIONS(-D${BUILD_HMIP_DEFINE})
ENDIF(BUILD_HMIP AND BUILD_MOBILITY)
IF(BUILD_HMIP)
IF(NOT BUILD_MOBILITY)
  MESSAGE(SEND_ERROR "Please turn on BUILD_MOBILITY in conjunction with BUILD_HMIP")
ENDIF(NOT BUILD_MOBILITY)
ENDIF(BUILD_HMIP)

OPTION(EWU_L2TRIGGER "Link up Trigger for MIPv6. make clean after toggling this option" OFF)
IF(EWU_L2TRIGGER AND BUILD_MOBILITY)
  SET(BUILD_L2TRIGGER_DEFINE "EWU_L2TRIGGER")
  ADD_DEFINITIONS(-D${BUILD_L2TRIGGER_DEFINE})
ENDIF(EWU_L2TRIGGER AND BUILD_MOBILITY)
IF(EWU_L2TRIGGER)
IF(NOT BUILD_MOBILITY)
  MESSAGE(SEND_ERROR "Please turn on BUILD_MOBILITY in conjunction with EWU_L2TRIGGER")
ENDIF(NOT BUILD_MOBILITY)
ENDIF(EWU_L2TRIGGER)

OPTION(EWU_MACBRIDGE "Link Layer Assisted Mobile IP Fast handoff Method" OFF)
IF(EWU_MACBRIDGE AND BUILD_MOBILITY)
  SET(BUILD_MACBRIDGE_DEFINE "EWU_MACBRIDGE")
  ADD_DEFINITIONS(-D${BUILD_MACBRIDGE_DEFINE})
ENDIF(EWU_MACBRIDGE AND BUILD_MOBILITY)
IF(EWU_MACBRIDGE)
IF(NOT BUILD_MOBILITY)
  MESSAGE(SEND_ERROR "Please turn on BUILD_MOBILITY in conjunction with EWU_MACBRIDGE")
ENDIF(NOT BUILD_MOBILITY)
ENDIF(EWU_MACBRIDGE)

OPTION(JLAI_FASTRA "Fast Router Advertisements: To take effect XML var MaxFastRAS > 0 " ON)
IF(JLAI_FASTRA)
  SET(BUILD_FASTRA_DEFINE "1")
ELSE(JLAI_FASTRA)
  SET(BUILD_FASTRA_DEFINE "0")
ENDIF(JLAI_FASTRA)

OPTION(EWU_FASTRS "Fast Router Solicitation:  To take effect XML var HostMaxRtrSolDelay = 0 " ON)
IF(EWU_FASTRS)
  SET(BUILD_FASTRS_DEFINE "1")
ELSE(EWU_FASTRS)
  SET(BUILD_FASTRS_DEFINE "0")
ENDIF(EWU_FASTRS)

OPTION(JLAI_ODAD "Optimistic DAD support always compiled in. XML var optimisticDAD toggles." ON)
IF(JLAI_ODAD)
  SET(BUILD_OPTIMISTIC_DAD_DEFINE "1")
ELSE(JLAI_ODAD)
  SET(BUILD_OPTIMISTIC_DAD_DEFINE "0")
ENDIF(JLAI_ODAD)
MARK_AS_ADVANCED(FORCE JLAI_ODAD)

OPTION(JLAI_EH "Edge Handover support" OFF)
IF(JLAI_EH AND BUILD_HMIP)
  SET(BUILD_EDGEHANDOVER_DEFINE "1")
ELSE(JLAI_EH AND BUILD_HMIP)
  SET(BUILD_EDGEHANDOVER_DEFINE "0")
  IF(JLAI_EH)
    MESSAGE(SEND_ERROR "Please turn on BUILD_HMIP first if you want JLAI_EH")
  ENDIF(JLAI_EH)
ENDIF(JLAI_EH AND BUILD_HMIP)

OPTION(SWOON_L2FUZZYHO "IEEE802.11b Link Layer Fuzzy Logic Handover" OFF)
IF(SWOON_L2FUZZYHO)
  SET(BUILD_L2FUZZYHO_DEFINE "1")
ELSE(SWOON_L2FUZZYHO)
  SET(BUILD_L2FUZZYHO_DEFINE "0")
ENDIF(SWOON_L2FUZZYHO)

OPTION(SWOON_L2FUZZYHO "IEEE802.11b Link Layer Fuzzy Logic Handover" OFF)
IF(SWOON_L2FUZZYHO)
  SET(BUILD_L2FUZZYHO_DEFINE "1")
ELSE(SWOON_L2FUZZYHO)
  SET(BUILD_L2FUZZYHO_DEFINE "0")
ENDIF(SWOON_L2FUZZYHO)

OPTION(WCHEN_MLDV2 "MLDv2 and custom L2 trigger(sendGQtoUpperLayer)" OFF)
IF(WCHEN_MLDV2)
  SET(BUILD_MLDV2_DEFINE "1")
ELSE(WCHEN_MLDV2)
  SET(BUILD_MLDV2_DEFINE "0")
ENDIF(WCHEN_MLDV2)

OPTION(BUILD_DEBUG "Build with support for debugging via gdb. make clean after toggling this option" ON)
IF(NOT WIN32)
IF(BUILD_DEBUG)
  ADD_DEFINITIONS(-UNDEBUG)
  ADD_DEFINITIONS( -fno-inline -pipe) 
  IF(CMAKE_CXX_COMPILER MATCHES "icpc")
    ADD_DEFINITIONS(-g)
  ELSE(CMAKE_CXX_COMPILER MATCHES "icpc")    
      ADD_DEFINITIONS(-ggdb3)
      SET(CMAKE_CXX_FLAGS -m32 CACHE STRING "build 32bit")
      SET(CMAKE_SHARED_LINKER_FLAGS -m32 CACHE STRING "build 32bit")    
  ENDIF(CMAKE_CXX_COMPILER MATCHES "icpc")
ELSE(BUILD_DEBUG)
  ADD_DEFINITIONS(-O2 -DNDEBUG -pipe)
ENDIF(BUILD_DEBUG)
ENDIF(NOT WIN32)

#Used to modify macros of RNG functions so we can use akaroa
#Its a hack until akaroa is properly integrated into omnet
IF(OPP_USE_AKAROA)
  SET(USE_AKAROA_DEFINE "1")
ELSE(OPP_USE_AKAROA)
  SET(USE_AKAROA_DEFINE "0")
ENDIF(OPP_USE_AKAROA)


OPTION(VERBOSE_DEBUG "DEPRECATED Build with extra debug messages (Use LIBCWD_DEBUG instead" OFF)
IF(VERBOSE_DEBUG)
  ADD_DEFINITIONS( -DTESTIPv6 )
  IF(BUILD_MOBILITY)
    ADD_DEFINITIONS(-DTESTMIPv6)
  ENDIF(BUILD_MOBILITY)
ENDIF(VERBOSE_DEBUG)
MARK_AS_ADVANCED(FORCE VERBOSE_DEBUG)

OPTION(BUILD_UNITTESTS "Build with simple regression tests (requires CPPUnit)" OFF)
IF(BUILD_UNITTESTS)
  SET(BUILD_UNITTESTS_DEFINE "USE_CPPUNIT")
  ADD_DEFINITIONS( -D${BUILD_UNITTESTS_DEFINE})
ENDIF(BUILD_UNITTESTS)

OPTION(LIBCWD_DEBUG "Build with libcwd debug streams support. make clean after toggling this option")
IF(LIBCWD_DEBUG)
  ADD_DEFINITIONS( -DCWDEBUG)
  IF(ARCH64)
    ADD_DEFINITIONS(-DPLATFORM64bit=1 -DCWDEBUG_ALLOC=1)  
  ENDIF(ARCH64)
ENDIF(LIBCWD_DEBUG)

OPTION(BUILD_UML "Build User Mode Linux mixed mode example. make clean after toggling this option" OFF)
IF(BUILD_UML)
  ADD_DEFINITIONS(-DBUILD_UML_INTERFACE)
ENDIF(BUILD_UML)
MARK_AS_ADVANCED(FORCE BUILD_UML)

OPTION(BUILD_TOPOLOGY "Build with Topology Generater (Alpha)" OFF)
MARK_AS_ADVANCED(FORCE BUILD_TOPOLOGY)

OPTION(BUILD_DOCUMENTATION "Adds extra targets like dox,cl2html etc. see DocTargets.cmake" OFF)

OPTION(USE_XMLWRAPP "Build xmlwrapp (Deprecated. Upgrade omnet to use its parser)" OFF)
MARK_AS_ADVANCED(FORCE USE_XMLWRAPP)

OPTION(USE_XERCES "UNSUPPORTED!" OFF)
MARK_AS_ADVANCED(USE_XERCES)
IF(USE_XERCES)
  SET(USE_XERCES_DEFINE "USE_XERCES")
  ADD_DEFINITIONS(-D${USE_XERCES_DEFINE})
  MESSAGE(FATAL_ERROR "Please turn OFF. Xerces-C is not supported now.")
ENDIF(USE_XERCES)

OPTION(EARLY_LIBCWD "Turn Malloc debugging on. Requires omnetpp to have libcwd probes inserted" OFF)
MARK_AS_ADVANCED(FORCE EARLY_LIBCWD)

OPTION(REVERT_QUEUE "Use activity in En/DeQueue to prove that Ethernet is at fault Affects only IP/IPv4/MAC_LLC/Enqueue.h & Dequeue.h" OFF)
MARK_AS_ADVANCED(FORCE REVERT_QUEUE)
IF(REVERT_QUEUE)
  ADD_DEFINITIONS(-DQUEUE_ACTIVITY)
ENDIF(REVERT_QUEUE)

IF(NOT BUILD_SHARED_LIBS)
  OPTION(ONE_BIG_EXE "Compile all sources into one big executable instead of a library per subdirectory. Useful when building static to prevent missing weak linkage symbols like ctors" ON)
ENDIF(NOT BUILD_SHARED_LIBS)

## Variables used by other cmake files

IF(NOT WIN32)
  SET(COMPILER_WARNINGS -Wall CACHE STRING "compiler flags for warnings")
ELSE(NOT WIN32)
  #-Wall takes forever on vsxpress2005
  SET(COMPILER_WARNINGS /W1 CACHE STRING "compiler flags for warnings")
ENDIF(NOT WIN32)

SET(SOURCE_EXTENSION "cc")

################################ END USER OPTIONS ###############################
#Do not not know what SOURCE_GROUP is supposed to achieve
#SOURCE_GROUP(CXX_SRCS "[^_][^n][.]cc$")
#SET_SOURCE_FILES_PROPERTIES(${CXX_SRCS} COMPILE_FLAGS -Wall)
#SOURCE_GROUP(NEDC_SRCS "_n[.]cc$")
#SET_SOURCE_FILES_PROPERTIES(${NEDC_SRCS} COMPILE_FLAGS -Wno-unused)

#ADD_DEFINITIONS( -DXML_ADDRESSES -DROUTING -DPREFIXTIMER -DADDRESSTIMER -DADDR_RES -DUSE_MOBILITY   -DCWDEBUG)
