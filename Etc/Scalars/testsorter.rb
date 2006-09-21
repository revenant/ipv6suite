require "datasorter";
require "pp"

#test exception handling
#a=Datasorter::ScalarManager.new.loadFile("/home/jmll/nosuch.vec")

sm=Datasorter::ScalarManager.new
ds=Datasorter::DataSorter.new(sm)


Dir["/home/jmll/other/IPv6SuiteWithINET/Research/Networks/EHComp/EHComp_eh_*_20*.sca"].each{|file|
  f=sm.loadFile(file)
#p sm.values.length

#both stringsets
#p sm.scalars.keys
#p sm.modules.keys
}

RSlave = "R --slave --quiet --vanilla --no-readline"
p = IO.popen(RSlave, "w+")

for node in [ "cn", "mn" ] 
  ints = ds.getFilteredScalarList("*", "*#{node}.udpApp[*]", "* % *")
  next if ints.size == 0
  values = ints.collect{|i| sm.getValue(i).value}
  p.puts %|a=c(#{values.join(",")});mean(a)|
  sum = values.inject{|sum, i| sum + i}
  mean = sum/ints.length
  puts "mean is #{mean} of #{values.length} values for #{node}"  
  #ignore the dimension of answer
  puts "mean according to R is #{p.gets.split(" ")[1]}"
end

exit

p sm.runs.length
ints = ds.getFilteredScalarList("*", "*.udpApp[*]", "* % *")

p sm.runs.length

p ints.length

ints.each{|i|
 datum=sm.getValue(i)
#date is empty

#datum.run.fileAndRunName is useless for me because I aready encode the factors
#into the network part of module

#runName is not great either as it includes the line number. If I was to check
#other scalars besides that one visually in file or scan others may be of use
#but not now

p datum.run.runNumber.to_s + " " + datum.module +  " " +datum.scalar +  " " +datum.value.to_s

#network name at prefix of module and node at 2nd field when split by .
p datum.module.split(".",3)
#see tkcmd.cc getListboxLine_cmd for rest of display datum
#p datum.run.fileAndRunName, datum.run.runName
}
