#TCPNode.ned no longer exists
#ADD_TEST(PingTestWithOldTCPNode TestNetwork -r 2)
ADD_TEST(VideoStream TestNetwork -r 4)
COLLECT_CHECK_TEST(VideoStreamSignalling)
