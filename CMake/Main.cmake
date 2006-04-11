# -*- CMAKE -*-


SET(MISCDIR ${PROJECT_SOURCE_DIR}/Etc)
SET(SCRIPTDIR ${MISCDIR}/scripts)
SET(BINMISCDIR ${PROJECT_BINARY_DIR}/Etc)
SET(BINSCRIPTDIR ${BINMISCDIR}/scripts)
SET(DUMMY_SOURCE ${BINMISCDIR}/dummy.cc)

#INLINENET is used to test building for inline example subdirs or user network
#directory using the self built and installed ipv6suite-xxx.ix86.rpm 

STRING(COMPARE EQUAL ${CMAKE_HOME_DIRECTORY} ${IPv6Suite_SOURCE_DIR}  INLINENET)

INCLUDE(${CMAKEFILES_PATH}/Options.cmake)

INCLUDE(${CMAKEFILES_PATH}/IntelICCOptions.cmake)

INCLUDE(${CMAKE_ROOT}/Modules/Dart.cmake)

INCLUDE(${CMAKEFILES_PATH}/Configure.cmake)

INCLUDE(${CMAKEFILES_PATH}/Macros.cmake)

IF(INLINENET)
  INCLUDE(${CMAKEFILES_PATH}/RunOnce.cmake)
ENDIF(INLINENET)

IF(INLINENET)
  INCLUDE(${CMAKEFILES_PATH}/CustomTargets.cmake)
  
  INCLUDE(${CMAKEFILES_PATH}/ExportIPv6Suite.cmake)
ENDIF(INLINENET)
