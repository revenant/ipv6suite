# -*- cmake -*-
SET(DOCUMENTATION_DIR ${PROJECT_SOURCE_DIR}/Documentation)
SET(GENERATED_DOCUMENTATION_DIR ${PROJECT_BINARY_DIR}/Documentation)
MAKE_DIRECTORY(${GENERATED_DOCUMENTATION_DIR})

#  SET(DOXGEN_HTML_OUTPUT ${PROJECT_NAME}-${IPV6SUITE_RELEASE}-doc)
SET(DOXGEN_HTML_OUTPUT ${PROJECT_NAME}-doc)
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/Doxyfile  ${PROJECT_BINARY_DIR}/Doxyconf @ONLY)
INCLUDE (${CMAKE_ROOT}/Modules/FindDoxygen.cmake)

OPTION(DOX_PERKS "Turns on things like CALL_GRAPH and SEARCH_ENGINE in doxygen config file" OFF)
MARK_AS_ADVANCED(DOX_PERKS)
IF(DOX_PERKS)
  SET(DOX_BOOL_VALUE "YES")
ELSE(DOX_PERKS)
  SET(DOX_BOOL_VALUE "NO")
ENDIF(DOX_PERKS)

SET(RPMCOMMAND rpm -q omnetpp|grep -v icc|xargs -i rpm -q {} --qf '%{name}-%{version}')
IF(${CMAKE_BACKWARDS_COMPATIBILITY} MATCHES 1.8)
  ADD_CUSTOM_TARGET(dox	 
    perl -i -pwe \"s/OPPVERSION/$$\(${RPMCOMMAND}\)/g\" ${PROJECT_BINARY_DIR}/Doxyconf \;
    ${DOXYGEN} ${PROJECT_BINARY_DIR}/Doxyconf \;
    cd ${GENERATED_DOCUMENTATION_DIR}/${DOXGEN_HTML_OUTPUT} \; 
    find . -size 0 |xargs rm \;
    cd ..\; tar jcf ${PROJECT_NAME}-doc{.tar.bz2,} \;
    )

ELSE(${CMAKE_BACKWARDS_COMPATIBILITY} MATCHES 1.8)
  ADD_CUSTOM_TARGET(dox
    ${DOXYGEN} ${PROJECT_BINARY_DIR}/Doxyconf \;
    cd ${GENERATED_DOCUMENTATION_DIR}/${DOXGEN_HTML_OUTPUT} \; 
    find . -size 0 |xargs rm \;
    cd ..\; tar jcf ${PROJECT_NAME}-doc{.tar.bz2,} \;
    )
ENDIF(${CMAKE_BACKWARDS_COMPATIBILITY} MATCHES 1.8)

ADD_CUSTOM_TARGET(oppdox cd ${GENERATED_DOCUMENTATION_DIR}\;
  cp -pr /usr/share/doc/$$\(${RPMCOMMAND}\)/doxy/ /tmp/omnetpp-doc\;
  cd /tmp \;
  tar jcf omnetpp-doc{.tar.bz2,} \;
  rm -fr ${GENERATED_DOCUMENTATION_DIR}/omnetpp-doc \;
  mv omnetpp-doc{,.tar.bz2} ${GENERATED_DOCUMENTATION_DIR} \;
  )

ADD_CUSTOM_TARGET(linkdoxnet cd ${GENERATED_DOCUMENTATION_DIR}/${DOXGEN_HTML_OUTPUT}\;
  perl -i -pwe \"s|= 0|= 1|\" installdox \;
  perl installdox -l omnetpp.tag@http://www-personal.monash.edu.au/~swoon/omnetpp-doc/ \;
  )
#Requires X even though it doesn't draw any windows (don't do this over network
#sucks up huge bandwidth and you don't notice unless you run gkrellm) 
ADD_CUSTOM_TARGET(neddoc cd ${PROJECT_SOURCE_DIR}\;
  unset XMODIFIERS \;
  opp_neddoc 
  -o ${GENERATED_DOCUMENTATION_DIR}/html -a -d ../${DOXGEN_HTML_OUTPUT}
  -t ${GENERATED_DOCUMENTATION_DIR}/ipv6suite.tag \;
  cd ${GENERATED_DOCUMENTATION_DIR}/html\;
  web2png -t \;
  web2png -d \;
  )
#  DEPENDS dox)

ADD_CUSTOM_TARGET(chm		    cd ${PROJECT_BINARY_DIR}/Documentation/${DOXGEN_HTML_OUTPUT}\;
  wine '/local/f/Program Files/HTML Help Workshop/hhc.exe' index.hhp\;
  mv index.chm ../${PROJECT_NAME}-${IPV6SUITE_RELEASE}.chm\;
  )
# installdox -lomnetpp.tag@omnetpp.chm::
#  DEPENDS dox)
ADD_DEPENDENCIES(chm dox)

#This stylesheet is deprecated as Andras' script does that and more
#xsltproc -o netconf.html \-\-nonet netconf-annotation.xsl netconf.xsd
ADD_CUSTOM_TARGET(xsddoc	    sh ${MISCDIR}/XSD2HTML/xsd2html.sh 
  -o ${GENERATED_DOCUMENTATION_DIR}/xsddoc ${PROJECT_SOURCE_DIR}/Etc/netconf.xsd\;
  cd ${GENERATED_DOCUMENTATION_DIR}/xsddoc\;
  web2png -t \;
  web2png -d \;
  )

ADD_CUSTOM_TARGET(statcvs	    cd ${PROJECT_SOURCE_DIR}\;
  cvs -f -z 9 log > ${GENERATED_DOCUMENTATION_DIR}/cvs.log \;
  unset LC_CTYPE\;
  java -jar ~/Java/statcvs.jar  -viewcvs "http://localhost/cgi-bin/viewcvs.cgi/${PROJECT_NAME}" -title ${PROJECT_NAME} -output-dir ${GENERATED_DOCUMENTATION_DIR}/${PROJECT_NAME}-statcvs ${GENERATED_DOCUMENTATION_DIR}/cvs.log ${PROJECT_SOURCE_DIR}\;
  rm ${GENERATED_DOCUMENTATION_DIR}/cvs.log \;
  cd ${GENERATED_DOCUMENTATION_DIR}\; tar jcf ${PROJECT_NAME}-statcvs.tar.bz2 ${PROJECT_NAME}-statcvs\;
  )

ADD_CUSTOM_TARGET(slochtml	    cd ${PROJECT_SOURCE_DIR}\;
  sloccount --wide  . > ${GENERATED_DOCUMENTATION_DIR}/topincip.sloc\;
  cd IP\;
  sloccount --wide --addlangall . > ${GENERATED_DOCUMENTATION_DIR}/ip.sloc\;
  cd ${GENERATED_DOCUMENTATION_DIR}\;
  python ${MISCDIR}/sloc2html.py topincip.sloc > totalsloc.html\;
  python ${MISCDIR}/sloc2html.py ip.sloc > ipsloc.html \;
  rm *.sloc \;
  )

ADD_CUSTOM_TARGET(cccc cd ${GENERATED_DOCUMENTATION_DIR}\;
  find ${PROJECT_SOURCE_DIR} -name \\*.cc|cccc --outdir=cppMetrics - \;
  tar jcvf cppMetrics.tar.bz2 cppMetrics\;
  )

CONFIGURE_FILE(${SCRIPTDIR}/cl2html.sh.in ${GENERATED_DOCUMENTATION_DIR}/cl2html.sh @ONLY)
FOREACH(DAYS "" 7 30 90)
  ADD_CUSTOM_TARGET(cl2html${DAYS} cd ${GENERATED_DOCUMENTATION_DIR}\;
    sh cl2html.sh ${DAYS}\;
    )
ENDFOREACH(DAYS)

ADD_CUSTOM_TARGET(stats echo ALL STATS DONE DEPENDS cl2html slochtml cccc statcvs)
ADD_CUSTOM_TARGET(docs echo ALL DOCS DONE DEPENDS dox xsddoc neddoc)
