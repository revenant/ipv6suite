FIND_LIBRARY(LIBCWD_LIBRARY cwd
	$ENV{HOME}/lib/lib
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
)

SET(LIBCWD_PATH libcwd)
FIND_PATH(LIBCWD_INCLUDE_DIR ${LIBCWD_PATH}/debug.h
	$ENV{HOME}/lib/include
	$ENV{HOME}/include	
	/usr/local/include
	/usr/include	
)
IF(LIBCWD_INCLUDE_DIR)
ELSE(LIBCWD_INCLUDE_DIR)
  SET(LIBCWD_PATH libcw)
  FIND_PATH(LIBCWD_INCLUDE_DIR ${LIBCWD_PATH}/debug.h
    $ENV{HOME}/lib/include
    $ENV{HOME}/include	
    /usr/local/include
    /usr/include	
    )
ENDIF(LIBCWD_INCLUDE_DIR)




IF(LIBCWD_INCLUDE_DIR)
  IF(LIBCWD_LIBRARY)
    SET(LIBCWD_FOUND "YES")
  ENDIF(LIBCWD_LIBRARY)
ENDIF(LIBCWD_INCLUDE_DIR)
