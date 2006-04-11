# -*- CMAKE -*-
# This CMakeLists.txt file handles the creation of files needed by
# other client projects that use IPv6Suite.  Nothing is built by this
# CMakeLists.txt file.  This CMakeLists.txt file must be processed by
# CMake after all the other CMakeLists.txt files in the IPv6Suite tree,
# which is why the we are the last cmake file in Main.cmake.

# Needed to get non-cached variable settings used in IPv6SuiteConfig.cmake.in
# Already done by prior cmake files anyway
#INCLUDE( ${MODULE_PATH}/FindNetlib.cmake )
#INCLUDE( ${MODULE_PATH}/FindQv.cmake )
#INCLUDE( ${MODULE_PATH}/FindZLIB.cmake )
#INCLUDE( ${MODULE_PATH}/FindPNG.cmake )
#INCLUDE( ${MODULE_PATH}/FindJPEG.cmake )
#INCLUDE( ${MODULE_PATH}/FindTIFF.cmake )
#INCLUDE( ${MODULE_PATH}/FindMPEG2.cmake )

# Save the compiler settings so another project can import them.
INCLUDE(${CMAKE_ROOT}/Modules/CMakeExportBuildSettings.cmake)
SET(IPv6Suite_BUILD_SETTINGS_FILE ${IPv6Suite_BINARY_DIR}/IPv6SuiteBuildSettings.cmake)
CMAKE_EXPORT_BUILD_SETTINGS(${IPv6Suite_BUILD_SETTINGS_FILE})

# Save library dependencies.
SET(IPv6Suite_LIBRARY_DEPENDS_FILE ${IPv6Suite_BINARY_DIR}/IPv6SuiteLibraryDepends.cmake)
EXPORT_LIBRARY_DEPENDENCIES(${IPv6Suite_LIBRARY_DEPENDS_FILE})

# Create the IPv6SuiteConfig.cmake file for the build tree.
CONFIGURE_FILE(${CMAKEFILES_PATH}/IPv6SuiteConfig.cmake.in
               ${IPv6Suite_BINARY_DIR}/IPv6SuiteConfig.cmake @ONLY IMMEDIATE)
