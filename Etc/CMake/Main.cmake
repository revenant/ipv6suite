# -*- CMAKE -*-
# prebuild target to execute before build requires
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
SET(CMAKE_BACKWARDS_COMPATIBILITY 2.0 CACHE STRING "2.4 uses default path before our custom path for omnet libs unless we do NO_DEFAULT_PATH in every FIND_LIBRARY" FORCE)

SET(OPP_USE_TK OFF CACHE BOOL "OFF unless you are sure tk gui can build")
SET(OPP_USE_MPI OFF CACHE BOOL "OFF unless you are sure lam libs are not 64bit")

OPTION(BUILD_SHARED_LIBS "Build with shared libraries." ON)
SET(ONE_BIG_EXE ON)

SET(MISCDIR ${PROJECT_SOURCE_DIR}/Etc)
SET(SCRIPTDIR ${MISCDIR}/scripts)
SET(BINMISCDIR ${PROJECT_BINARY_DIR}/Etc)
SET(BINSCRIPTDIR ${BINMISCDIR}/scripts)
SET(DUMMY_SOURCE ${BINMISCDIR}/dummy.cc)

STRING(COMPARE NOTEQUAL ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR} OUTOFSOURCE)

INCLUDE(${CMAKEFILES_PATH}/Options.cmake)

INCLUDE(${CMAKEFILES_PATH}/IntelICCOptions.cmake)

#INCLUDE(${CMAKE_ROOT}/Modules/Dart.cmake)

INCLUDE(${CMAKEFILES_PATH}/Configure.cmake)

INCLUDE(${CMAKEFILES_PATH}/Macros.cmake)

IF(OUTOFSOURCE)
#Create Research, Etc, boost links 
  ADD_CUSTOM_TARGET( prebuild ALL rm -fr  ${PROJECT_BINARY_DIR}/Etc ${PROJECT_BINARY_DIR}/Research ${PROJECT_BINARY_DIR}/boost 
		   COMMAND ln -sf ${PROJECT_SOURCE_DIR}/Etc ${PROJECT_BINARY_DIR}/Etc
		   COMMAND ln -sf ${PROJECT_SOURCE_DIR}/Research ${PROJECT_BINARY_DIR}/Research
		   COMMAND ln -sf ${PROJECT_SOURCE_DIR}/boost ${PROJECT_BINARY_DIR}/boost
		   COMMENT "Creating symbolic links for OutofSource builds"
		   WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
		   VERBATIM)
ENDIF(OUTOFSOURCE)

ADD_CUSTOM_TARGET( hold ALL echo holding jobs
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  COMMENT "Pause jobs waiting in queue while building" 
  VERBATIM)

INCLUDE(${CMAKEFILES_PATH}/LinkLibraries.cmake)

#mainly for the doc targets
INCLUDE(${CMAKEFILES_PATH}/CustomTargets.cmake)

ADD_DEFINITIONS(-DBOOST_WITH_LIBS -DWITH_IPv6 -DUSE_MOBILITY -DFASTRS -DFASTRA -DUSE_HMIP -DEDGEHANDOVER=1)
 
INCLUDE_DIRECTORIES(${OPP_INCLUDE_PATH})
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})
#By default on Linux env this is usually same as /usr/include, but just in case
INCLUDE_DIRECTORIES(${BOOSTROOT})

IF(WIN32)
  #include windows specific includes/lib dirs part of SDK and
  #VC (run vcvars32.bat in console before cmakesetup)
  INCLUDE_DIRECTORIES($ENV{INCLUDE})
  FIND_LIBRARY(USER32 wsock32 $ENV{LIB})
  ADD_DEFINITIONS(/NODEFAULTLIB:MSVCRTD /NODEFAULTLIB:LIBCMT)
ENDIF(WIN32)
