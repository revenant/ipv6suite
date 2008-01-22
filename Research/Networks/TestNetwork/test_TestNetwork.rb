
# {{{ Unit Test #

##Unit test for this class/module
require 'test/unit'
require 'bigdecimal'
require 'bigdecimal/math'

class TC_TestNetwork < Test::Unit::TestCase 
#include RStuff
def test_rtrSolicitationSeparation
testvec = 'VideoStream.vec'
vecnum = `grep 'RS sent' #{testvec}|cut -f 2 -d ' '`.split("\n")
rtrsols = `grep ^#{vecnum[0].to_i}  #{testvec}|cut -f 2`.split("\n")
rtrsols.collect!{|x| x.to_f}
assert( rtrsols.length == 3, "max_rtr_sol is 3 by default but got #{rtrsols.length}")

ans1=(rtrsols[0]-rtrsols[1]).abs 
assert( ans1== 4, "rtr_sol_interval is 4 by default but got #{ans1}")
ans2=(rtrsols[1]-rtrsols[2]).abs 
assert(ans2 == 4,"rtr_sol_interval is 4 by default but got #{ans2}")
end

def test_DAD
  #very first NS delayed by uniform(0,max_rtr_sol_delay) which is 1 by defualt
  #In fact acc. to rfc4862 all 1st NS done for addr configured from
  #multicast RA need this as all unicast addr need DAD now
  #we'll use odad to avoid it 
  #NS separated RETRANS_TIMER s (1)

#make it test 2 vec one with HostDupAddrDetectTransmits = 4 and other with standard of 1

  #standard of 1 occurs when no xml config file specified this occurs
  #in run 4
  testvec = 'VideoStream.vec'
  vecnum = `grep 'NS sent' #{testvec}|cut -f 2 -d ' '`.split("\n")
ngbrsols = `grep ^#{vecnum[0].to_i}  #{testvec}|cut -f 2`.split("\n")
ngbrsols.collect!{|x| x.to_f}
assert(ngbrsols.length == 1, "no of ngbrsols of r4 should only be one and is separated from addr assigned by 1")

#testing with xml file  run 6
  testvec = 'VideoStreamc.vec'
#client 1 with 2 addresses and dtd defaults for retrans timer
  vecnum = `grep 'NS sent' #{testvec}|grep client1|cut -f 2 -d ' '`.split("\n")
ngbrsols = `grep ^#{vecnum[0].to_i}  #{testvec}|cut -f 2`.split("\n")
ngbrsols.collect!{|x| x.to_f}
ans1=(ngbrsols[0]-ngbrsols[1]).abs
assert( ngbrsols.length == 8, "TestNetwork.xml HostDupAddrDetectTransmits is 4 by default but got #{ngbrsols.length}")
assert( ans1== 0.12, "Default retrans_timer in netconf2.dtd is 0.12 but got #{ans1}")
ans2=(ngbrsols[1]-ngbrsols[2]).abs
assert(ans2 == 0.12,"Default retrans_timer in netconf2.dtd is 0.12 but got #{ans2}")
ans3=(ngbrsols[2]-ngbrsols[3]).abs
assert(ans3 == 0.12,"Default retrans_timer netconf2.dtd is 0.12 but got #{ans2}")

#for client2 with dtd defaults
  vecnum = `grep 'NS sent' #{testvec}|grep client2|cut -f 2 -d ' '`.split("\n")
ngbrsols = `grep ^#{vecnum[0].to_i}  #{testvec}|cut -f 2`.split("\n")
ngbrsols.collect!{|x| x.to_f}
assert( ngbrsols.length == 2, "TestNetwork.xml is 4 by default but got #{ngbrsols.length}")
ans1=(ngbrsols[0]-ngbrsols[1]).abs

assert( ans1 - 1 < @diffConsideredZero,
        "Default retrans_timer in netconf2.dtd is 0.12 but got #{ans1}")

end

def test_AR
  #MAX_MULTICAST_SOL times NS separated by at least RETRANS_TIMER
 #AR NS not recorded in vec files
ngbrsols = `grep -v Vid test5.out|grep 'NS(AR' |egrep '>'|cut -d ' ' -f 3`.split("\n")
ngbrsols.collect!{|x| x.to_f}
ans1=(ngbrsols[0]-ngbrsols[1]).abs 
assert( ngbrsols.length == 3, "max_multi_sol is 3 by default but got #{ngbrsols.length}")
assert( ans1== 1, "retrans_timer is 1 by default but got #{ans1}")
ans2=(ngbrsols[1]-ngbrsols[2]).abs 
assert(ans2 == 1,"retrans_timer is 1 by default but got #{ans2}")
end

def setup
  @diffConsideredZero = BigDecimal.new("1e-7")
end

end
