#! /usr/bin/env ruby
#
#  Author: Johnny Lai
#  Copyright (c) 2006 Johnny Lai
#
# =DESCRIPTION
#  Submit jobs to Sun grid engine 
#
# =REVISION HISTORY
#  INI 2006-09-21 Changes
#

require 'fileutils'

submitfile = "job.sh"
FileUtils.mkpath("agg")
ARGF.each_line{|l|
File.open(submitfile, "w"){|f|
f.puts <<END
#!/bin/bash
# Generated via #{__FILE__}
#{l}
END
}
#sample line l
#./wcmc -f wcmc_n_n_n_n_22.ini -r 2
logfile = l.split(' ')[2].split('.')[0]
`qsub -q netsimque -cwd -o #{logfile} -S /bin/bash -j y #{submitfile}`
}
