%{!?cvsdate: %{expand: %%define cvsdate %%(/bin/date +"%Y%m%d")}}
%define cvsdate p1
#define cvsdate pre4
#define cvsdate %nil
%{!?libcwd: %define libcwd 0 }
%{?l_prefix: %define openpkg 1}
%{!?l_prefix: %define openpkg 0}
%{!?shared_libs %define shared_libs 1}

%define srcext tgz
#define srcext tar.bz2

#Fix FC3 and above unset of DISPLAY by saving it as configure test of wish needs it
%{!?display: %{expand: %%define display %%(echo $DISPLAY)}}

%define doxygen_version %(/bin/rpm -q doxygen)
%define fedora_dist %(cat /etc/issue|grep -ic fedora)
%define yoper_dist %(cat /etc/issue|grep -ic yoper)
%{!?no_doxygen: %{expand: %%define no_doxygen %%(echo %%{doxygen_version}|grep -c not)}}
%if "%{no_doxygen}" == "0"
  %define graphviz_version %(/bin/rpm -q graphviz)
  %{!?no_graphviz: %{expand: %%define no_graphviz %%(echo %%{graphviz_version} |grep -c not)}}
#Andras is building doc by default in src release
%define no_graphviz 1
%endif

%define compiler %(echo $CC $CXX)
%{!?use_icc: %{expand: %%define use_icc %%(echo %%{compiler}|grep -c icc)}}

#CC=icc CXX=icpc rpmbuild -ba --define "cvsdate 20031007" omnetpp.spec
%if "%{use_icc}" == "1"
  %define modoptflags %(echo %{optflags} -g|sed 's|-O.|-O0|')
  %define icctag icc
  %define _prefix /opt
%else
  %define modoptflags %(echo %{optflags} |sed 's|-fomit-frame-pointer| |'|sed 's|-momit-leaf-frame-pointer| |'|sed 's|-O.|-O0|')
  %define icctag %nil
%endif

%define myrelease 1
#####################################################

Summary: OMNeT++ is a discrete event simulation tool
Name: omnetpp
Version: 3.2
#Version: 2.3_%{cvsdate}
Release: %{myrelease}%{cvsdate}%{icctag}
#Release: 0.%{cvsdate}_%{myrelease}%{icctag}
URL: www.omnetpp.org
Source0: http://whale.hit.bme.hu/omnetpp/download/release/%{name}-%{version}%{cvsdate}.%{srcext}
%if "%{no_graphviz}" == "0"
Source1: Doxyfile
%endif

License: Academic Public License
Group: Applications/Engineering
%if "%{openpkg}" == "0"
BuildRoot: %{_tmppath}/%{name}-root
Requires: tk >= %{expand: %%(rpm -q --qf %%{VERSION} tk)} perl libxml2
%if "%{fedora_dist}" == "1"
BuildRequires: tcl-devel >= 8.3 tk-devel >= 8.3 
%endif
%if "%{yoper_dist}" == "0"
BuildRequires: libxml2-devel
%endif
BuildRequires: tk >= 8.3 bison
#Don't know why tcl and tk packages don't Provide this
Provides: libtcl.so libtk.so
%if "%{use_icc}" == "1"
#version in caps otherwise reverts to the version of omnetpp instead
#BuildRequires: intel-icc7 = %{expand: %%(rpm -q --qf %%{VERSION} intel-icc7)}
#Requires: intel-icc7  = %{expand: %%(rpm -q --qf %%{VERSION} intel-icc7)}
BuildRequires: intel-icc8 = %{expand: %%(rpm -q --qf %%{VERSION} intel-icc8)}
Requires: intel-icc8  = %{expand: %%(rpm -q --qf %%{VERSION} intel-icc8)}
%endif
%if "%{libcwd}" == "1"
%if "%{use_icc}" == "1"
Error: Cannot use libcwd when compiling for intel 
%endif
BuildRequires: libcwd = %{expand: %%(rpm -q --qf %%{VERSION} libcwd)}
Requires:  libcwd = %{expand: %%(rpm -q --qf %%{VERSION} libcwd)}
#0.99.26
#for omnetpp 3.x need libcwd == 0.99.33 (34 version changes namespace names)
%endif

%else
Class: Eval
BuildRoot: %{l_buildroot}
AutoReq:  no
#Openpkg overrides Required for omnet configure
%define _build	i686-pc-linux-gnu
%define _host	%{_build}
%endif


#####################################################

%define oshare %{_datadir}/%{name}-%{version}
%define oinclude %{_includedir}/%{name}
%define odoc %{_docdir}/%{name}-%{version}

##########################################################

%description
OMNeT++ is a discrete event simulation tool developed by Andras Varga. It was
primarily designed to simulate computer networks, multi-processors and other
distributed systems, but it may be useful for modelling other systems as
well. In the past few years, OMNeT++ become a popular network simulation tool in
the scientific community as well as in industrial settings.

If you're unsure whether OMNeT++ suits your needs, this page
http://whale.hit.bme.hu/omnetpp/links.htm may help you find out.

Using optflags=%{modoptflags}
With libcwd malloc checking in libenvir-cw.so=%{libcwd}

##########################################################

%prep
%setup -q -n %{name}-%{version}%{cvsdate}


##########################################################

%build
if [ "x$DISPLAY" == "x" ]; then
#Newer distributions aka fc3 unset display and wish/blt test both need X
  export DISPLAY="%{display}"
  if [ x"%{display}" == 'x' ]; then
    echo please rerun with rpm --rebuild --define \"display \$DISPLAY\"
    exit 1
  fi
fi
export PATH=`pwd`/bin:$PATH:
export LD_LIBRARY_PATH=`pwd`/lib
%if "%{use_icc}" == "1"
  #source /opt/intel/compiler70/ia32/bin/iccvars.sh
  source /opt/intel_cc_80/bin/iccvars.sh
%endif
#rm configure.user
cat<< EOF >> configure.user
#RPM additions
CFLAGS='${CFLAGS:-%{modoptflags}}'
#AKAROA_CFLAGS=
#AKAROA_LIBS=
# Set to "yes" to enable simulation executables load NED files dynamically.
WITH_NETBUILDER=yes
# Set to "yes" to enable the parallel distributed simulation feature.
WITH_PARSIM=yes
#Libxml2
XMLPARSER=libxml
LIBXML_CFLAGS="`xml2-config --cflags`"
LIBXML_LIBS="`xml2-config --libs`"
XML_LIBS=-lerror
XML_CFLAGS=
#MPI_CFLAGS="-I /usr/include/mpi2c++"
#MPI_LIBS=
#problems here! Tries to output to these directories
#OMNETPP_SAMPLES_DIR="%{odoc}/samples"
#OMNETPP_TUTORIAL_DIR="%{odoc}/tutorial"
OMNETPP_BITMAP_PATH=".;./bitmaps;%{oshare}/bitmaps"
#Build process tries to delete things so this is also a no no
#OMNETPP_BIN_DIR="%{_bindir}"
#Build process will use this directory :()
#OMNETPP_INCL_DIR="%{oinclude}"
#Not necessary because this is a default library path
#OMNETPP_LIB_DIR="%{_libdir}"
OMNETPP_TKENV_DIR="%{oshare}/tkenv"
OMNETPP_GNED_DIR="%{oshare}/gned"
OMNETPP_PLOVE_DIR="%{oshare}/plove"
%if "%{shared_libs}" == "0"
build_shared_libs=no
%endif
TK_CFLAGS="-I%{_prefix}/X11R6/include"
BLT_LIBS="-L%{_libdir}/blt2.4 -lBLT24"
EOF


#Hopefully this fixes mandrake's missing macro
%{!?_smp_mflags:  %{expand: %%define _smp_mflags  %%([ -z "$RPM_BUILD_NCPUS" ] && RPM_BUILD_NCPUS="`/usr/bin/getconf _NPROCESSORS_ONLN`"; [ "$RPM_BUILD_NCPUS" -gt 1 ] && echo "-j$RPM_BUILD_NCPUS")}}

%configure
#Set executable bits for any scripts that need it (particularly opp_msgc otherwise we use the one installed on the system)
#_scripts/setperm #does not work either as it sets output targets which don't exist at this stage
chmod 755 src/nedc/opp_msgc
make
#make manual
make tests
#Still cannot build in parallel prob cause dependencies are wrong, always missing nedxml lib
#%{?_smp_mflags}  

##########################################################

%install
rm -rf $RPM_BUILD_ROOT
%if "%{cvsdate}" == ""
%define no_graphviz 1
%endif
%if "%{no_graphviz}" == "0"
cp -p %{SOURCE1} .
doxygen Doxyfile
%endif

perl -i -pwe 's|^OMNETPP_INCL_DIR=[^@].*|OMNETPP_INCL_DIR=%{_includedir}\/%{name}|g' bin/opp_makemake
perl -i -pwe 's|^OMNETPP_LIB_DIR=[^@].*|OMNETPP_LIB_DIR=%{_libdir}|g' bin/opp_makemake
mkdir -pv $RPM_BUILD_ROOT%{_bindir}
mkdir -pv $RPM_BUILD_ROOT%{_libdir}
mkdir -pv $RPM_BUILD_ROOT%{oinclude}
mkdir -pv $RPM_BUILD_ROOT%{oshare}/bitmaps
mkdir -pv $RPM_BUILD_ROOT%{oshare}/tkenv
mkdir -pv $RPM_BUILD_ROOT%{oshare}/gned
mkdir -pv $RPM_BUILD_ROOT%{oshare}/plove
find . -name 'CVS'| xargs rm -fr 
cp -pr include/* $RPM_BUILD_ROOT%{oinclude}
cp -p include/ChangeLog $RPM_BUILD_ROOT%{oinclude}
cp -p lib/*  $RPM_BUILD_ROOT%{_libdir}
chmod 755 bin/opp_msgc
cp -p bin/*  $RPM_BUILD_ROOT%{_bindir}
rm -f $RPM_BUILD_ROOT%{_bindir}/makemake
perl -i -pwe "s|NEDC=.*$|NEDC=%{_bindir}/nedc|g" $RPM_BUILD_ROOT%{_bindir}/opp_makemake
cp -pr bitmaps/* $RPM_BUILD_ROOT%{oshare}/bitmaps
cp -p src/tkenv/* $RPM_BUILD_ROOT%{oshare}/tkenv 
cp -p src/gned/* $RPM_BUILD_ROOT%{oshare}/gned
cp -p src/plove/*.{tcl,res,rc,ico} $RPM_BUILD_ROOT%{oshare}/plove
pushd $RPM_BUILD_ROOT%{oshare}
find . \( -name '*.o' -o -name '*.h' -o -name '*.cc' \) -exec rm {} \;
popd
pushd samples
find . -name '*.o' -exec rm {} \;
#todo fix the rundemo script for dir where runtcl file is 
popd
%if "%{libcwd}" == "1"
cp -p configure.user configure.user.normal
perl -i -pwe 's|^.*CFLAGS.*$||' configure.user
cat<< EOF >> configure.user
CFLAGS='${CFLAGS:-%{modoptflags} -DCWDEBUG -DEARLY_CWDEBUG}'
#LDFLAGS=-lcwd
EOF
%configure
pushd src/envir
make clean;make
mv libenvir.so $RPM_BUILD_ROOT%{_libdir}/libenvir-cw.so
popd
%endif
#Fix for newer versions of cp checking copying diff sources with same name
mv doc/README doc/README.doc
%if "%{openpkg}" == "1"
%{l_rpmtool} files -v -ofiles -r$RPM_BUILD_ROOT %{l_files_std}
%endif
##########################################################

%clean
rm -rf $RPM_BUILD_ROOT

##########################################################
%if "%{openpkg}" == "0"
%files
%defattr(-,root,root)
%doc README doc/* samples contrib
%{_bindir}/*
%{_libdir}/*
%{_includedir}/%{name}
%{oshare}
%else
%files -f files
%endif
##########################################################

%post
%if "%{openpkg}" == "0"
ldconfig
%endif
#ldconfig -r %{_prefix} 

##########################################################

%postun
%if "%{openpkg}" == "0"
ldconfig
%endif
#ldconfig -r %{_prefix} 

##########################################################

%if "%{openpkg}" == "0"
%changelog
* Fri Mar 25 2006 Johnny Lai <johnny.lai@eng.monash.edu.au> - 3.2p1-1
- updated to 3.2p1

* Fri Oct 28 2005 Johnny Lai <johnny.lai@eng.monash.edu.au> - 3.2-1
- updated to 3.2 final

* Thu Oct 13 2005 Johnny Lai <johnny.lai@eng.monash.edu.au> - 3.2-2pre4
- Updated to 3.2pre4

* Thu Jul 28 2005 Johnny Lai <johnny.lai@eng.monash.edu.au> - 3.2-2pre2
- Updated to 3.2pre2

* Fri Jul 22 2005 Johnny Lai <johnny.lai@eng.monash.edu.au> - 3.2-2pre1
- Updated to 3.2pre1
- Saves DISPLAY env var so no need to define in rpmbuild command

* Sat Jul  2 2005 Johnny Lai <johnny.lai@eng.monash.edu.au> - 3.1-2
- added define option for DISPLAY so can be built remotely too (as long as X11 forwarding is on)
- fixed detection of BLT by adding -L for fedora distros

* Sat Jun 11 2005 Johnny Lai <johnny.lai@eng.monash.edu.au> - 3.1-1 
- Update to 3.1
- copy all things in include dir

* Mon Jan 17 2005 Johnny Lai <johnny.lai@eng.monash.edu.au> - 3.0-1
- Update to 3.0 final
- copy bitmap dirs
- define cvsdate as %nil for non cvs releases

* Thu Dec  2 2004 Johnny Lai <johnny.lai@eng.monash.edu.au> 3.0-0.a9_12
- added rpmbuild option shared_libs.
- added FC3 DISPLAY fix for wish 

* Fri Nov 19 2004 Johnny Lai <johnny.lai@eng.monash.edu.au> 3.0-0.a9_11
- updated to a9

* Tue Sep 14 2004 Johnny Lai <johnny.lai@eng.monash.edu.au> 3.0pre-20040914-11
- Requires: libxslt removed since that was removed from IPv6Suite
- Requires: libxml2 added for omnet's xml parser DTD validation
- fixed build script to copy tests/core and building tests

* Sat Jul 31 2004 Johnny Lai <johnny.lai@eng.monash.edu.au> 3.0pre-20040731-11
- Fixed opp_makemake NEDC var and removed makemake

* Tue Jun  8 2004 Johnny Lai <johnny.lai@eng.monash.edu.au> 3.0pre_20040608-10
- Updated with const ned parameter fix
- Fixed spec to work for both RH and OpenPkg.

* Wed Jun  2 2004 Johnny Lai <johnny.lai@eng.monash.edu.au> 3.0pre_20040602-9
- Updated for gcc34 build on redhat. Added build for OpenPkg

* Sun May 16 2004 Johnny Lai <johnny.lai@eng.monash.edu.au> 3.0pre_20040516-9
- Updated to 3.0 preview 4

* Fri Apr 16 2004 Johnny Lai <johnny.lai@eng.monash.edu.au> 3.0pre_20040416-9
- Fixed delete [] flagged by excessive mpatrol warnings

* Tue Feb 17 2004 Johnny Lai <johnny.lai@eng.monash.edu.au> 3.0-pre_20040217-8
- Updated to 3.0 preview 1

* Wed Dec 31 2003 Johnny Lai <johnny.lai@eng.monash.edu.au> 2.3_20031231-8
- Updated to icc 8.0

* Sun Oct 12 2003 Johnny Lai <johnny.lai@eng.monash.edu.au> 2.3_20031012-7
- Added patch to remove valgrind/libcw warnings

* Wed Oct  1 2003 Johnny Lai <johnny.lai@eng.monash.edu.au> 2.3_20031009-6icc
- icc build has _prefix set to /opt now for simultaneous installation with
  normal builds

* Sun Sep 28 2003 Johnny Lai <johnny.lai@eng.monash.edu.au> 2.3_20030928-5icc
- Added intel compiler tag to revision. Tested icc build with: 
  alias iccget='source /opt/intel/compiler70/ia32/bin/iccvars.sh'
  iccget;CC=icc CXX=icpc rpmbuild -ba omnetpp.spec

* Mon Sep  8 2003 Johnny Lai <johnny.lai@eng.monash.edu.au> 2.3_20030908-4
- Fixed cvs date format from %e to %d

* Thu Aug 21 2003 Johnny Lai <johnny.lai@eng.monash.edu.au> 2.3_20030821-3
- Added generation of usman.html
- Set -O0 and -ggdb3 in modoptflags

* Thu Aug 14 2003 Johnny Lai <johnny.lai@eng.monash.edu.au> 2.3_20030816-2
- Testing distributed nedc compilation patch
- Patched Makefile.in to build on SMP machines i.e. make -j 2 

* Thu Aug 14 2003 Johnny Lai <johnny.lai@eng.monash.edu.au> 2.3_20030814-1
- Provide tcl.so and tk.so since RH packages don't provide those
- Updated CVS version has opp_msgc patch incorporated
- Automatically use current date
- Remove -momit-leaf-frame-pointer and -O? from optflags

* Mon Aug 11 2003 Johnny Lai <johnny.lai@eng.monash.edu.au> 2.3_20030811-1
- Real cvs version has opp_neddoc fix

* Wed Aug  6 2003 Johnny Lai <johnny.lai@eng.monash.edu.au> 2.3_20030801-8
- Fix opp_makemake -L missing lib path (make fails link stage)

* Fri Aug  1 2003 Johnny Lai <johnny.lai@eng.monash.edu.au> 2.3_20030801-7
- Updated to CVS version

* Sun Jul  6 2003 Johnny Lai <johnny.lai@eng.monash.edu.au> 2.3-7
- Remove -fomit-frame-pointer from optflags
- Added opp_msgc.patch to support cmake omnetpp plugin's OPP_WRAP_MSGC command

* Fri Jun 27 2003 Johnny Lai <johnny.lai@eng.monash.edu.au> 2.3-6
- Generate full Doxygen option added for omnetpp.tag

* Thu Jun 26 2003 Johnny Lai <johnny.lai@eng.monash.edu.au> 2.3-5
- Reverted includedir to be specified after build process

* Wed Jun 25 2003 Johnny Lai <johnny.lai@eng.monash.edu.au> 2.3-4
- Added TKENV_DIR to build settings and the missing tkenv/gned/plove dirs

* Sun Jun 22 2003 Johnny Lai <johnny.lai@eng.monash.edu.au> 2.3-3
- Added conditional patch build

* Fri Jun 20 2003 Johnny Lai <johnny.lai@eng.monash.edu.au>
- removed extraneous _includedir files. 
- Fixed opp_makemake with correct libdir and includedir
- Added correct bitmap path
- Include patch module array of size zero for omnet

* Thu Jun 19 2003 Johnny Lai <johnny.lai@eng.monash.edu.au>
- Initial build.
%endif

