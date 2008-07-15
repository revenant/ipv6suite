ADD_TEST(VoipTest Baseline -f Voip.ini -r1)
COLLECT_CHECK_TEST(VoipTest)

ADD_TEST(Immobile1msPing Baseline -f Baseline.ini -r 1)
COLLECT_CHECK_TEST(Immobile1msPing)

#something wrong with mobile versions as other side not receiving
#ADD_TEST(Immobile1msPingMIPv6 Baseline -f Baseline.ini -r 2)
#COLLECT_CHECK_TEST(Immobile1msPingMIPv6)

#I thought should be same as result in r1 but "minisculely" worse
#ADD_TEST(MobilePing Baseline -f Baseline.ini -r 3)
#COLLECT_CHECK_TEST(MobilePing)

#ADD_TEST(MobilePingMIPv6 Baseline -f Baseline.ini -r 4)
#COLLECT_CHECK_TEST(MobilePingMIPv6)


#Shadowing without movement
#ADD_TEST(ImmobileShadowing10ms Baseline -r 5)
#Shadowing with movement
#ADD_TEST(FastRANet Baseline -r 6)
