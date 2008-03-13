# -*- CMAKE -*-
######### "Create Release Macros" ##############################################

OPTION(CREATE_IP6_RELEASE "List source files in build (was Enable building IPv6 release)" OFF)
MARK_AS_ADVANCED(FORCE CREATE_IP6_RELEASE)
IF (CREATE_IP6_RELEASE)
  #clear last invocation
  SET(RELEASE_FILENAME "${CMAKE_CURRENT_BINARY_DIR}/releaseFiles")
  WRITE_FILE(${RELEASE_FILENAME} "")
ENDIF (CREATE_IP6_RELEASE)
MACRO(RELEASE_FILES_APPEND files)
  SET(RELEASE_FILENAME "${CMAKE_CURRENT_BINARY_DIR}/releaseFiles")
  IF (CREATE_IP6_RELEASE)
    FOREACH(file ${files})
      WRITE_FILE(${RELEASE_FILENAME} "${file}" APPEND)
    ENDFOREACH(file)
  ENDIF (CREATE_IP6_RELEASE)
ENDMACRO(RELEASE_FILES_APPEND)

MACRO(RELEASE_FILES files)
  IF (CREATE_IP6_RELEASE)
#  WRITE_FILE(${RELEASE_FILENAME} "CMakeLists.txt")
#  RELEASE_FILES_APPEND("${files};omnetpp.ini;ChangeLog;README;*.xml;*.ned")
  EXEC_PROGRAM(pwd OUTPUT_VARIABLE current_path)
  #Only gives back the top level pwd not every directory :()
#  GET_FILENAME_COMPONENT(current_dir "${current_path}" NAME)
#  GET_FILENAME_COMPONENT(current_dir "${CMAKE_CURRENT_BINARY_DIR}" NAME)
#  WRITE_FILE(${RELEASE_FILENAME} "${current_dir}.ned"  APPEND)
  ENDIF (CREATE_IP6_RELEASE)
ENDMACRO(RELEASE_FILES)

#Only cc sources can use this form because SET_SOURCE_FILES_PROPERTIES only
#works on these
MACRO(RELEASE_SOURCES dir_srcs)
  SET_SOURCE_FILES_PROPERTIES(${dir_srcs} COMPILE_FLAGS "${COMPILER_WARNINGS}")
  IF (CREATE_IP6_RELEASE)

#  WRITE_FILE(${RELEASE_FILENAME} "CMakeLists.txt")
#  RELEASE_FILES_APPEND("*.ned;*.xml;ChangeLog;*.h")

  #Still required because <subdir>/*.h cannot be obtained directly through find
  #and ls doesn't work :()
  FOREACH(file ${dir_srcs})
    #Write the normal file (Handle case of without explicit extension)
    STRING(REGEX MATCH ".[.]." RESULT "${file}")
    IF(NOT RESULT)
      RELEASE_FILES_APPEND("${file}.${SOURCE_EXTENSION}")
    ELSE(NOT RESULT)
      RELEASE_FILES_APPEND("${file}")
    ENDIF(NOT RESULT)

    #Test for .cc and create a .h placeholder (Does not work)
#    STRING(REGEX MATCH "[.]${SOURCE_EXTENSION}" RESULT "${file}")
#    IF(RESULT)
#      STRING(REGEX REPLACE ".[.]${SOURCE_EXTENSION}.$" ".h" RESULT "${file}")
#      WRITE_FILE(${RELEASE_FILENAME} ${file}  APPEND)
#    ENDIF(RESULT)
  ENDFOREACH(file)
ENDIF(CREATE_IP6_RELEASE)

#  STRING(ASCII 35 POUND)
#  WRITE_FILE("${CMAKE_CURRENT_BINARY_DIR}/test_${classname}.cxx" "${POUND}define ${classname}_MAIN_NEEDED\n${POUND}include <${CLASSNAME}.h>")
#  ADD_EXECUTABLE(test_${classname} "${CMAKE_CURRENT_BINARY_DIR}/test_${classname}.cxx")
#  TARGET_LINK_LIBRARIES(test_${classname} ${libname})
ENDMACRO(RELEASE_SOURCES)

#This does not work because cmake probably cannot do recursive variable lookup
MACRO(RELEASE_SOURCES_CURRENT)
  EXEC_PROGRAM(pwd OUTPUT_VARIABLE current_path)
  GET_FILENAME_COMPONENT(current_dir "${current_path}" NAME)  
  SET(realname "${current_dir}_SRCS")
  MESSAGE(realname is ${realname})
  #Does not work
  RELEASE_SOURCES("${${current_dir}_SRCS}")
  RELEASE_SOURCES("${${realname}}")
ENDMACRO(RELEASE_SOURCES_CURRENT)


##################### "OutOfSourceCopyConfig target aux macros" ################

#When USER_RELEASE is false OutOfSourceCopyConfig will updates existing files from PROJECT_SOURCE_DIR
MACRO(COPY_MISSING_CONFIG_FILES files)
  IF (${CMAKE_CURRENT_SOURCE_DIR} MATCHES ${CMAKE_CURRENT_BINARY_DIR})
  ELSE(${CMAKE_CURRENT_SOURCE_DIR} MATCHES ${CMAKE_CURRENT_BINARY_DIR})
    IF(${ConfigCopy} MATCHES "^do$")
      FOREACH(file ${files})
	EXEC_PROGRAM(ls ARGS ${file} OUTPUT_VARIABLE exists )
	IF(USER_RELEASE)
	  IF(${exists} MATCHES "^ls: ${file}: No such file or directory")
	    CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${file}  ${CMAKE_CURRENT_BINARY_DIR}/${file} COPY_FILE)
	  ENDIF(${exists} MATCHES "^ls: ${file}: No such file or directory")
	ELSE(USER_RELEASE)
	  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${file}  ${CMAKE_CURRENT_BINARY_DIR}/${file} COPY_FILE)
	ENDIF(USER_RELEASE)
      ENDFOREACH(file)
      MARK_AS_ADVANCED(FORCE ConfigCopy)
    ENDIF(${ConfigCopy} MATCHES "^do$")
  ENDIF (${CMAKE_CURRENT_SOURCE_DIR} MATCHES ${CMAKE_CURRENT_BINARY_DIR})
ENDMACRO(COPY_MISSING_CONFIG_FILES)

MACRO(COPY_MISSING_CONFIG_FILES_FOR_SIMULATION)
  GET_FILENAME_COMPONENT(current_dir "${CMAKE_CURRENT_BINARY_DIR}" NAME)
#  MESSAGE("in macro COPY_MISSING_CONFIG_FILES_FOR_SIMULATION copying ${current_dir}.xml;omnetpp.ini ")
  COPY_MISSING_CONFIG_FILES("${current_dir}.xml;omnetpp.ini")
ENDMACRO(COPY_MISSING_CONFIG_FILES_FOR_SIMULATION)


##################### "Build macros" ###########################################

#Sets NEDSOURCES variable for use in OPP_WRAP_NEDC
#OPP_WRAP_NEDC outputs GENERATED_NEDC_FILES for use in ADD_LIBRARY or ADD_EXECUTABLE
MACRO(APPEND_NED_EXT sources)
#MESSAGE("sources is ${sources}")
SET(NEDSOURCES "")
  FOREACH(source ${sources})
#    MESSAGE("source is ${source}")
    SET(NEDSOURCES ${NEDSOURCES} ${source}.ned)
  ENDFOREACH(source)
#MESSAGE("NEDSOURCES is ${NEDSOURCES}")
ENDMACRO(APPEND_NED_EXT)


MACRO(LINK_OPP_LIBRARIES target libraries)
IF(NOT ONE_BIG_EXE)
  IF(NOT DYNAMIC_NED)
    ADD_EXECUTABLE(${target} ${GENERATED_NEDC_FILES})
  ELSE(NOT DYNAMIC_NED)
    ADD_EXECUTABLE(${target} ${DUMMY_SOURCE})
  ENDIF(NOT DYNAMIC_NED)
  TARGET_LINK_LIBRARIES(${target} ${libraries})
ELSE(NOT ONE_BIG_EXE)
  #ln -s ${IPv6Suite} ${target}
  EXEC_PROGRAM(${LN_EXE}  ${CMAKE_CURRENT_BINARY_DIR} ARGS -sf ${BigExe} ${target})
ENDIF(NOT ONE_BIG_EXE)
ENDMACRO(LINK_OPP_LIBRARIES)

MACRO(LINK_CONF_OPP_LIBRARIES target)
  LINK_OPP_LIBRARIES(${target} "${OPP_LIBRARIES}")
ENDMACRO(LINK_CONF_OPP_LIBRARIES)

MACRO(GENNEDSOURCES target ned_includes ned_sources)
#  MESSAGE("Ned sources is ${ned_sources}")
  APPEND_NED_EXT("${ned_sources}") 
#  MESSAGE("NEDSOURCES is ${NEDSOURCES}")
  OPP_WRAP_NEDC(${target} ${ned_includes} ${NEDSOURCES})
  SET_SOURCE_FILES_PROPERTIES(${GENERATED_NEDC_FILES}  GENERATED COMPILE_FLAGS -Wno-unused)
ENDMACRO(GENNEDSOURCES target ned_includes ned_sources)

#Requires files `basename \`pwd\``.xml and omnetpp.ini if using
#OutOfSourceCopyConfig custom target
MACRO(CREATE_SIMULATION target ned_includes ned_sources) #
  GENNEDSOURCES(${target} "${ned_includes}"  "${ned_sources}")

  IF(NOT BUILD_SHARED_LIBS)
    #It appears that these source files properties only exist in the current
    #Dir/cmakelist.txt because in subdir's cmakelist.txt cannot use the source
    #file here as it complains source does not exist unless we also set generated property in there for these files again
    SET_SOURCE_FILES_PROPERTIES(${GENERATED_MSGC_FILES}  GENERATED PROPERTIES GENERATED ON COMPILE_FLAGS -Wall)
  ENDIF(NOT BUILD_SHARED_LIBS)

  #This line is the actual generation of the standard executable.
  LINK_CONF_OPP_LIBRARIES(${target})

  RELEASE_FILES("${NEDSOURCES}")
  COPY_MISSING_CONFIG_FILES_FOR_SIMULATION()
  IF(TK_LIBRARY)
    #Prob. need to embed tcl/tk into omnet but never tried so leave for shared
    IF(BUILD_SHARED_LIBS)
      LINK_OPP_LIBRARIES(tk${target} "${OPP_TKGUILIBRARIES}")
    ENDIF(BUILD_SHARED_LIBS)
  ENDIF(TK_LIBRARY)
ENDMACRO(CREATE_SIMULATION target ned_includes ned_sources)

#Requires a variable ${target}_ned_includes and file ${target}.ned
MACRO(CREATE_SIM target)
  CREATE_SIMULATION(${target} ${target}_ned_includes "${target}")
ENDMACRO(CREATE_SIM target)

FIND_PROGRAM(BASH bash)
##################### "Test macros" ###########################################
OPTION(COLLECT_RESULT "Should results be collected or just test against past collected results (.out.bz2 files)" OFF)
MACRO(COLLECT_CHECK_TEST testname)
  IF (COLLECT_RESULT)
    ADD_TEST(collect${testname} ${BASH} -c "bzip2 test.out && mv test.out.bz2 ${testname}.out.bz2")
  ELSE (COLLECT_RESULT)
    ADD_TEST(check${testname} ${BASH} -c "bzcat ${testname}.out.bz2|diff -u - test.out")
  ENDIF (COLLECT_RESULT)
ENDMACRO(COLLECT_CHECK_TEST)

MACRO(COLLECT_TEST testname)
    ADD_TEST(collect${testname} ${BASH} -c "bzip2 test.out && mv test.out.bz2 ${testname}.out.bz2")
ENDMACRO(COLLECT_TEST)
