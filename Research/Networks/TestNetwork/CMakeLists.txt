#TCPNode.ned no longer exists
#ADD_TEST(PingTestWithOldTCPNode TestNetwork -r 2)
ADD_TEST(VideoStream4 TestNetwork -r 4)
ADD_TEST(VideoStream5 TestNetwork -r 5)
ADD_TEST(move5 /bin/mv test.out test5.out)
ADD_TEST(VideoStream6 TestNetwork -r 6)
ADD_TEST(move6 /bin/mv test.out test6.out)
ADD_TEST(VideoStream7 TestNetwork -r 7)
ADD_TEST(newXMLNoDiff ${BASH} -c "diff -u test6.out test.out")
ADD_TEST(PingStreamODAD TestNetwork -r 8)
COLLECT_CHECK_TEST(PingStreamODAD)
ADD_TEST(PingStream TestNetwork -r 9)
COLLECT_CHECK_TEST(PingStream)
ADD_TEST(RtrAdvInt TestNetwork -r 10)
ADD_TEST(RtrAdvIntMIPv6 TestNetwork -r 11)
ADD_TEST(ValidateSignalling ${RUBY_EXECUTABLE} test_TestNetwork.rb)
