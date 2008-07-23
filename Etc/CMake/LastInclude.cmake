# -*- CMAKE -*-
#would be nice to go inside LinkLibraries.cmake but need INET_SRCS which is
#generated at toplevel dir. Can be there if generated one is included by Main.cmake

IF(ONE_BIG_EXE)  #evolved to ONE BIG LIB now
  SET(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/lib)
  
  IF(NOT WIN32)
    SET(OUTPUTDIR "./")
  ELSE(NOT WIN32)
    SET(OUTPUTDIR "")
  ENDIF(NOT WIN32)
  
  ADD_LIBRARY(inet ${INET_SRCS})
  ADD_DEPENDENCIES(inet prebuild hold)
  
  SET(DUMMY_FILE ${CMAKE_CURRENT_BINARY_DIR}/empty.cc)
  FILE(WRITE ${DUMMY_FILE} "//empty file\n")
   
  SET(INET ${OUTPUTDIR}INET)
  ADD_EXECUTABLE(${INET} ${DUMMY_FILE})
  TARGET_LINK_LIBRARIES(${INET} inet ${OPP_CMDLIBRARIES} )
  
  IF(NOT LIBCWD_DEBUG)
    IF(OPP_USE_TK)
      SET(tkINET ${OUTPUTDIR}tkINET)
      ADD_EXECUTABLE(${tkINET} ${DUMMY_FILE})
      TARGET_LINK_LIBRARIES(${tkINET} inet ${OPP_TKGUILIBRARIES} )
    ENDIF(OPP_USE_TK)
  ENDIF(NOT LIBCWD_DEBUG)
ENDIF(ONE_BIG_EXE)

IF(NOT WIN32)
  ENABLE_TESTING()
  OPP_WRAP_TEST(Tests)
  INCLUDE(FindRuby)
  IF (RUBY_EXECUTABLE)
    SUBDIRS(
    Etc/scripts/
    )
  ENDIF (RUBY_EXECUTABLE)
  SUBDIRS(
  Research/Networks/
  )
ENDIF(NOT WIN32)
