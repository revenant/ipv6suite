# -*- CMAKE -*-
# vxl/config/cmake/UseVXL.cmake
# (also copied by CMake to the top-level of the vxl build tree)
#
# This CMake file may be included by projects outside VXL.  It
# configures them to make use of VXL headers and libraries.  The file
# is written to work in one of two ways.
#
# The preferred way to use VXL from an outside project with UseVXL.cmake:
#
#  FIND_PACKAGE(VXL)
#  IF(VXL_FOUND)
#    INCLUDE(${VXL_CMAKE_DIR}/UseVXL.cmake)
#  ELSE(VXL_FOUND)
#    MESSAGE("VXL_DIR should be set to the VXL build directory.")
#  ENDIF(VXL_FOUND)
#
# Read vxl/config/cmake/VXLConfig.cmake for the list of variables
# provided.  The names have changed to reduce namespace pollution.

# If this file has been included directly by a user project instead of
# through VXL_USE_FILE from VXLConfig.cmake, simulate old behavior.

# IPv6SuiteConfig.cmake has now been included.  Use its settings.
IF(IPv6Suite_CONFIG_CMAKE)
  # Load the compiler settings used for IPv6Suite.
  IF(IPv6Suite_BUILD_SETTINGS_FILE)
    INCLUDE(${CMAKE_ROOT}/Modules/CMakeImportBuildSettings.cmake)
    CMAKE_IMPORT_BUILD_SETTINGS(${IPv6Suite_BUILD_SETTINGS_FILE})
  ENDIF(IPv6Suite_BUILD_SETTINGS_FILE)

  LOAD_COMMAND(OPP_WRAP_NEDC ${IPv6Suite_DIR}/Etc/CMake /usr/lib)
  LOAD_COMMAND(OPP_WRAP_MSGC ${IPv6Suite_DIR}/Etc/CMake /usr/lib)
  
  INCLUDE(${IPv6Suite_CMAKE_DIR}/Macros.cmake)
  INCLUDE_DIRECTORIES(${OPP_INCLUDE_PATH})
  INCLUDE(${IPv6Suite_CMAKE_DIR}/Configure.cmake)
  INCLUDE(${IPv6Suite_CMAKE_DIR}/LinkLibraries.cmake)

  # Use the standard IPv6Suite include directories.
  INCLUDE_DIRECTORIES(${IPv6Suite_INCLUDE_DIR})
  
  # Add link directories needed to use IPv6Suite.
  LINK_DIRECTORIES(${IPv6Suite_LIBRARY_DIR})
  
  
  IF(IPv6Suite_PROVIDE_STANDARD_OPTIONS)
    # Provide the standard set of IPv6Suite CMake options to the project.
    INCLUDE(${IPv6Suite_CMAKE_DIR}/IPv6SuiteStandardOptions.cmake)  
  ENDIF(IPv6Suite_PROVIDE_STANDARD_OPTIONS)
ENDIF(IPv6Suite_CONFIG_CMAKE)
