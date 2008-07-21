# ++ CMAKE ++
## Most of these targets can be rewritten under cmake >= 1.8 to have custom dependencies
## using ADD_DEPENDENCIES or adding a DEPENDS clause to ADD_CUSTOM_TARGET

SET(FIND_COMMAND_EXEC_END "\\\;")
#STRING(REGEX REPLACE "/" "\\" BINDIR ${JavaR2_BINARY_DIR})
#STRING(REGEX REPLACE "\\" "/" FIND_COMMAND_FIXED ${FIND_COMMAND_END})

IF(OPP_VERSION3_TEST)
# done as part of CMakeListGen.rb already
#  ADD_CUSTOM_TARGET(preloadNed ALL find ${PROJECT_SOURCE_DIR} -name \\*.ned > ${BINMISCDIR}/nedfiles.lst)
ENDIF(OPP_VERSION3_TEST)
##---------------------------Clean targets--------------------------------------

ADD_CUSTOM_TARGET(cleanNed          cd ${PROJECT_BINARY_DIR}\; find . \\\( -name '*_n.${SOURCE_EXTENSION}' -o -name
'*_m.${SOURCE_EXTENSION}' -o -name 
'*_m.h' \\\) -exec rm {} ${FIND_COMMAND_EXEC_END} )

ADD_CUSTOM_TARGET(cleanAll  make clean\; make cleanNed)

##---------------------------Emacs-----------------------------------------------

#Without grepping it will actually execute more than 1 command as there is 20k
#command line limit
ADD_CUSTOM_TARGET(etags             cd ${PROJECT_SOURCE_DIR}\;find . -name '*.h' -o
-name '*.cc'   |grep -v '.*_n.*' |xargs ctags -e -R --extra=+q)

#Emacs cannot tolerate Loki sources in EBROWSE
ADD_CUSTOM_TARGET(ebrowse           cd ${PROJECT_SOURCE_DIR}\; find . -name
'*.cc' -o -name '*.h' |grep -v '.*_n.*' | grep -v '.*Loki.*'|xargs -l450 -p ebrowse)

##---------------------------Documentation--------------------------------------

IF(BUILD_DOCUMENTATION)
  IF(NOT WIN32)
    INCLUDE(${CMAKEFILES_PATH}/DocTargets.cmake)
  ENDIF(NOT WIN32)
ENDIF(BUILD_DOCUMENTATION)

##---------------------------Parallel builds------------------------------------

# Requires distcc and environment var DISTCC_HOSTS see man distcc
ADD_CUSTOM_TARGET(pb     cd ${PROJECT_BINARY_DIR}\; make -j `cat /proc/cpuinfo |grep processor|wc|cut -c 7-8` \; )
ADD_CUSTOM_TARGET(pbTest cd ${PROJECT_BINARY_DIR}\; time bash -c "rm -fr ~/.ccache\;make cleanAll\; make pb" )
ADD_CUSTOM_TARGET(serialBuildTest   cd ${PROJECT_BINARY_DIR}\; time bash -c "rm -fr ~/.ccache\;unset DISTCC_HOSTS \;make cleanAll\; make")

##--------------------------Install Targets------------------------------------

CONFIGURE_FILE(${SCRIPTDIR}/CopyInstallFiles.sh.in
               ${BINSCRIPTDIR}/CopyInstallFiles.sh @ONLY IMMEDIATE)
ADD_CUSTOM_TARGET(customInstall cd ${PROJECT_BINARY_DIR}\;sh ${BINSCRIPTDIR}/CopyInstallFiles.sh) 

#To do it cmake way would require putting these two lines into every directory
#with these files in it. Since only some directories have CMakeLists.txt I'll
#pass on that
#INSTALL_FILES(${PROJECT_BINARY_DIR}/include .*\\.h$)
#INSTALL_FILES(${PROJECT_BINARY_DIR}/lib .*\\.so$)


##--------------------------Test Targets------------------------------------
ADD_CUSTOM_TARGET(validateXML cd ${PROJECT_BINARY_DIR}\;
  find . -path ./Testing -prune -o  -path ./Testing -prune -o -name \\*.xml -a ! -name \\*schema\\* |grep -v Testing |grep -v Documentation|xargs xmllint --noout --valid \;
  )

ADD_CUSTOM_TARGET(oppTest cd ${PROJECT_BINARY_DIR}/tests \;
  ${OPP_TEST} -w ${CMAKE_CURRENT_BINARY_DIR} -g ${PROJECT_SOURCE_DIR}/tests/*.test \; cd - \; make)
#Use this for schema but complains about types not matching schema when
#valXML(xerces4j) says its correct. Bad Schema support in libxml2?
#xmllint --noout --schema Etc/netconf.xsd
