#! /usr/bin/env ruby
#
#  Author: Johnny Lai
#  Copyright (c) 2006, 2008 Johnny Lai
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

require 'optparse'
require 'pp'

$test  = false

#
# submit job (1 per line) in jobs file to the Sun Grid engine via qsub.  Each job
# is run within ConfTest.rb until either @runLimit is reached or precision of
# @confIntVariable is attained.
#
class SmartSubmit
  VERSION       = "$Revision$"
  REVISION_DATE = "7 Jan 2008"
  AUTHOR        = "Johnny Lai"
 
  #
  # Returns a version string similar to:
  #  <app_name>:  Version: 1.2 Created on: 2002/05/08 by Jim Freeze
  # The version number is maintained by CVS. 
  # The date is the last checkin date of this file.
  # 
  def version
    "Version: #{VERSION.split[1]} Created on: " +
      "#{REVISION_DATE.split[1]} by #{AUTHOR}"
  end

  def initialize
    @debug    = false 
    @verbose  = false
    @quit     = false

    @confIntVariable = "client1,rtpl3Handover of client1"
    @runLimit = 100
    @precision = 0.05

    get_options

  rescue => err
    STDERR.puts usage
    STDERR.puts "\n"
    STDERR.puts err
    STDERR.puts err.backtrace if err
    exit 1
  end

  #
  # Returns usage string
  #
  def usage
    ARGV.options
  end

  #
  # Processes command line arguments
  #
  def get_options
    ARGV.options { |opt|
      opt.banner = version
      opt.banner = "Usage: ruby #{File.basename __FILE__} [options] jobsfile"
      opt.separator ""
      opt.separator "Specific options:"

      opt.on("--civar nameofvariable", "-c", String, "Variable to determine precision for"){ |@confIntVariable|   
      }

      opt.on("-p", "--precision f", Float, "Precision of variable to stop sim at specified in --civar") {|@precision|
      }

      opt.on("-r", "--runlimit x", Integer, "Runlimit for ConfTest.rb") {|@runLimit|
      }

      opt.separator ""
      opt.separator "Common options:"

      opt.on("--doc=DIRECTORY", String, "Output rdoc (Ruby HTML documentation) into directory"){|dir|
        system("rdoc -a --webcvs=\"http://localhost/cgi-bin/viewcvs.cgi/\" --tab-width=2 --diagram --inline-source -N -o #{dir} #{__FILE__}")
      }

      opt.on("-x", "parse arguments and show Usage") {|@quit|}

      opt.on("--verbose", "-v", "print intermediate steps to STDOUT"){|@verbose|}

      opt.on("--debug", "-X", "print debugging info to STDOUT"){|@debug|}

      opt.on("--unittest", "run unit tests"){|$test|}

      opt.on_tail("-h", "--help", "What you see right now") do
        puts opt
        exit
      end

      opt.on_tail("--version", "Show version") do
        #puts OptionParser::Version.join('.')
        puts version
        exit
      end

      opt.on_tail("")

      opt.parse!
    } or  exit(1);

    raise ArgumentError, "No jobs file specified!", caller[0] if not ARGV.size > 0 and not $test

    if @quit
      pp self
      (print ARGV.options; exit) 
    end

  end

  #
  # Launches the application
  #  SmartSubmit.new.run
  #
  def run
    if false
      wait = `cp -p /usr/lib/libxml2.so.2 ~/src/other/IPv6SuiteWithINET/lib`
      wait = `cp -p /usr/lib/libboost_signals.so.1.33.1 ~/src/other/IPv6SuiteWithINET/lib/libboost_signals.so.2`
    end

    if (/hn\d/.match(`hostname`))
      ruby = "~/bin/ruby"
    else
      ruby = "ruby"
    end

    lines = 0
    File.open(ARGV[0], "r"){|f| lines = f.to_a }

    submitfile = "job.sh"

    lines.size.times {|li|
      l = lines[li]
      next if /^\s+$/ =~ l

      #sample line l
      #./wcmc -f wcmc_n_n_n_n_22.ini -r2
      logfile = l.split(' ')[2].split('.')[0]
      logfile = "~/simlogs/" + logfile + ".log"
      startrun = l.split(' ')[3].split('r')[1]
      if startrun.to_i != 1
        subline = %|#{ruby} #{File.dirname(__FILE__)}/ConfTest.rb -s #{startrun} -a -g "#{@confIntVariable}" -r #{@runLimit} -p #{@precision} "#{lines[li].chomp}"|
      else        
        subline = %|#{ruby} #{File.dirname(__FILE__)}/ConfTest.rb -a -g "#{@confIntVariable}" -r #{@runLimit} -p #{@precision} "#{lines[li].chomp}"|
      end

      File.open(submitfile, "w"){|f|
        f.puts <<END
#!/bin/bash
# Generated via #{__FILE__}
. /etc/profile.d/modules.sh
module load R
echo $LD_LIBRARY_PATH
echo $PATH
#{subline}
END
      }

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

    } #lines.size.times

  end#run
  
end#SmartSubmit

if $0 == __FILE__ then
  $app = SmartSubmit.new
  if not $test
    $app.run
    exit
  end

end

# {{{ Unit Test #

##Unit test for this class/module
require 'test/unit'

class TC_SmartSubmit < Test::Unit::TestCase
  def test_quoteString
    #assert_equal("\"quotedString\"",
    #             quoteString("quotedString"),
    #             "The quotes should surround the quotedString")
  end
end

if $0 != __FILE__ then
  ##Fix Ruby debugger to allow debugging of test code
  require 'test/unit/ui/console/testrunner'
  Test::Unit::UI::Console::TestRunner.run(TC_SmartSubmit)
end

# }}}
