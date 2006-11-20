ADD_TEST(FastRANet MIPv6Network -f MIPv6Network.ini -r 3)
COLLECT_CHECK_TEST(MIPv6Network-r3-ro)

ADD_TEST(FastRANetPingPong MIPv6Network -f MIPv6Network.ini -r 4)
COLLECT_CHECK_TEST(MIPv6Network-r4-ro)

ADD_TEST(FastRANetRT MIPv6Network -f MIPv6Network.ini -r 5)
COLLECT_CHECK_TEST(MIPv6Network-r5)

ADD_TEST(FastRANetRTPingPong MIPv6Network -f MIPv6Network.ini -r 6)
COLLECT_CHECK_TEST(MIPv6Network-r6)

##Haven't fixed and will prob. not return home
#ADD_TEST(FastRANetRTHome MIPv6Network -f MIPv6Network.ini -r 7)
#COLLECT_CHECK_TEST(FastRANetRTHome)

#ADD_TEST(FastRANetRTPingPongHome MIPv6Network -f MIPv6Network.ini -r 8)
#COLLECT_CHECK_TEST(FastRANetRTPingPongHome)

#fix routes
#ADD_TEST(PCOAFNet MIPv6Network -f PCOAForwarding.ini -r 1)

#missing default route ask Eric
#ADD_TEST(MIPv6NetworkSiumlMov2 MIPv6Network -f MIPv6NetworkSimulMove.ini -r 5)
