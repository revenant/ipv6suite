#TCPNode.ned no longer exists
#ADD_TEST(PingTestWithOldTCPNode TestNetwork -r 2)
ADD_TEST(VideoStream4 TestNetwork -r 4)
ADD_TEST(VideoStream5 TestNetwork -r 5)
ADD_TEST(move /bin/mv test.out test5.out)
ADD_TEST(VideoStream6 TestNetwork -r 6)
#COLLECT_CHECK_TEST(VideoStream)
ADD_TEST(ValidateSignalling ${RUBY_EXECUTABLE} test_TestNetwork.rb)
