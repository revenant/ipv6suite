ADD_TEST(FastRANet MIPv6Network -f MIPv6Network.ini -r 3)
ADD_TEST(checkFastRANet /usr/bin/bash -c "bzcat MIPv6Network-r3-ro.out.bz2|diff -u - test.out")

ADD_TEST(FastRANetPingPong MIPv6Network -f MIPv6Network.ini -r 4)
ADD_TEST(checkFastRANetPingPong /usr/bin/bash -c "bzcat MIPv6Network-r4-ro.out.bz2|diff -u - test.out")

ADD_TEST(FastRANetRT MIPv6Network -f MIPv6Network.ini -r 5)
ADD_TEST(checkFastRANetRT /usr/bin/bash -c "bzcat MIPv6Network-r5.out.bz2|diff -u - test.out")

ADD_TEST(FastRANetRTPingPong MIPv6Network -f MIPv6Network.ini -r 6)
ADD_TEST(checkFastRANetRTPingPong /usr/bin/bash -c "bzcat MIPv6Network-r6.out.bz2|diff -u - test.out")

#fix routes
#ADD_TEST(PCOAFNet MIPv6Network -f PCOAForwarding.ini -r 1)

#missing default route ask Eric
#ADD_TEST(MIPv6NetworkSiumlMov2 MIPv6Network -f MIPv6NetworkSimulMove.ini -r 5)
