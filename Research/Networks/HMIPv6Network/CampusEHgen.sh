ruby ~/src/IPv6SuiteWithINET/Etc/scripts/NedFile.rb -m 50 -M -p -n CampusEH -r 20
ruby ~/src/IPv6SuiteWithINET/Etc/scripts/NedFile.rb -m 50 -M -p -n CampusEHmip -r 20
ruby ~/src/IPv6SuiteWithINET/Etc/scripts/NedFile.rb -m 50 -M -p -n CampusEHhmip -r 20
perl -i -pwe "s|hierarchicalMIPv6Support='on'||g" CampusEHmip.xml
perl -i -pwe "s|edgeHandoverType='Timed'||g"  CampusEHhmip.xml
#Further changes for  hmip using static assigned HA
#XML
#turn off all maps
#for all ars turn on advSendAdv for PPP interfaces connected to other routers so map options get forwarded. (should add option in NDStateRouter to detect incoming RtrAdv with mapoption and start sending our own rtrAdv?)
#cr1 needs to have homeagent, hmip and map = on also all its ppp ifaces connectted to other routers need to advertise the iface address as a map option.

ruby << EOF > run.txt
#Write bit for actual runs in omnetpp.ini and use auto seed gen from run number
#recommended by Andras to STeve
runcount = 60
modname = "CampusEH"
netname = "HMIPv6Network"
#configs = [ "", "hmip", "mip"]
configs = [ "", "hmip", "mip"]
configs.each{|c| 
	c.insert(0, modname) 
	c=c+".ini"
}
configs.each{|c|
1.upto(runcount) do |i|
      vectorfile = c+ i.to_s + ".vec"
      scalarfile = c+ i.to_s + ".sca"
puts "[Run #{i}]"
puts "output-vector-file = #{vectorfile}"
puts "output-scalar-file = #{scalarfile}"
end

#write distjobs file
1.upto(runcount) do |i|
puts "./#{netname} -f #{c} -r #{i}"
end
}
EOF
scp run.txt CampusEH* supersupreme.ctie.monash.edu.au:
