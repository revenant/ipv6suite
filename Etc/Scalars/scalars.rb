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
# Assumes all files specified by -f pattern are results from
# yaml. If default pattern of *.sca are from different yaml files please change!
#

class Scalars
  include General
  include RStuff

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

    @dir = "."
    #all nodes will be processed unless option passed in to pass subset
    @nodes = [ "*" ]
    @scalars = [ "*" ]
    @module = "*"
    @print = false
    @pattern = "*.sca"
    @config = nil
    @csvOutput = true

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
      opt.banner = "Usage: ruby #{File.basename __FILE__} [options] basename"
      opt.separator ""
      opt.separator "Specific options:"

      opt.on("-x", "parse arguments and show Usage") {|@quit|}

      opt.on("--verbose", "-v", "print intermediate steps to STDERR"){|@verbose|}

      opt.on("--chdir dir", "-c", "Process the *.sca files in dir"){|@dir|}

      opt.on("--file pattern", "-f", "Process files conforming to {pattern} files. Default is *.sca"){|@pattern|}

      opt.on("--nodes \"mn,cn,ha\"", "-n", String,  "nodes to output scalars for separated by commas. default is *"){|nodes|
        @nodes = nodes.split(",")
      }

      opt.on("--modulename mod", "-m", String, "module name to query for values"){|@module|}

      opt.on("--nocsv", "do not output csv files (#{not @csvOutput})"){|@csvOutput| @csvOutput = false}

      opt.on("--scalarname \"*%*,rtpOctetCount\"", "-s", String, "scalar name/s to query separated by commas. One output csv per scalar"){|scalars|
        @scalars = scalars.split(",")
      }

      opt.on("-C", "--config configfile", String, "Use the specified yaml file for configurations to generate"){|@config| @config = File.expand_path(@config) }

      opt.on("--print", "-p", "Print scalar names available for the specified set of modulename and node"){|@print|}

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

      opt.on_tail(%|e.g. -c "~/other/IPv6SuiteWithINET/Research/Networks/EHComp" -n "cn,mn" -m "udpApp[*]" -s "*%*" EHComp|,
                  "Note that default of *.sca may be incorrect in your case!")
      #Samples end

      opt.parse!
    } or  exit(1);

    raise ArgumentError, "No config file specified!!", caller[0] if not @config and not $test

    if @quit
      pp self
      (print ARGV.options; exit)
    end

  end

  # }}}

  def readConfigs
    require 'multiconfig'
    mcg = MultiConfigGenerator.new
    factors, levels = mcg.readConfigs(@config)
    configNames = mcg.generateConfigNames(factors, levels)
    [configNames, factors, levels]
  end

  def aggregateScalars(basename)
    require 'fileutils'
    configs = readConfigs
    puts "Trying to concatenate all runs belonging to a config into one scalar file inside agg dir"
    `rm -fr agg` if not Dir["agg"].empty?
    for c in configs[0]
      FileUtils.mkpath("agg")
      `cat #{basename}#{DELIM}#{c}*.sca > agg/#{basename}#{DELIM}#{c}.sca`
    end
    configs
  end

  #
  # Launches the application
  #  Scalars.new.run
  #
  def run
    basename = ARGV[0]
    @dir = File.expand_path(@dir)
    sm=Datasorter::ScalarManager.new
    ds=Datasorter::DataSorter.new(sm)

    files = Dir["#{@dir}/#{@pattern}"]
if false
    if files.size > 1020 #limited by __FD_SETSIZE (1024)
      STDERR.puts "Sorry cannot process #{files.size} files at one time as limited by __FD_SETSIZE of 1024"
  #    configNames, factors, levels = aggregateScalars(basename)
      configNames, factors, levels = readConfigs
      files = Dir["agg/#{basename}*.sca"]
    end
end
    configNames, factors, levels = readConfigs

    file = nil

    files.each_with_index{|file, index|
      if @verbose
        puts "Loading scalar ##{index}" + file
      else
        print %|#{"\b"*80}#{index+1}/#{files.size}|
        $defout.flush
      end
      sm.loadFile(file)
    }

    file = nil

    if @verbose
      puts "All scalar names are "
      p sm.scalars.keys
      puts ""
      #puts "module names are "
      #p sm.modules.keys
      puts ""
    end

    #require 'General'
    #p = IO.popen(RInterface::RSlave, "w+")

    #Collect scalarNames so we can output correct header and csv files

    #scalarName May not be unique across modules should check what happens if
    #not (but c++ interface is a set so if not unique some values will get
    #erased?)
    scalarNames = Set.new
    moduleNames = Set.new
    loop = 0
    for config in configNames
      for node in @nodes
        for scalar in @scalars
          puts "node in #{config} is #{node}:#{@module}:#{scalar}" if @debug
          ints = ds.getFilteredScalarList("*#{RInterface::DELIM}#{config}*", "*.#{node}.#{@module}", scalar)
          ints.each{|i|
            v = sm.getValue(i)
            scalarNames.add v.scalar
            moduleNames.add v.module
          }
        end
      end
      break if @print
    end

    if @print
      puts "\nscalar names after filtering are :"
      puts scalarNames.to_a.join("\n")
      puts "module names after filtering are :"
      puts moduleNames.to_a.join("\n")
      exit
    end

if @csvOutput
    for scalarName in scalarNames
      header = false
      outputfile = scalarName +".csv"
      File.open(outputfile, "w"){|f|
        for config in configNames
          for node in @nodes
            ints = ds.getFilteredScalarList("*#{RInterface::DELIM}#{config}*", "*.#{node}.#{@module}", scalarName)
            next if ints.size == 0
            encodedFactors = config.split(RInterface::DELIM)
            ints.each{|i|
              datum = sm.getValue(i)
              run = datum.run.runNumber
              nodename = datum.module.split(".")[1]
              csvdelim = ","
              if (not header)
                puts "outputting csv file #{f.path}"
                f.puts factors.join(csvdelim) + csvdelim + %|node| + csvdelim + "run" + csvdelim + scalarName
                header = true
              end
              f.puts encodedFactors.join(csvdelim) + csvdelim + nodename + csvdelim + run.to_s + csvdelim + datum.value.to_s
              #p.puts %{ a = data.frame(#{expandFactors(encodedFactors, factors)}, node=#{quoteValue(nodename)}, run=#{run}, #{scalarName} = #{datum.value}}
            }
          end
        end
      }
      if File.size(outputfile) == 0
        cout = IO.open(1, "w")
        endl = "\n"
        cout << "Deleting "<< outputfile<< endl
        File.delete(outputfile)
      end
    end
end

  #Reduce no. of csvs by factor of 5 works only for cStdDev output
  combinedScalars = combineCSVs(Dir["*.mean.csv"])
  #Grab all csvs in current directory for analysis
  combinedScalars = Dir["*.csv"].collect{|e| e.split(".csv")[0]}  
  analyseWithR(combinedScalars, factors)

    rescue => err
      puts err
      puts err.backtrace
      puts "file processed at the time was #{file}" if not file.nil?

  end#run

  #Expects an array of csv filenames to work on
  def combineCSVs(array = Dir["*.mean.csv"])
    array = array.select{|i| 
      i =~ /[.]mean[.]csv/
    }
    array.collect!{|e|
      e.split(".mean.csv")[0]
    }
    stats = %|samples,mean,stddev,max,min|.split(",")
    array.each{|e|
      for s in stats do
        wait = `cut -d ',' -f 5 < "#{e}.#{s}.csv" > #{s}.tmp` 
      end   
      wait = `cut -d ',' -f 1-4 "#{e}.mean.csv" | paste -d "," - #{stats.join(".tmp ") + ".tmp"} > "#{e}.csv"`
      wait = `rm #{stats.join(".tmp ") + ".tmp"}`
      if not @debug
        stats.each{|i| `rm "#{e}.#{i}.csv"` } 
      end
    }
    array
  end

  #Process and Store all cscalars[x].csv data into Rdata file
  def analyseWithR(cscalars, factors)
    configNames, factors, levels = readConfigs if factors.nil?
    rdataFile = "scalars.Rdata"
    loadRfile = %||
    saveRfile = %|save.image("#{rdataFile}")|
    sourceFunctions = %|source("#{File.dirname(__FILE__)}/../scripts/functions.R")|
    cscalars.each{|variable|
      loadRfile = %|load("#{rdataFile}");| if File.exist?(rdataFile)
      v = variable.gsub(/[ ]/,'.') #make it look like R valid name
      v = v.gsub(/[%]/,'.') #make it look like R valid name
      expanded = ""
      factors.map{|e| expanded += "#{e}=#{e},"}      
      expanded.chop! #remove last ,
      run = `echo 'a=read.csv("#{variable}.csv");with(a, aggregate(run, list(#{expanded}),length))'| #{RSlave}`
      #means & samples do not exist for simple scalars written at sim end and are not cStdDev type scalar
      samples = `echo 'a=read.csv("#{variable}.csv");with(a, aggregate(#{v}.samples, list(#{expanded}),sum))'| #{RSlave} 2>&1`
      puts "scalar is #{variable}"
      puts "runs are ", run
      if samples !~ /Error/
        mean = `echo 'a=read.csv("#{variable}.csv");with(a, aggregate(#{v}.mean, list(#{expanded}),mean))'| #{RSlave} 2>&1`
        puts "samples are ", samples
        puts "mean is ", mean
        wait = `echo '#{loadRfile} a=read.csv("#{variable}.csv");#{sourceFunctions};a=jl.renameColumn(a,5,"samples");a=jl.renameColumn(a,6,"mean");a=jl.renameColumn(a,7,"stddev");#{v} <- a;rm(a);#{saveRfile}'|#{RSlave}`
        loadRfile = %|load("#{rdataFile}");| if File.exist?(rdataFile)
        puts executeInR(%|#{loadRfile} #{sourceFunctions};with(#{v}, jl.tapply(#{v},list(#{expanded})))|)
      else
        puts "mean of #{variable}", executeInR(%|#{loadRfile} a=read.csv("#{variable}.csv");with(a, aggregate(#{v}, list(#{expanded}),mean));#{v} <- a;rm(a);#{saveRfile}|)                                      
      end
    }
  end

  def notesInR
<<END
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
END
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
  include General
  def test_scalars
    scripttest = "test_scalars.sh"
    testscalar = "wcmc_y_3.sca"
    File.open(scripttest, "w"){|f|
      script = <<END
cd #{TESTDIR}
bunzip2 -k #{testscalar}.bz2
ruby #{__FILE__} -m "udpApp[*]" -C test_multiconfig_script.yaml -p -f #{testscalar}
END
      f.puts script
    }
    output = `sh #{scripttest}`
    result = <<END
\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b1/1
scalar names after filtering are :
rtp % dropped from client1
rtpTransitTime of client1.max
rtpTransitTime of server4[0].max
rtpTransitTime of server4[0].mean
rtpl3Handover of client1.min
rtpHandover of client1.max
rtpl2Handover of client1.min
rtpTransitTime of server4[0].stddev
rtpTransitTime of client1.min
rtp received from server4[0]
rtp % dropped from server4[0]
rtpl3Handover of client1.samples
rtpTransitTime of server4[0].min
rtpl3Handover of client1.stddev
rtpl2Handover of client1.samples
rtpHandover of client1.min
rtp dropped from client1
rtpl2Handover of client1.mean
rtpTransitTime of server4[0].samples
rtp received from client1
rtpOctetCount
rtpl3Handover of client1.mean
rtpHandover of client1.mean
rtpHandover of client1.samples
rtpTransitTime of client1.mean
rtpl2Handover of client1.stddev
rtp dropped from server4[0]
rtpl3Handover of client1.max
rtpHandover of client1.stddev
rtpTransitTime of client1.stddev
rtpTransitTime of client1.samples
rtpl2Handover of client1.max
module names after filtering are :
wcmc_y_3Net.server4.udpApp[0]
wcmc_y_3Net.client1.udpApp[0]
END
    #result unusable as contains extra formatting somewhere
    assert_equal(result, output, "scalars failed to match")
    assert($? == 0, "scalars failed with #{output}")
    assert(output.split("\n").length == (389-352), 
           "wrong test output line count #{output.split("\n").length}, \n#{output}")

    File.open(scripttest, "w"){|f|
    script = <<END
ruby #{__FILE__} -m "udpApp[*]" -C test_multiconfig_script.yaml -p -n client1
END
    f.puts script
    }
    output = `sh #{scripttest}`
    assert(output.split("\n").length == 28, "should be less than previous output #{output}")
    ensure
      File.delete(testscalar)
      File.delete(scripttest)
  end

  def setup
    Dir.chdir(File.expand_path(TESTDIR))
  end

  def teardown
  end

end #end class TC_Scalars

if $test
  ##Fix Ruby debugger to allow debugging of test code
  require 'test/unit/ui/console/testrunner'
  result = Test::Unit::UI::Console::TestRunner.run(TC_Scalars)
  if not result.passed?
    exit(result.failure_count) if result.failure_count > 0
    exit(result.error_count)
  end
end

# }}}
