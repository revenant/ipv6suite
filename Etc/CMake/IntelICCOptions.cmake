#source /opt/intel_xxx/bin/iccvars.sh
#CC=icc CXX=icpc ccmake [source dir]
IF(CMAKE_CXX_COMPILER MATCHES "icpc")
  #Turn off thread safety locks in Intel iostream libs
  ADD_DEFINITIONS(-D_IOSTREAM_OP_LOCKS=0)
  EXEC_PROGRAM(icc ARGS -v OUTPUT_VARIABLE ICCVERSION)

#Must be cmake bug to not allow ADD_DEFINITION to work inside if loop
#  IF(ICCVERSION MATCHES "8\\.0")
    #-Xlinker -rpath -Xlinker /opt/intel_cc_80/lib
    #-Xlinker -rpath -Xlinker /opt/intel/compiler70/ia32/lib
    #Precompiled headers #pragma hdrstop after common header includes
#    ADD_DEFINITIONS(-pch -pch_dir ${CMAKE_BINARY_DIR}/pch)
    #-no-gcc to remove __GNUC__ related macros
    #FP stack check (no longer in icc 10)
    #ADD_DEFINITIONS(-Wcheck -no-gcc -fpstkchk) #-Wp64
    ADD_DEFINITIONS(-Wcheck -no-gcc -diag-disable 424,981,304,654,304,444) 
#869 param was  never referenced, 383 value copied to temp, ref to temp used
ADD_DEFINITIONS(-diag-disable 869,383)
    #static verification
    OPTION(ICC_STATIC_VERIFICATION  "Enable static verification i.e. no lib/exe output " OFF)
    IF(ICC_STATIC_VERIFICATION)
      ADD_DEFINITIONS(-diag-enable sv3 -diag-enable sv-include -diag-file-append staticVerif)
    ENDIF(ICC_STATIC_VERIFICATION)
    #-cxxlib-gcc to use gcc runtime library
    OPTION(ICC_BRIEF_WARNINGS "Enable one liner warnings for icc" OFF)
    IF(ICC_BRIEF_WARNINGS)
      ADD_DEFINITIONS(-Wbrief)
    ELSE(ICC_BRIEF_WARNINGS)
      #omnetpp won't build with this flag nor would IPv6Suite unless changes were made
      ADD_DEFINITIONS(-strict_ansi)
    ENDIF(ICC_BRIEF_WARNINGS)
    IF(NOT BUILD_DEBUG)
      ADD_DEFINITIONS(-axN)
    ENDIF(NOT BUILD_DEBUG)
    OPTION(ICC_BUILD_PGI "Profile Information options for code coverage and test tool" OFF)
    IF(ICC_BUILD_PGI)
      SET(PROF_DIR  $ENV{PROF_DIR})# DOCSTRING "PROF_DIR env variable pointing to directory to store optimisation files")
      IF(PROF_DIR)
      ELSE(PROF_DIR)	
        MESSAGE(SEND_ERROR "Please set environment variable PROF_DIR e.g. export PROF_DIR=${CMAKE_BINARY_DIR}/prof")
      ENDIF(PROF_DIR)
      #ENDIF(NOT "${PROF_DIR}")
      #Run instrumented executable and outputs .dyn files in prof_dir
      #profmerge -profdir "${PROF_DIR}" -prof_dpi case1.dpi)
      #rm $PROF_DIR/*.dyn
      #Run again with diff input 
      #profmerge with diff dpi output name
      #repeat n times
      #tselect -dpi_list tests_list -spi pgopti.spi #tests.lst list name of .dpi on each line
      ADD_CUSTOM_TARGET(codecov 
	cd ${PROF_DIR}\;
	profmerge -prof_dir ${PROF_DIR} -prof_dpi someruns.dpi \;
	codecov -spi pgopti.spi -dpi someruns.dpi -counts -demang -prj ${PROJECT_NAME})
      OPTION(ICC_PGI_GEN "Generate profile info. When off will feed the .spi file back for PGO" ON)
      IF(ICC_PGI_GEN)
        ADD_DEFINITIONS(-prof_genx -prof_dir "${PROF_DIR}")
      ELSE(ICC_PGI_GEN)
        ADD_DEFINITIONS(-prof_use)
      ENDIF(ICC_PGI_GEN)
    ENDIF(ICC_BUILD_PGI)
#  ENDIF(ICCVERSION MATCHES "8\\.0")

IF(ICCVERSION MATCHES "8\\.1")
  #link with intel c++ libs (make sure omnetpp and other dependencies use the
  #same settings (.rpmrc) or else will get errors and strange segfault at start)

  #Intel can now use gcc stdc++ lib properly so no need for this
  #SET(CMAKE_EXE_LINKER_FLAGS -cxxlib-icc) 
  #SET(CMAKE_SHARED_LINKER_FLAGS -cxxlib-icc)
  #SET(CMAKE_MODULE_LINKER_FLAGS -cxxlib-icc)
  #ADD_DEFINITIONS(-cxxlib-icc)
ENDIF(ICCVERSION MATCHES "8\\.1")

  IF(ICC_LIBCWD_DEBUG_HACK)
    ADD_DEFINITIONS(${DOUT_HACK})
  ENDIF(ICC_LIBCWD_DEBUG_HACK)
ENDIF(CMAKE_CXX_COMPILER MATCHES "icpc")
