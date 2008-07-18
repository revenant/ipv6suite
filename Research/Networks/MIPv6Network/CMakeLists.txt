#Turn it on for non RT scenarios to show similar results to with no RR.
#Test new xml parameters and simple ping 
ADD_TEST(FastRANet MIPv6Network -f MIPv6Network.ini -r 3)
COLLECT_CHECK_TEST(MIPv6Network-r3-ro)

ADD_TEST(FastRANetPingPong MIPv6Network -f MIPv6Network.ini -r 4)
COLLECT_CHECK_TEST(MIPv6Network-r4-ro)

#Run 3 is worse than this as RT goes through HA and updating HA is
#quicker than binding with CN
ADD_TEST(FastRANetRT MIPv6Network -f MIPv6Network.ini -r 5)
COLLECT_CHECK_TEST(MIPv6Network-r5)

ADD_TEST(FastRANetRTPingPong MIPv6Network -f MIPv6Network.ini -r 6)
COLLECT_CHECK_TEST(MIPv6Network-r6)

ADD_TEST(FastRANetRTPingPongHome MIPv6Network -f MIPv6Network.ini -r 8)
COLLECT_CHECK_TEST(MIPv6Network-r8)

#EBU is older version without piggybacking
ADD_TEST(FastRANetEBU MIPv6Network -f MIPv6Network.ini -r 13)
COLLECT_CHECK_TEST(MIPv6Network-r13)

#same as previous except no peer support for ebu 
#This test is giving back 64% drops obviously not correct
ADD_TEST(FastRANetEBUCompat MIPv6Network -f MIPv6Network.ini -r 14)
COLLECT_CHECK_TEST(MIPv6Network-r14)

#fix routes
#ADD_TEST(PCOAFNet MIPv6Network -f PCOAForwarding.ini -r 1)

#missing default route ask Eric
#ADD_TEST(MIPv6NetworkSiumlMov2 MIPv6Network -f MIPv6NetworkSimulMove.ini -r 5)

#ADD_TEST(EarlyBUTurnOff perl -i -pwe s|earlyBU='on'|earlyBU='off'|g MIPv6FastRANetwork.xml)
