#! /bin/sh

if [ $# -ne 2 ]
then
  echo "Please enter the name of scenario and the number of runs required (eg. <name> <no. of runs>)" 
  exit 1
fi

let "numberOfSeeds=${2}"
setRun=1
             
cp SimulMove2.ini SimulMove2_${1}.ini
seedtool g 1 10000000 $numberOfSeeds > seeds.txt
for seed in `cat seeds.txt`;
do
     cat <<EOF >>SimulMove2_${1}.ini
[Run ${setRun}]
network = mipv6NetworkSimulMove2
output-vector-file = MN-MNComms-${setRun}.vec
seed_lcg32 = ${seed}
include ../../../Etc/default.ini
EOF
     ../../../bin/INET -u Cmdenv -f SimulMove2_${1}.ini -r ${setRun}
     cat "MN-MNComms-${setRun}.vec" >> MN-MNComms_${1}.vec
#     rm omnetpp-${setRun}.ini
     setRun=`expr $setRun + 1`
done
    
