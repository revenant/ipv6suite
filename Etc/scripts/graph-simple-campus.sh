

#DEBUG=y
############Main
SHDIR=~/bash
# Directory to store plots and output data obtained under subdir $FILENAME
DATADIR=~/src/phantasia/master/output
# Dir for ipv6suite sources (part of cps fuction too)
#SOURCEDIR=~/src/IPv6Suite
SOURCEDIR=~/src/IPv6SuiteWithINET
#SRCDIR=~/src/IPv6Suite-cvsbuildtest/IPv6Suite
#SCRIPTDIR=~/src/IPv6Suite/Etc/scripts
SCRIPTDIR=${SOURCEDIR}/Etc/scripts

# Retrieve bash cps functions and convnofastras
. $SHDIR/functions


echo "$0 $*"

NOTIMEOUT="y"

#Set to p to make it parallel
PARALLEL=$1
#Time to wait is TIMETORUN*CHECKTIME before considering run has gone wild and
#will be killed
TIMETORUN=$2
#Sleeps this many seconds before checking if sim has finished either by grepping
#for finishing line or as stated above (see graph-omnetpp-runs.sh)
CHECKTIME=$3
#END RUN number
NUMBEROFRUNS=$4

if [ $# -eq 5 ]; then
    BEGINRUNNUMBER=$5
    #TODO backup seed.txt and delete the first BEGINRUNNUMBER-1 seeds
else
    BEGINRUNNUMBER=0
fi

if [ "$TIMETORUN" = "" ]; then
    TIMETORUN=100
fi
if [ "$PARALLEL" = "" ]; then
    PARALLEL=s
fi
if [ "$CHECKTIME" = "" ]; then
    CHECKTIME=10
fi

#SRCDIR=~/src/IPv6Suite-cvsbuildtest/IPv6Suite
SRCDIR=$SOURCEDIR
#TOPDIR=/local/cronbuild/IPv6Suite-eh-cwd
#Do not use NFS home drive otherwise will not work
#TOPDIR=$SRCDIR
TOPDIR=/local/cronbuild/IPv6SuiteWithINET



##Configuration specific things (some configs are build differently others only
##differ in run number of xml file)

EXDIRNAMES="HMIPv6Network"
#EXDIRNAMES="MIPv6Network"

SIMDIR=Examples/IPv6/${EXDIRNAMES}
SIMRUN=1
SIMEXE=./${EXDIRNAMES}
INIFILE=CampusEH
XMLFILE=$INIFILE
BEGINSIMTIME=10 #Used by graph-plot-graph.sh
SIMTIMELIMIT="81.5" #set to diff value by each config (limit in ini file to 2 decimal places.) Make sure ping will have times exceeding this as graph-omnetpp-runs.sh checks for this too as a proper run.

NETNAME=`grep "network =" $TOPDIR/$SIMDIR/$INIFILE.ini |cut -d " " -f 3` #mipv6fastRANet
#Assumes output vector filename is omnetpp.vec if not change or make
#graphomnetpp-runs enforce this via modding ini file

echo topdir is $TOPDIR 
pushd $TOPDIR &>/dev/null
if [ $? -ne 0 ]; then
    echo "Failed to change to $TOPDIR"
    exit;
fi

#Assuming things made
pushd $SIMDIR #&> /dev/null

cp -p $XMLFILE.xml{,.orig}

#CONFIGURATIONS="hmip-sait-ro hmip-sait-noro hmip-sait-hmip"
CONFIGURATIONS="hmip-sait-ro hmip-sait-hmip mip-sait"
#CONFIGURATIONS="hmip-sait-ro"

for conf in $CONFIGURATIONS
do
FILENAME=$conf
#Change Run number depending on conf
if [ "$conf" = "hmip-sait-noro" ]; then
  cp -p $XMLFILE.xml{.orig,}
  perl -i -pwe 's|routeOptimisation="on"|routeOptimisation="off"|g' $XMLFILE.xml
  perl -i -pwe "s|routeOptimisation='on'|routeOptimisation='off'|g" $XMLFILE.xml
fi
if [ "$conf" = "hmip-sait-hmip" ]; then
  cp -p $XMLFILE.xml{.orig,}
  perl -i -pwe 's|edgeHandoverType="Timed"||g' $XMLFILE.xml
  perl -i -pwe "s|edgeHandoverType='Timed'||g" $XMLFILE.xml
fi
if [ "$conf" = "mip-sait" ]; then
    EXDIRNAMES="MIPv6Network"
    SIMEXE=./${EXDIRNAMES}
    SIMDIR=Examples/IPv6/${EXDIRNAMES}
    popd
    pushd $SIMDIR &> /dev/null
    cp -p $XMLFILE.xml{.orig,}
    perl -i -pwe 's|edgeHandoverType="Timed"||g' $XMLFILE.xml
    perl -i -pwe "s|edgeHandoverType='Timed'||g" $XMLFILE.xml
    perl -i -pwe 's|hierarchicalMIPv6Support="on"||g' $XMLFILE.xml
    perl -i -pwe "s|hierarchicalMIPv6Support='on'||g" $XMLFILE.xml
fi
if [ ! -f $SCRIPTDIR/graph-omnetpp-runs.sh ]; then
    SCRIPTDIR=$SOURCEDIR/Etc/scripts
fi
    echo $conf
. $SCRIPTDIR/graph-omnetpp-runs.sh
done
#cp -p $XMLFILE.xml{.orig,}
