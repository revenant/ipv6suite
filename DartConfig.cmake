#
# This is the DartConfig.cmake file for the IPv6Suite.
# The variables here would be set by the software project administrator
# and should not be modified by the user.
#

# Dashboard is opened for submissions for a 24 hour period starting at
# the specified NIGHLY_START_TIME. Time is specified in 24 hour format.
SET (NIGHTLY_START_TIME "1:00:00 EST")

# Dart server to submit results (used by client)
#SET (DROP_METHOD "ftp")
SET (DROP_METHOD "scp")
#SET (DROP_SITE "hydra.ctie.monash.edu.au")
SET (DROP_SITE "localhost")
#SET (DROP_LOCATION "/pub")
SET (DROP_LOCATION "public_html/incoming")
#SET (DROP_SITE_USER "anonymous")
#SET (DROP_SITE_PASSWORD "ipv6suite-tester@localhost")
SET (TRIGGER_SITE 
       "http://${DROP_SITE}/cgi-bin/Submit-IPv6Suite-TestingResults.pl")

# Dart server configuration 
SET (CVS_WEB_URL "http://${DROP_SITE}/cgi-bin/viewcvs.cgi/IPv6Suite/")
#SET (CVS_WEB_CVSROOT "IPv6Suite")
SET (CVS_WEB_CVSROOT "") #viewcvs used so blank

OPTION(BUILD_DOXYGEN "Build source documentation using doxygen" "Off")
#SET (DOXYGEN_URL "http://${DROP_SITE}/IPv6Suite/Doxygen/html/" )
SET (DOXYGEN_URL "http://www-personal.monash.edu.au/~swoon/IPv6Suite-doc/" )
SET (USE_DOXYGEN "On")
SET (DOXYGEN_CONFIG "${PROJECT_BINARY_DIR}/Doxyfile" )

SET (USE_GNATS "Off")
SET (GNATS_WEB_URL "http://${DROP_SITE}/cgi-bin/gnatsweb.pl/IPv6Suite/")

# Problem build email delivery variables
SET (DELIVER_BROKEN_BUILD_EMAIL "") #Don't send any emails
#SET (DELIVER_BROKEN_BUILD_EMAIL "Continuous")
SET (EMAIL_FROM "dart@natura")
SET (DARTBOARD_BASE_URL "http://192.168.2.3/Testing")
SET (EMAIL_PROJECT_NAME "IPv6Suite")
SET (BUILD_MONITORS "{.* jmll@localhost}")
SET (CVS_IDENT_TO_EMAIL "{cvsuser1 aa@aaaa.com} {cvsuser2 bb@bbbb.com}")
SET (DELIVER_BROKEN_BUILD_EMAIL_WITH_CONFIGURE_FAILURES "1")
SET (DELIVER_BROKEN_BUILD_EMAIL_WITH_BUILD_ERRORS "1")
SET (DELIVER_BROKEN_BUILD_EMAIL_WITH_BUILD_WARNINGS "0")
SET (DELIVER_BROKEN_BUILD_EMAIL_WITH_TEST_NOT_RUNS "1")
SET (DELIVER_BROKEN_BUILD_EMAIL_WITH_TEST_FAILURES "0")


# Continuous email delivery variables
SET (CONTINUOUS_FROM "dart@natura")
SET (SMTP_MAILHOST "smtp.monash.edu.au")
#SET (CONTINUOUS_MONITOR_LIST "johnny.lai@eng.monash.edu.au")
SET (CONTINUOUS_MONITOR_LIST "jmll@localhost")
SET (CONTINUOUS_BASE_URL "http://hydra.ctie.monash.edu.au/Testing")

# Copy over the testing logo
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/tux.gif
     ${PROJECT_BINARY_DIR}/Testing/HTML/TestingResults/Icons/Logo.gif COPYONLY)

# If DROP_METHOD is set to "scp", then add this FIND_PROGRAM to your DartConfig
FIND_PROGRAM(ScpCommand scp DOC 
     "Path to scp command, sometimes used for submitting Dart results.")

#By default it includes this but reports heaps of omnetpp as well as my own
#leaks so not useful since file is huge 
#--show-reachable=yes --workaround-gcc296-bugs=yes
#SET(VALGRIND_COMMAND_OPTIONS -q --skin=memcheck --leak-check=yes --num-callers=100)
#memcheck too slow just do addrcheck
SET(VALGRIND_COMMAND_OPTIONS -q -v --tool=addrcheck --leak-check=yes --show-reachable=yes --workaround-gcc296-bugs=yes --num-callers=100)
