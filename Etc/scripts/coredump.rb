File.open("debug.txt", "w"){|f|
f.puts <<END
bt
q
END
}
hash = {}
dumps = []
exe = File.basename(Dir.pwd)
Dir["core.*"].each{|corefile|
  dump = `gdb #{exe} -q -x debug.txt #{corefile}|grep ^\#`
  if dump =~ /\?\?/m
  puts "#{corefile} is stuffed i.e. no bt so deleting"
  File.delete(corefile)
else
  #puts dump.gsub(/^[^#].*$/m,"")
  #dump = `echo #{dump}|grep ^#` 
#dump.split('\n').each{|l|
 #puts l if l =~ /^#.*$/
#}

if false
puts "corefile #{corefile} dumped:"
puts dump
end
dumps << dump if not dumps.include?(dump)
hash[corefile] = dumps.index(dump)
end
}
puts "There were #{dumps.size} unique dumps out of #{hash.size}"
puts dumps.to_s
