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

    @dir = "."
    #all nodes will be processed unless option passed in to pass subset
    @nodes = [ "*" ] 
    @scalars = [ "*" ]
    @module = "*"
    @print = false

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

      opt.on("-chdir dir", "-c", "Process the *.sca files in dir"){|@dir|}

      opt.on("--nodes \"mn,cn,ha\"", "-n", String,  "nodes to output scalars for separated by commas. default is *"){|nodes|
        @nodes = nodes.split(",")
      }

      opt.on("--modulename mod", "-m", String, "module name to query for values"){|@module|
      }

      opt.on("--scalarname \"*%*,rtpOctetCount\"", "-s", String, "scalar name/s to query separated by commas. One output csv per scalar"){|scalars|        
        @scalars = scalars.split(",")
      }

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

      opt.on_tail(%|-n "cn,mn" -m "udpApp[*]" -s "*%*" EHComp|,
                  "")
      #Samples end

      opt.parse!
    } or  exit(1);

    if @quit
      pp self
      (print ARGV.options; exit) 
    end

  end

  # }}}

  #
  # Launches the application
  #  Scalars.new.run
  #
  def run
    path = File.expand_path("~/other/IPv6SuiteWithINET/Research/Networks/EHComp")
    @dir = path
    sm=Datasorter::ScalarManager.new
    ds=Datasorter::DataSorter.new(sm)

    Dir["#{@dir}/*.sca"].each{|file|
      puts "Loading scalar file " + file if @verbose
      @file = file
      sm.loadFile(file)
    }

    @file = nil

    if @verbose
      puts "scalar names are "
      p sm.scalars.keys
      puts ""
      #puts "module names are " 
      #p sm.modules.keys
      puts ""
    end

    require 'multiconfig'
    require 'General'
    mcg = MultiConfigGenerator.new
    factors, levels = mcg.readConfigs
    configNames = mcg.generateConfigNames(factors, levels)

    #p = IO.popen(RInterface::RSlave, "w+")

    #Collect scalarNames so we can output correct header and csv files

    #scalarName May not be unique across modules should check what happens if
    #not (but c++ interface is a set so if not unique some values will get
    #erased?)
    scalarNames = Set.new
    moduleNames = Set.new
    for config in configNames
      for node in @nodes
        for scalar in @scalars
          ints = ds.getFilteredScalarList("*#{config}*", "*#{node}.#{@module}", scalar)
          ints.each{|i|
            v = sm.getValue(i)
            scalarNames.add v.scalar
            moduleNames.add v.module
          }
        end
      end
      #Just do one config if print names otherwise gets repetitive
      break if @print
    end

    if @print
      puts "scalar names are "
      puts scalarNames.to_a.join("\n")
      puts "module names are " 
      puts moduleNames.to_a.join("\n")
      exit
    end

    for scalarName in scalarNames 
      header = false
      outputfile = scalarName +".csv"
      File.open(outputfile, "w"){|f|
        for config in configNames
          for node in @nodes
            ints = ds.getFilteredScalarList("*#{config}*", "*#{node}.#{@module}", scalarName)
            next if ints.size == 0
            sum = ints.inject(0){|sum, i| sum + sm.getValue(i).value}
            mean = sum/ints.length
            encodedFactors = config.split(RInterface::DELIM)
            ints.each{|i|
              datum = sm.getValue(i)
              run = datum.run.runNumber
              csvdelim = ","            
              if (not header)
                puts "outputting csv file #{f.path}"
                f.puts factors.join(csvdelim) + csvdelim + %|node| + csvdelim + "run" + csvdelim + scalarName 
                header = true
              end
              f.puts encodedFactors.join(csvdelim) + csvdelim + node + csvdelim + run.to_s + csvdelim + datum.value.to_s
              #p.puts %{ a = data.frame(#{expandFactors(encodedFactors, factors)}, node=#{quoteValue(node)}, run=#{run}, #{scalarName} = #{datum.value}}
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

    rescue => err
      puts err
      puts err.backtrace
      puts "file processed at the time was #{@file}" if not @file.nil?

  end#run

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
