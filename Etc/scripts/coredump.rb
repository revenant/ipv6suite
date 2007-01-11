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
  next if corefile !~ /[.]\d+$/
  dump = `gdb #{exe} -q -x debug.txt #{corefile}|grep ^\#`
  if dump =~ /\?\?/m
  puts "#{corefile} is stuffed i.e. no bt so deleting"
  File.delete(corefile)
else

#omit address of frame pointer and argument addresses which vary across process
#invocations as we used different seeds

dump.gsub!(/(\s[[:xdigit:]]x[[:xdigit:]]+\s)|\S[[:xdigit:]]x[[:xdigit:]]+/," ")
dumps << dump if not dumps.include?(dump)
index = dumps.index(dump)
if hash.include? index
  hash[index] << corefile 
else
  hash[index] = [corefile]
end
end
}
puts "There were #{dumps.size} unique dumps out of #{hash.size}"
puts dumps.to_s
puts hash.each{|k,v|
  puts v[0] + " belongs to dump #k" 
  v.each_index{|i| 
    next if i == 0
    File.delete(v[i])
  }
}

#todo iterate dumps by index and output the line numbers for the first corefile
#in hash[index]
