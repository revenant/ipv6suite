#AKAROA_INCLUDE_PATH = where akaroa.H can be found
#AKAROA_LIBRARIES = library to link against

FIND_PATH(AKAROA_INCLUDE_PATH akaroa.H
	  /usr/local/include
	  /usr/include
)

FIND_LIBRARY(AKAROA_LIBRARY
	     NAMES akaroa
	     PATHS /usr/local/lib /usr/lib
)

MARK_AS_ADVANCED(AKAROA_INCLUDE_PATH AKAROA_LIBRARY)	    
