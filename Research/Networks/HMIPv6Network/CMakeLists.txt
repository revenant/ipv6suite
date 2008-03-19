ADD_TEST(SimpleHMIP HMIPv6Network -f HMIPv6Simple.ini -r 1)
COLLECT_CHECK_TEST(SimpleHMIP)
#better than previous
ADD_TEST(SimpleHMIP-RO HMIPv6Network -f HMIPv6Simple.ini -r 4)
COLLECT_CHECK_TEST(SimpleHMIP-RO)
#better than both above.
ADD_TEST(SimpleHMIP-RO-ebu HMIPv6Network -f HMIPv6Simple.ini -r 5)
COLLECT_CHECK_TEST(SimpleHMIP-RO-ebu)
ADD_TEST(MapToMapHandover HMIPv6Network -f HMIPv6Sait.ini -r 1)
COLLECT_CHECK_TEST(MapToMapHandover)
ADD_TEST(MapToMapHandover-RO HMIPv6Network -f HMIPv6Sait.ini -r 2)
COLLECT_CHECK_TEST(MapToMapHandover-RO)
ADD_TEST(MapToMapHandover-RO-ebu HMIPv6Network -f HMIPv6Sait.ini -r 4)
COLLECT_CHECK_TEST(MapToMapHandover-RO-ebu)

#EH untested also ebu versions of these
#ADD_TEST(EarlyBUTurnOff perl -i -pwe s|earlyBU='on'|earlyBU='off'|g SaitEHCal.xml HMIPv6Sait-ro.xml)
ADD_TEST(EH1secondPing HMIPv6Network -f SaitEHCal.ini -r 1)
COLLECT_CHECK_TEST(EH1secondPing)
ADD_TEST(EHUDPStream HMIPv6Network -f SaitEHCal.ini -r 3)
COLLECT_CHECK_TEST(EHUDPStream)
ADD_TEST(EHPingStream HMIPv6Network -f SaitEHCal.ini -r 4)
COLLECT_CHECK_TEST(EHPingStream)
ADD_TEST(EH10msPing HMIPv6Network -f SaitEHCal.ini -r 2)
COLLECT_CHECK_TEST(EH10msPing)

