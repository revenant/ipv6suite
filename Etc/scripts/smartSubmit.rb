#! /usr/bin/env ruby
#
#  Author: Johnny Lai
#  Copyright (c) 2006 Johnny Lai
#
# =DESCRIPTION
#  Submit jobs to Sun grid engine
#  Assumes jobs are listed in a file named jobs.txt one process per line
#
#  Should run multiconfig.rb with -r 1 i.e. just one replication as this will
#  submit a job that uses ConfTest.rb to run the actual number of replications required
#
# =REVISION HISTORY
#  INI 2007-07-15 Changes
#  

runlimit = 100
confIntVariable = "client1,rtpl3Handover of client1"
if (/hn\d/.match(`hostname`))
  ruby = "~/bin/ruby"
else
  ruby = "ruby"
end

lines = 0
File.open("jobs.txt","r"){|f| lines = f.to_a }

if false
wait = `cp -p /usr/lib/libxml2.so.2 ~/src/other/IPv6SuiteWithINET/lib`
wait = `cp -p /usr/lib/libboost_signals.so.1.33.1 ~/src/other/IPv6SuiteWithINET/lib/libboost_signals.so.2`
end

submitfile = "job.sh"
li=0
lines.size.times do
  l = lines[li]
  #sample line l
  #./wcmc -f wcmc_n_n_n_n_22.ini -r 2
  logfile = l.split(' ')[2].split('.')[0]
  logfile = "~/simlogs/" + logfile + ".log"

  subline = %|#{ruby} #{File.dirname(__FILE__)}/ConfTest.rb -a -g "#{confIntVariable}" -r #{runlimit} "#{lines[li].chomp}"|
  File.open(submitfile, "w"){|f|
    f.puts <<END
#!/bin/bash
# Generated via #{__FILE__}
. ~/bash/defaults
. ~/bash/grid
echo $LD_LIBRARY_PATH
echo $PATH
#{subline}
END
  }
  li += 1
  if /hn\d/.match(`hostname`)
    qline = "qsub -q dque -cwd -o #{logfile} -S /bin/bash -j y #{submitfile}"
    puts qline
    suboutput = `#{qline}`
    if $? != 0
      puts "failed to submit job error was #{suboutput}"
      exit
    end
  else
    puts subline
  end
#  puts "START new scenario at " + Time.now.to_s
#  puts subline
#  `#{subline}`
end
