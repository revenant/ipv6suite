ADD_TEST(FastRANet3 MIPv6Network -f MIPv6Network.ini -r 3)

ADD_TEST(FastRANet4 MIPv6Network -f MIPv6Network.ini -r 4)

#fix routes
#ADD_TEST(PCOAFNet MIPv6Network -f PCOAForwarding.ini -r 1)

#missing default route ask Eric
#ADD_TEST(MIPv6NetworkSiumlMov2 MIPv6Network -f MIPv6NetworkSimulMove.ini -r 5)
