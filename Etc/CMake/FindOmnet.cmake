# -*- CMAKE -*-
# Find the native FLTK includes and library
#
# OPP_NEDC, where to find nedc or nedtool binary
# OPP_WRAP_NEDC, This allows the OPP_WRAP_NEDC to work
# OPP_BASE_DIR, if src of omnetpp installed
# OPP_INCLUDE_DIR, where to find include files 
# OPP_LIBRARY_PATH, where to look for Library file
# OPP_LIBRARIES, list of OPP libraries to link with
# OPP_CMDENV_GUI_LIBRARY, full path to cmdenv lib
# OPP_TCL_GUI_LIBRARY, full path to Tk GUI lib
# OPP_ENVIR_LIBRARY, where to look for envir lib

FIND_PATH(OPP_BASE_DIR include/omnetpp.h
		       ${PROJECT_SOURCE_DIR}/../omnetpp
		       /usr
		       /usr/local
		       /opt
		       DOC "Path to OMNeT++")

FIND_PROGRAM(OPP_NEDC nedc ${OPP_BASE_DIR}/bin DOC "Full Path of nedc")

IF(OPP_BASE_DIR MATCHES NOTFOUND)
  IF(EXISTS ${OPP_NEDC})
    GET_FILENAME_COMPONENT(OPP_BASE ${OPP_NEDC} PATH)
    GET_FILENAME_COMPONENT(OPP_BASE_DIR_TMP ${OPP_BASE} PATH)
    SET(OPP_BASE_DIR ${OPP_BASE_DIR_TMP} CACHE INTERNAL "Base installation of omnetpp")
  ELSE(EXISTS ${OPP_NEDC})
    MESSAGE(FATAL_ERROR "Cannot find OMNeT++ nedc program")
  ENDIF(EXISTS ${OPP_NEDC})
ENDIF(OPP_BASE_DIR MATCHES NOTFOUND)

FIND_PROGRAM(OPP_MSGC opp_msgc ${OPP_BASE_DIR}/bin DOC "Full path to opp_msgc")
  IF(EXISTS ${OPP_MSGC})
  ELSE(EXISTS ${OPP_MSGC})
    MESSAGE(FATAL_ERROR "Cannot find OMNeT++ opp_msgc program")
  ENDIF(EXISTS ${OPP_MSGC})

FIND_PROGRAM(OPP_TEST opp_test ${OPP_BASE_DIR}/bin DOC "Full path to opp_test")
IF(EXISTS ${OPP_TEST})
ELSE(EXISTS ${OPP_TEST})
  MESSAGE(FATAL_ERROR "Cannot find OMNeT++ opp_test program")
ENDIF(EXISTS ${OPP_TEST})

FIND_PATH(OPP_INCLUDE_PATH omnetpp.h
			     ${OPP_BASE_DIR}/include/omnetpp
			     /usr/include/omnetpp
			     /usr/local/include/omnetpp
			     /opt/include/omnetpp
)

IF(NOT OPP_INCLUDE_PATH)
  MESSAGE(FATAL_ERROR "Cannot find OMNeT++ include directory")
ENDIF(NOT OPP_INCLUDE_PATH)

#Bad idea as version number changed between beta releases till final
#EXEC_PROGRAM(grep ${OPP_INCLUDE_PATH}  ARGS "\"VERSION 0x0300\" defs.h"
#  RETURN_VALUE OPP_VERSION3_TEST_VAR)

OPTION(OPP_VERSION3_TEST "OMNet++ 3 compatibility mode. Turn this off to use opp 2.3 (UNSUPPORTED)" ON)
MARK_AS_ADVANCED(OPP_VERSION3_TEST)

IF(OPP_VERSION3_TEST)
  #We really need to use Tcl/Tk 8.4 for omnetpp 3 otherwise
  #gned does not display network at all.
  INCLUDE(${CMAKEFILES_PATH}/FindTCL.cmake)
  ADD_DEFINITIONS(-DOPP_VERSION=3)
  SET(USE_XMLWRAPP OFF CACHE BOOL "Build xmlwrapp (DO NOT TURN THIS ON)" FORCE)
  FIND_PROGRAM(OPP_NEDTOOL nedtool ${OPP_BASE_DIR}/bin DOC "Full Path of nedtool")
  IF(EXISTS ${OPP_NEDTOOL})
    #legacy ipsuite requires reording of ned module decl so will leave this for now until we migrate to INETFramework
    #SET(OPP_NEDC "${OPP_NEDTOOL}" CACHE PATH "nedc is no longer maintained" FORCE)
  ELSE(EXISTS ${OPP_NEDTOOL})
    MESSAGE(FATAL_ERROR "Cannot find OMNeT++ nedtool program")
  ENDIF(EXISTS ${OPP_NEDTOOL})

  OPTION(DYNAMIC_NED "DO NOT turn off and back on again. OPP_WRAP_NEDC will do nothing if on." ON)

ELSE(OPP_VERSION3_TEST)
  INCLUDE(${CMAKE_ROOT}/Modules/FindTCL.cmake)
ENDIF(OPP_VERSION3_TEST)
MARK_AS_ADVANCED(TCL_LIBRARY_DEBUG TK_LIBRARY_DEBUG TCL_TCLSH TK_WISH)

IF(TK_LIBRARY)
  OPTION(OPP_USE_TK "Use TK GUI" 1)
ELSE(TK_LIBRARY)
  OPTION(OPP_USE_TK "Use TK GUI" 0)
ENDIF(TK_LIBRARY)

IF(LIBCWD_DEBUG)
  IF(EARLY_LIBCWD)
    SET(LIBCW_EXT "-cw")
  ENDIF(EARLY_LIBCWD)
ENDIF(LIBCWD_DEBUG)

SET(OPP_LIBRARY_PATHS 
  ${OPP_BASE_DIR}/lib 
  /usr/lib /usr/local/lib /usr/local/omnetpp/lib)

FIND_LIBRARY(OPP_ENVIR_LIBRARY envir${LIBCW_EXT}
  PATHS ${OPP_LIBRARY_PATHS}
)

IF(OPP_ENVIR_LIBRARY)
  GET_FILENAME_COMPONENT(OPP_LIBRARY_PATH ${OPP_ENVIR_LIBRARY} PATH CACHE)
ENDIF(OPP_ENVIR_LIBRARY)

FIND_LIBRARY(OPP_CMDENV_GUI_LIBRARY cmdenv
 PATHS ${OPP_LIBRARY_PATHS}
)
 
FIND_LIBRARY(OPP_TCL_GUI_LIBRARY tkenv
 PATHS ${OPP_LIBRARY_PATHS}
)
 
FIND_LIBRARY(OPP_SIMSTD_LIBRARY sim_std
 PATHS ${OPP_LIBRARY_PATHS}
)

FIND_LIBRARY(OPP_SIMMPI_LIBRARY sim_mpi
 PATHS ${OPP_LIBRARY_PATHS}
)

IF(OPP_VERSION3_TEST)
  FIND_LIBRARY(OPP_NEDXML_LIBRARY nedxml
    PATHS ${OPP_LIBRARY_PATHS}
    )

  SET(OPP_NEDXML_LIBRARIES ${OPP_NEDXML_LIBRARY})
  INCLUDE(${CMAKEFILES_PATH}/FindXML.cmake)
  #xmlwrapp has libxml2 as dependency already
  IF(NOT USE_XMLWRAPP)
    SET(OPP_NEDXML_LIBRARIES ${OPP_NEDXML_LIBRARIES} ${XML_LIBRARY})
  ENDIF(NOT USE_XMLWRAPP)
  #For omnetpp to have installed sucessfully expat would have been installed
  MARK_AS_ADVANCED(FORCE OPP_NEDXML_LIBRARY)
ENDIF(OPP_VERSION3_TEST)

IF(OPP_USE_TK)
  SET( OPP_GUI_LIBRARY ${OPP_TCL_GUI_LIBRARY} ) 
ELSE(OPP_USE_TK)
  SET( OPP_GUI_LIBRARY ${OPP_CMDENV_GUI_LIBRARY} )
ENDIF(OPP_USE_TK)
 
OPTION(OPP_USE_MPI "Parallel simulation with MPI" OFF)
IF(OPP_USE_MPI)
  INCLUDE(${CMAKE_ROOT}/Modules/FindMPI.cmake)
  IF(MPI_INCLUDE_PATH)
    #Based on LAM (mpicc -showme)
    SET(MPI_LIBRARIES lamf77mpi ${MPI_LIBRARY} lam util)
    INCLUDE_DIRECTORIES(MPI_INCLUDE_PATH)
  ELSE(MPI_INCLUDE_PATH)
    MESSAGE(FATAL_ERROR "OPP_USE_MPI error: Please specify MPI_INCLUDE_PATH and MPI_LIBRARY directly")
  ENDIF(MPI_INCLUDE_PATH)
  IF(NOT OPP_SIMMPI_LIBRARY)
    MESSAGE(FATAL_ERROR "OPP_SIMMPI_LIBRARY error: Cannot find sim_mpi. Please rebuild OMNeT++ with MPI support first")
  ENDIF(NOT OPP_SIMMPI_LIBRARY)
  IF(OPP_USE_TK)
    MESSAGE(SEND_ERROR "Please disable OPP_USE_TK before attempting to build with MPI")
  ELSE(OPP_USE_TK)
    SET( OPP_SIM_LIBRARY ${OPP_SIMMPI_LIBRARY}) 
  ENDIF(OPP_USE_TK)
ELSE(OPP_USE_MPI)
  SET( OPP_SIM_LIBRARY ${OPP_SIMSTD_LIBRARY})
ENDIF(OPP_USE_MPI)

OPTION(OPP_USE_AKAROA "MRIP with Akaroa" OFF)
#include by default as opp 3 will detect akaroa and depend on it
INCLUDE(${CMAKEFILES_PATH}/FindAkaroa.cmake)
IF(AKAROA_INCLUDE_PATH)
  SET(AKAROA_LIBRARIES ${AKAROA_LIBRARY} args fl)
ENDIF(AKAROA_INCLUDE_PATH)
IF(OPP_USE_AKAROA)
  IF(AKAROA_INCLUDE_PATH)
    INCLUDE_DIRECTORIES(AKAROA_INCLUDE_PATH)
  ELSE(AKAROA_INCLUDE_PATH)
    MESSAGE(SEND_ERROR "OPP_USE_AKAROA error: Please specify AKAROA_INCLUDE_PATH and AKAROA_LIBRARY directly")
  ENDIF(AKAROA_INCLUDE_PATH)
  IF(OPP_USE_TK)
    MESSAGE(SEND_ERROR "Please disable OPP_USE_TK before attempting to use Akaroa")
  ENDIF(OPP_USE_TK)
ENDIF(OPP_USE_AKAROA)

IF(BUILD_SHARED_LIBS) 
  SET(TCLTK_LIBS ${TCL_LIBRARY} ${TK_LIBRARY})
ELSE(BUILD_SHARED_LIBS)
  SET(TCLTK_LIBS ${TK_STUB_LIBRARY} ${TCL_STUB_LIBRARY})
ENDIF(BUILD_SHARED_LIBS)

SET(DL_LIB dl) #needs to be last for static builds

SET( OPP_CMDLIBRARIES ${OPP_ENVIR_LIBRARY} ${OPP_SIMSTD_LIBRARY} ${OPP_NEDXML_LIBRARIES} ${OPP_CMDENV_GUI_LIBRARY} ${AKAROA_LIBRARIES} ${DL_LIB})
#Added akaroa lib dep as opp needs it even if we do not want to use akoutvector
SET( OPP_TKGUILIBRARIES ${OPP_ENVIR_LIBRARY} ${OPP_SIMSTD_LIBRARY} ${OPP_NEDXML_LIBRARIES} ${OPP_TCL_GUI_LIBRARY} ${TCLTK_LIBS} ${AKAROA_LIBRARIES} ${DL_LIB})
SET( OPP_MPILIBRARIES ${OPP_ENVIR_LIBRARY} ${OPP_CMDENV_GUI_LIBRARY} ${OPP_SIMMPI_LIBRARY} ${OPP_NEDXML_LIBRARIES} ${MPI_LIBRARIES} ${AKAROA_LIBRARIES} ${DL_LIB})

SET( OPP_LIBRARIES ${OPP_ENVIR_LIBRARY} ${OPP_GUI_LIBRARY} ${OPP_SIM_LIBRARY} ${OPP_NEDXML_LIBRARIES} ${MPI_LIBRARIES} ${AKAROA_LIBRARIES} ${DL_LIB})

IF(OPP_USE_TK)
  SET(OPP_LIBRARIES ${OPP_LIBRARIES} ${TCLTK_LIBS})
ENDIF(OPP_USE_TK)

MARK_AS_ADVANCED(OPP_ENVIR_LIBRARY OPP_SIMSTD_LIBRARY OPP_NEDC OPP_MSGC OPP_CMDENV_GUI_LIBRARY OPP_TCL_GUI_LIBRARY OPP_SIMMPI_LIBRARY)
 
# Enable the Wrap NEDC command
IF (OPP_NEDC AND OPP_INCLUDE_PATH)
  IF(OPP_LIBRARY_PATH)
    SET ( OPP_WRAP_NEDC 1 CACHE INTERNAL "Can we honour the OPP_WRAP_NEDC command" )
    IF(OPP_MSGC)
      SET ( OPP_WRAP_MSGC 1 CACHE INTERNAL "Can we honour the OPP_WRAP_MSGC command" )
    ENDIF(OPP_MSGC)
  ENDIF(OPP_LIBRARY_PATH)
ENDIF (OPP_NEDC AND OPP_INCLUDE_PATH)
 
#  Set HAS_OPP
#  This is the final flag that will be checked by
#  other code that requires OMNeT++ for compile/run.
#
IF(OPP_NEDC)
  IF(OPP_INCLUDE_PATH)
    IF(OPP_LIBRARIES)
      SET (HAS_OPP 1 CACHE INTERNAL "OMNET++ library, headers and nedc are available")
      IF(NOT NEDC_CFLAGS)
        SET(NEDC_CFLAGS -Wno-unused )
      ENDIF(NOT NEDC_CFLAGS)
      INCLUDE(${CMAKEFILES_PATH}/LoadCommands.cmake)
    ENDIF(OPP_LIBRARIES)
  ENDIF(OPP_INCLUDE_PATH)
ENDIF(OPP_NEDC)
