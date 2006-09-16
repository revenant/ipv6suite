require "datasorter";
require "pp"

#test exception handling
#a=Datasorter::ScalarManager.new.loadFile("/home/jmll/nosuch.vec")

sm=Datasorter::ScalarManager.new
ds=Datasorter::DataSorter.new(sm)

#f=sm.loadFile("/home/jmll/other/IPv6SuiteWithINET/Research/Networks/EHComp/EHComp_eh_500_50_y_1.vec")
Dir["/home/jmll/other/IPv6SuiteWithINET/Research/Networks/EHComp/EHComp_eh_*_20*.sca"].each{|file|

puts file
f=sm.loadFile(file)
#p sm.values.length

#both stringsets
#p sm.scalars.keys
#p sm.modnames.keys
}

p sm.runs.length
#how do I wrap this shit
ints = ds.getFilteredScalarList("*", "*.udpApp[*]", "* % *")

p sm.runs.length

#sm.runs.each{|r|
#  p r.fileAndRunName
#}

p ints.length

ints.each{|i|
 datum=sm.getValue(i)
#date is empty

#datum.run.fileAndRunName is useless for me because I aready encode the factors
#into the network part of modname

#runName is not great either as it includes the line number. If I was to check
#other scalars besides that one visually in file or scan others may be of use
#but not now

p datum.run.runNumber.to_s + " " + datum.modname +  " " +datum.scalar +  " " +datum.value.to_s

#network name at prefix of modname and node at 2nd field when split by .
p datum.modname.split(".",3)
#see tkcmd.cc getListboxLine_cmd for rest of display datum
#p datum.run.fileAndRunName, datum.run.runName
}
