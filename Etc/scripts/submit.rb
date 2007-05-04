#! /usr/bin/env ruby
#
#  Author: Johnny Lai
#  Copyright (c) 2006 Johnny Lai
#
# =DESCRIPTION
#  Submit jobs to Sun grid engine
#  Assumes jobs are listed in a file named jobs.txt one process per line
#
# =REVISION HISTORY
#  INI 2006-09-21 Changes
#

#require 'fileutils'
#FileUtils.mkpath("agg")

if ARGV.size < 1
  puts "missing argument number of replications to run (-r parameter in multiconfig.rb)"
  exit
end
runCount = ARGV[0].to_i

f=File.open("jobs.txt","r")
lines = f.to_a
f.close

submitfile = "job.sh"
li=0
(lines.size/runCount).times do

  #File.open(submitfile + li.to_s, "w"){|f|
  File.open(submitfile, "w"){|f|
    f.puts <<END
#!/bin/bash
# Generated via #{__FILE__}
END

    l = lines[li]
    #sample line l
    #./wcmc -f wcmc_n_n_n_n_22.ini -r 2
    logfile = l.split(' ')[2].split('.')[0]
    logfile = "~/simlogs/" + logfile + ".log"

    runCount.times do
      f.puts "#{lines[li]}"
      li = li+1
    end

    `qsub -q netsimque -cwd -o #{logfile} -S /bin/bash -j y #{submitfile}`
    puts "qsub -q netsimque -cwd -o #{logfile} -S /bin/bash -j y #{submitfile}"
  }

end
