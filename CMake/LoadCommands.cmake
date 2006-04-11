# -*- cmake -*-
###### "Dynamic Loading" #######################################################

IF(NOT BINMISCDIR)
  SET(BINMISCDIR ${PROJECT_BINARY_DIR})
ENDIF(NOT BINMISCDIR)

SET(GENERAL_INI ${BINMISCDIR}/general.ini)
#satisfy include in default.ini
WRITE_FILE(${GENERAL_INI} "") 

MACRO(COMPILE_SOURCE)
  FILE(GLOB COMPILE_SOURCE_FILES *.cc)
  IF(COMPILER_WARNINGS)
    SET_SOURCE_FILES_PROPERTIES(${COMPILE_SOURCE_FILES} PROPERTIES  COMPILE_FLAGS ${COMPILER_WARNINGS})
  ENDIF(COMPILER_WARNINGS)
ENDMACRO(COMPILE_SOURCE)

#Recursively descends into all subdirs and adds all msg sources to GENERATED_MSGC_FILES.
#Generated files are placed in current directory
#To actually generate add GENERATED_MSGC_FILES to an ADD_EXECUTABLE or ADD_LIBRARY command
#or to another sources list macro
MACRO(OPP_WRAP_MSGC_ALL)
  FILE(GLOB_RECURSE SOURCES *.msg)
  FOREACH(file ${SOURCES})
    STRING(REGEX REPLACE "[.]msg$" "_m.cc" SOURCE ${file})
    STRING(REGEX REPLACE "^.*/" "" SOURCE ${SOURCE})
    STRING(REGEX REPLACE "[.]msg$" "_m.h" HEADER ${file})
    STRING(REGEX REPLACE "^.*/" "" HEADER ${HEADER})
    ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE} COMMAND ${OPP_MSGC}
      ARGS  -h ${file} DEPENDS ${OPP_MSGC}
      MAIN_DEPENDENCY ${file})
    ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${HEADER} COMMAND echo
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE})
    SET(GENERATED_MSGC_FILES ${GENERATED_MSGC_FILES} ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE})
    SET(MSGC_GEN "${MSGC_GEN};${CMAKE_CURRENT_BINARY_DIR}/${SOURCE};${CMAKE_CURRENT_BINARY_DIR}/${HEADER}")
  ENDFOREACH(file)
  SET_SOURCE_FILES_PROPERTIES(${GENERATED_MSGC_FILES} PROPERTIES GENERATED ON)
  SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${MSGC_GEN}")  
ENDMACRO(OPP_WRAP_MSGC_ALL)

#Manually specified list of msg files 
MACRO(OPP_WRAP_MSGC dummyTarget dummyIncludes )
  FOREACH(file ${ARGN})
    STRING(REGEX REPLACE "[.]msg$" "_m.cc" SOURCE ${file})
    STRING(REGEX REPLACE "^.*/" "" SOURCE ${SOURCE})
    STRING(REGEX REPLACE "[.]msg$" "_m.h" HEADER ${file})
    STRING(REGEX REPLACE "^.*/" "" HEADER ${HEADER})
    ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE} COMMAND ${OPP_MSGC}
      ARGS  -h ${CMAKE_CURRENT_SOURCE_DIR}/${file} DEPENDS ${OPP_MSGC}
      MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${file})
    ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${HEADER} COMMAND echo
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE})
    SET(GENERATED_MSGC_FILES ${GENERATED_MSGC_FILES} ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE})
    SET(MSGC_GEN "${MSGC_GEN};${CMAKE_CURRENT_BINARY_DIR}/${SOURCE};${CMAKE_CURRENT_BINARY_DIR}/${HEADER}")
  ENDFOREACH(file)    
  SET_SOURCE_FILES_PROPERTIES(${GENERATED_MSGC_FILES} PROPERTIES GENERATED ON)
  SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${MSGC_GEN}")
ENDMACRO(OPP_WRAP_MSGC)


#Does not work DirName should be name of current directory as opp_test -r -v expects that
MACRO(OPP_WRAP_TEST DirName)
  ADD_CUSTOM_COMMAND(TARGET ${DirName} PRE_BUILD COMMAND ${OPP_TEST} ARGS -g *.test)
  FILE(GLOB TESTS *.test)
  #FOREACH(test ${ARGN})
  FOREACH(test ${TESTS})
    STRING(REGEX REPLACE "[.]test$" ".cc" SOURCE ${test})  
    STRING(REGEX REPLACE "^.*/" "" SOURCE ${SOURCE})
    STRING(REGEX REPLACE "[.]test$" ".ned" TESTNED ${test})
    STRING(REGEX REPLACE "^.*/" "" TESTNED ${TESTNED})
    STRING(REGEX REPLACE "[.]test$" ".ini" TESTINI ${test})
    STRING(REGEX REPLACE "^.*/" "" TESTINI ${TESTINI})
    SET(WORKDIR ${CMAKE_CURRENT_BINARY_DIR})
#    FILE(MAKE_DIRECTORY ${WORKDIR})
    ADD_CUSTOM_COMMAND(OUTPUT ${WORKDIR}/${SOURCE} COMMAND ${OPP_TEST}
      ARGS -w ${CMAKE_CURRENT_BINARY_DIR} -g ${test} DEPENDS ${OPP_TEST}
      MAIN_DEPENDENCY ${test})
          
    SET(MSGC_GEN "${MSGC_GEN};${TESTNED};${TESTINI};${SOURCE}")
    SET(TESTS_SOURCES ${TESTS_SOURCES} ${SOURCE})
    #Creates .cc depending on whether a %global i.e. source file is generated or not
    #Which means not all tests will have a .cc file generated
  ENDFOREACH(test)    
  
  #IF we can run some script to generate the cc and then use this to populate file I guess it may work but how to run script before
  #cmake generates the make file?. Treat test directory as separate project?
  #FILE(GLOB SOURCES ${CMAKE_CURRENT_BINARY_DIR}/*.cc)
  
  SET_SOURCE_FILES_PROPERTIES(${TESTS_SOURCES} PROPERTIES GENERATED ON)
  
  ADD_EXECUTABLE(${DirName} ${TESTS_SOURCES} )
  
  SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${MSGC_GEN}" )
  
ENDMACRO(OPP_WRAP_TEST )

MACRO(OPP_WRAP_NEDC dum INCLUDES)
  FOREACH(file ${ARGN})
    STRING(REGEX REPLACE "[.]ned$" "_n.cc" SOURCE ${file})
    STRING(REGEX REPLACE "^.*/" "" SOURCE ${SOURCE})   
    IF (${INCLUDES} MATCHES "[^.]")
      IF(${INCLUDES})
        STRING(REGEX REPLACE ";" ";-I" IFLAGS "${${INCLUDES}}")
        SET(IFLAGS "-I${IFLAGS}")
#       MESSAGE("${INCLUDES} Iflags is ${IFLAGS}")  
      ENDIF(${INCLUDES})
    ENDIF (${INCLUDES} MATCHES "[^.]")

    ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE} COMMAND ${OPP_NEDC}
      ARGS  -h ${IFLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/${file} DEPENDS ${OPP_NEDC}
      MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${file})
    #MESSAGE(GENERATED NEDC_ FOREACH ${SOURCE})
    SET(GENERATED_NEDC_FILES ${GENERATED_NEDC_FILES} ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE})
    
    SET(MSGC_GEN "${MSGC_GEN};${CMAKE_CURRENT_BINARY_DIR}/${SOURCE}")
  ENDFOREACH(file)

  SET_SOURCE_FILES_PROPERTIES(${GENERATED_NEDC_FILES} GENERATED COMPILE_FLAGS -Wno-unused)
  SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${MSGC_GEN}")

  IF (${file} MATCHES "[.]ned")
    #WRITE_FILE(${PROJECT_BINARY_DIR}/nedlist "${file}" APPEND)
  ENDIF (${file} MATCHES "[.]ned")
ENDMACRO(OPP_WRAP_NEDC)

IF(OPP_VERSION3_TEST AND DYNAMIC_NED)
  WRITE_FILE(${GENERAL_INI} "preload-ned-files = @${BINMISCDIR}/nedfiles.lst *.ned")
  IF(NOT ONE_BIG_EXE)
    WRITE_FILE(${DUMMY_SOURCE} "")
  ENDIF(NOT ONE_BIG_EXE)

  #Use macro to replace loaded command and get it to do nothing for dynamic ned loading
  
  #Use a fake macro and write out ned files
  #This only works partially i.e. when files is more than one file we only get
  #first, At least this allows OPP_WRAP_NEDC to be ignored. CustomTargets
  #has a preloadNed target which lists all ned files. (make sure no duplicates
  #exist)

  MACRO(OPP_WRAP_NEDC dum files)
    FOREACH(file ${files})
      IF (${file} MATCHES "[.]ned")
        #WRITE_FILE(${PROJECT_BINARY_DIR}/nedlist "${file}" APPEND)
      ENDIF (${file} MATCHES "[.]ned")
    ENDFOREACH(file)
  ENDMACRO(OPP_WRAP_NEDC)
ENDIF(OPP_VERSION3_TEST AND DYNAMIC_NED)

MACRO(COPYCONFIG)
    FILE(GLOB_RECURSE INIFILES *.ini)
    FOREACH(INI ${INIFILES})
      #STRING(REGEX REPLACE "^.*/" "" INI ${INI})
      #STRING(REGEX REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" INI ${INI})
      #MESSAGE("ini is ${INI}")
      #CONFIGURE_FILE(${INI} ${CMAKE_CURRENT_BINARY_DIR}/${INI} COPYONLY)
    ENDFOREACH(INI)
  ENDMACRO(COPYCONFIG)
  
IF(PLUGINS)
  IF(INLINENET)
    # try to compile the command
    TRY_COMPILE(COMPILE_OK 
      ${PROJECT_BINARY_DIR}/Etc/CMake 
      ${PROJECT_SOURCE_DIR}/Etc/CMake 
      OPP_LOADED_COMMANDS) #CMAKE_FLAGS -DMUDSLIDE_TYPE:STRING=MUCHO)
    
    IF (NOT COMPILE_OK)
      MESSAGE("failed to compile OPP_LOADED_COMMANDS")
    ENDIF (NOT COMPILE_OK)
  ENDIF(INLINENET)

  # if the compile was OK, try loading the command
  LOAD_COMMAND(OPP_WRAP_MSGC ${PROJECT_BINARY_DIR}/Etc/CMake /usr/lib)
  LOAD_COMMAND(OPP_WRAP_NEDC ${PROJECT_BINARY_DIR}/Etc/CMake /usr/lib)

  # if the command loaded, execute the command
  IF (COMMAND OPP_WRAP_NEDC)
    #Test a sample ned file (does not expose problems of target dependencies missing)
    #    MESSAGE("OPP_WRAP_NEDC command loaded successfully")
  ELSE(COMMAND OPP_WRAP_NEDC)
    MESSAGE(FATAL_ERROR "Failed to load OPP_WRAP_NEDC command")
  ENDIF (COMMAND OPP_WRAP_NEDC)
  IF (COMMAND OPP_WRAP_MSGC)
    #    MESSAGE("OPP_WRAP_MSGC command loaded successfully")
  ELSE (COMMAND OPP_WRAP_MSGC)
    MESSAGE(FATAL_ERROR "Failed to load OPP_WRAP_MSGC command")
  ENDIF (COMMAND OPP_WRAP_MSGC) 
ENDIF(PLUGINS)

