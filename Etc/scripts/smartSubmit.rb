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

if (/hn1/.match(`hostname`))
  ruby = "~/bin/ruby"
else
  ruby = "ruby"
end

f=File.open("jobs.txt","r")
lines = f.to_a
f.close

submitfile = "job.sh"
li=0
lines.size.times do
  l = lines[li]
  #sample line l
  #./wcmc -f wcmc_n_n_n_n_22.ini -r 2
  logfile = l.split(' ')[2].split('.')[0]
  logfile = "~/simlogs/" + logfile + ".log"

  subline = %|#{ruby} #{File.dirname(__FILE__)}/ConfTest.rb -a -g "client1,rtpl3Handover of client1" -r #{runlimit} "#{lines[li].chomp}"|
  File.open(submitfile, "w"){|f|
    f.puts <<END
#!/bin/bash
# Generated via #{__FILE__}
#{subline}
END
  }
  li += 1
if false
  if /hn1/.match(`hostname`)
    `qsub -q netsimque -cwd -o #{logfile} -S /bin/bash -j y #{submitfile}` 
    puts "`qsub -q netsimque -cwd -o #{logfile} -S /bin/bash -j y #{submitfile}`"
  else
    puts subline
  end
end
  puts "START new scenario at " + Time.now
  puts subline
  `#{subline}`
end