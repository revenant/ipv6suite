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
`qsub -q netsimque -cwd -o log -S /bin/bash -j y #{submitfile}`
}
