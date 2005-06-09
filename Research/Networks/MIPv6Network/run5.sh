#! /bin/sh

if [ $# -ne 1 ]
then
  echo "Please enter the number of runs required (eg. <no. of runs>)" 
  exit 1
fi

let "numberOfSeeds=${1}"
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
     cat "MN-MNComms-${setRun}.vec" >> MN-MNComms.vec
     seed=`expr $seed + 1`
done
    
