#! /bin/sh

if [ $# -ne 2 ]
then
  echo "Please enter the name of scenario and the number of runs required (eg. <name> <no. of runs>)" 
  exit 1
fi

let "numberOfSeeds=${2}"
setRun=1

seedtool g 1 10000000 $numberOfSeeds > seeds.txt

for seed in `cat seeds.txt`;
do
     cp omnetpp${set}.ini omnetpp${set}-${setRun}.ini
     cat <<EOF >>omnetpp${set}-${setRun}.ini
[General]
output-vector-file = MN-MNComms-${setRun}.vec
seed-lcg32 = ${seed}
EOF
     ./cmdSimulMove2 -f omnetpp${set}-${setRun}.ini
     cat "MN-MNComms-${setRun}.vec" >> MN-MNComms_${1}.vec
     seed=`expr $seed + 1`
done
    
