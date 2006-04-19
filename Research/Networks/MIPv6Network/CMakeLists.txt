ADD_TEST(FastRANet MIPv6Network -f MIPv6Network.ini -r 3)

ADD_TEST(PCOAFNet MIPv6Network -f PCOAForwarding.ini -r 1)

ADD_TEST(MIPv6NetworkSiumlMov2 MIPv6Network -f MIPv6NetworkSimulMove.ini -r 5)
