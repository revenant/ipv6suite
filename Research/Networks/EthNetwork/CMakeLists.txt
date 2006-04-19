#will not work as it refers to the source dir version not the build tree's dir
#ADD_TEST(TestEthernetDir /bin/sh /home/jmll/scripts/opplink)
ADD_TEST(EthernetPing EthNetwork -r 1)
