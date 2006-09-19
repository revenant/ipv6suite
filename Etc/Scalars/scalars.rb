#! /usr/bin/env ruby
#
#  Author: Johnny Lai
#  Copyright (c) 2006 Johnny Lai
#
# =DESCRIPTION
# TODO: Add your description
#
# =REVISION HISTORY
#  INI 2006-09-19 Changes
#

#Put a directory into load path with other custom ruby libs
$LOAD_PATH<<`echo $HOME`.chomp + "/lib/ruby"
#Put current dir in load path in case this module requires other files
$LOAD_PATH<<File.dirname(__FILE__)
$LOAD_PATH<<File.dirname(File.dirname(__FILE__))+"/scripts"

require "datasorter"
require "RImportOmnet"

require 'optparse'
require 'pp'

$test  = false

#
# TODO: Add Description Here
#
class Scalars
  include General

  VERSION       = "$Revision$"
  REVISION_DATE = "19 Sep 2006"
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

  #
  # TODO: Add description here
  #
  def initialize
    @debug    = false 
    @verbose  = false
    @quit     = false
    
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
    #
    # TODO: Fill out usage in get_options as part of optparse API
    #
    ARGV.options
#    <<-USAGE
#Usage: #{File.basename $0} [-v] file
#  -v|--verbose      print intermediate steps to STDERR
#    USAGE
  end

  # {{{ process options #

  #
  # Processes command line arguments
  #
  def get_options
    ARGV.options { |opt|
      opt.banner = version
      opt.banner = "Usage: ruby #{File.basename __FILE__} [options] "
      opt.separator ""
      opt.separator "Specific options:"

      #Try testing with this 
      #ruby __FILE__ -x -c -s test
      opt.on("-x", "parse arguments and show Usage") {|@quit|}

      opt.on("--verbose", "-v", "print intermediate steps to STDERR"){|@verbose|}                                                                                                     
      #Sample options
      opt.on("--[no-]combine=[FLAG]", "-c", "load all results from subdirectories into",
             "one big RData file in topdir"){ |@opt_combine|}

      opt.on("--sub=SUBSET", "-s", String, "Plot subset only"){ |@opt_subset|   
        puts "subsetting on #{@opt_subset}"
      }

      # Cast 'time' argument to a Time object.
#      opt.on("-t", "--time [TIME]", Time, "Begin execution at given time") do |time|
#        options.time = time
#      end
      # Cast to octal integer.
      opt.on("-F", "--irs [OCTAL]", OptionParser::OctalInteger,
              "Specify record separator (default \\0)") do |rs|
        options.record_separator = rs
      end
      # List of arguments.
      opt.on("--list x,y,z", Array, "Example 'list' of arguments") do |list|
        options.list = list
      end

      opt.separator ""
      opt.separator "Common options:"

      opt.on("--doc=DIRECTORY", String, "Output rdoc (Ruby HTML documentation) into directory"){|dir|
        system("rdoc -a --webcvs=\"http://localhost/cgi-bin/viewcvs.cgi/\" --tab-width=2 --diagram --inline-source -N -o #{dir} #{__FILE__}")
      }

      opt.on("--verbose", "-v", "print intermediate steps to STDOUT"){|@verbose|}

      opt.on("--debug", "-X", "print debugging info to STDOUT"){|@debug|}

      opt.on("--test", "run unit tests"){|$test|}

      opt.on_tail("-h", "--help", "What you see right now") do
        puts opt
        exit
      end

      opt.on_tail("--version", "Show version") do
        #puts OptionParser::Version.join('.')
        puts version
        exit
      end

      opt.on_tail("By default ...splits data according to opt_filter, ",
                  "Subset plots on ... #{@opt_subset}. Will not create histograms")
      #Samples end

      opt.parse!
    } or  exit(1);

    if @quit
      pp self
      (print ARGV.options; exit) 
    end

    #raise ArgumentError, "No arguments or options specified (Maybe you need to remove this exception?) ", caller[0] if ARGV.size < 1 and not $test

  end

  # }}}

  #
  # Launches the application
  #  Scalars.new.run
  #
  def run
    ## TODO: Add code here

    #variable
    path = File.expand_path("~/tmp/process")
    path = File.expand_path("/home/jmll/other/IPv6SuiteWithINET/Research/Networks/EHComp")
    sm=Datasorter::ScalarManager.new
    ds=Datasorter::DataSorter.new(sm)

#    dirs = ["1pcerror", "noerror"]
#    dirs.each { |dir|
#      Dir["#{path}/#{dir}/*.sca"].each{|file|
      Dir["#{path}/*.sca"].each{|file|
        @file = file
        sm.loadFile(file)        
      }
#    }

    require 'multiconfig'
    require 'General'
    mcg = MultiConfigGenerator.new
    factors, levels = mcg.readConfigs
    configNames = mcg.generateConfigNames(factors, levels)

    p = IO.popen(RInterface::RSlave, "w+")

    header = false
    for config in configNames
      for node in [ "cn", "mn" ]
        #variables are modulename and scalarname
        ints = ds.getFilteredScalarList("*#{config}*", "*#{node}.udpApp[*]", "* % *")
        next if ints.size == 0
        sum = ints.inject(0){|sum, i| sum + sm.getValue(i).value}
        mean = sum/ints.length
#        puts "#{config}: mean is #{mean} of #{ints.length} values for #{node}"
        encodedFactors = config.split(RInterface::DELIM)
        ints.each{|i|
          datum = sm.getValue(i)
          run = datum.run.runNumber

          #May not be unique should check what happens if not (but c++ interface
          #is a set so if not unique some values will get erased?)
          scalarName = datum.scalar
          #scalarName = "loss"

          csvdelim = ","
    
          #Be careful with scalarName because dropped from mn/cn in case does
          #not matter much as implied from node but when really random partner
          #then will matter.
          if (not header)
            #header line
            puts factors.join(csvdelim) + csvdelim + %|node| + csvdelim + "run" + csvdelim + scalarName 
            header = true
          end
          puts encodedFactors.join(csvdelim) + csvdelim + node + csvdelim + run.to_s + csvdelim + datum.value.to_s
#          p.puts %{ a = data.frame(#{expandFactors(encodedFactors, factors)}, node=#{quoteValue(node)}, run=#{run}, #{scalarName} = #{datum.value}}
        }
      end
    end
    rescue => err
      puts err
      puts err.backtrace
      puts "file process at the time was #{@file}"

  end#run

  def notesInR
    #In R
    a = read.table("csv.out",sep=",", header=T)
    b = subset(a, ar=="y")
    length(a[,1])
    length(b[,1])
    boxplot(a$rtp...dropped.from.mn ~ a$scheme)


#Run this for each config (but not run too?)
b = subset(a.rtpDrop.cn, rtpDrop.cn > 9 & run == 1  & encodedFactors.join("&"))
#Number of times config exceed quality spec
sum(length(b[,1]))
#Interrun variation do subset for each run too and then graph histogram of sums
#exceed quality. (If not much spread good) or just calc. std dev
  end
end#class Scalars

if $0 == __FILE__ then
  $app = Scalars.new
  if not $test
    $app.run 
    exit 
  end

end

# {{{ Unit Test #

##Unit test for this class/module
require 'test/unit'

class TC_Scalars < Test::Unit::TestCase
  def test_quoteString
    #assert_equal("\"quotedString\"",
    #             quoteString("quotedString"),
    #             "The quotes should surround the quotedString")
  end

  def setup

  end
  
  def teardown

  end

end #end class TC_Scalars

if $test
  ##Fix Ruby debugger to allow debugging of test code
  require 'test/unit/ui/console/testrunner'
  Test::Unit::UI::Console::TestRunner.run(TC_Scalars)
end 

# }}}

