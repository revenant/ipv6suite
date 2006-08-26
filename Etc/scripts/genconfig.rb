#! /usr/bin/env ruby
#
#  Author: Johnny Lai
#  Copyright (c) 2006 Johnny Lai
#
# =DESCRIPTION
# TODO: Add your description
#
# =REVISION HISTORY
#  INI 2006-07-10 Changes
#

require 'optparse'
require 'pp'

#obtained from http://rubyforge.org/snippet/detail.php?type=snippet&id=2
class Object
    def deep_clone
        Marshal.load(Marshal.dump(self))
    end
end

#
# Generate configs by varying a ned delay in nedfile
#
class GenConfig
  VERSION       = "$Revision$"
  REVISION_DATE = "$Date$"
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

      opt.on("--doc=DIRECTORY", String, "Output rdoc (Ruby HTML documentation) into directory"){|dir|
        system("rdoc -a --webcvs=\"http://localhost/cgi-bin/viewcvs.cgi/\" --tab-width=2 --diagram --inline-source -N -o #{dir} #{__FILE__}")
      }

      opt.on("--verbose", "-v", "print intermediate steps to STDERR"){|@verbose|}                                                                                                     
      #Sample options
      opt.on("--[no-]combine=[FLAG]", "-c", "load all results from subdirectories into",
             "one big RData file in topdir"){ |@opt_combine|}

      opt.on("--sub=SUBSET", "-s", String, "Plot subset only"){ |@opt_subset|   
        puts "subsetting on #{@opt_subset}"
      }

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
        system("rdoc -o #{dir} #{__FILE__}")
      }

      opt.on("--verbose", "-v", "print intermediate steps to STDOUT"){|@verbose|}

      opt.on("--debug", "-X", "print debugging info to STDOUT"){|@debug|}

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

#    raise ArgumentError, "No arguments or options specified (Maybe you need to remove this exception?) ", caller[0] if ARGV.size < 1

  end

  #returns index for matching first line 
  def searchForLine(nedfile, regexp)
    nedfile.each_with_index {|line,index|
      if regexp.match(line) then
        return index
      end
    }
    0
  end

  #
  # Launches the application
  #  GenConfig.new.run
  #
  def run
    modname = "EHAnalysism"
    netname = "ehAnalysisNet"
    exename = "HMIPv6Network"
    cable = "EHAnalysisEdgeCable"
    configs = ["1","10","20", "50", "100"]
    runcount = 10

    configsOrig = configs.deep_clone

  #form names of ini/ned files and also the network name in ned file
    netnames = Array.new(configs.size, netname)
    configs.each_index{|c| 
      netnames[c]+=configs[c]+"ms"
      configs[c].insert(0, modname)
      configs[c] += "ms"
    }

   nedfile = IO.readlines(modname+".ned")
   inifile = IO.readlines(modname+".ini")

   delayLine = searchForLine(nedfile, Regexp.new("^channel"+"\s+"+cable))

pp configs, netnames if @debug   

    File.open("jobs.txt","w"){|jobfile|
    configs.each_with_index {|c,x|
      #write ini files
      File.open(c+".ini", "w"){ |ini|
#        ini.puts "include #{modname}.ini"

        inifile.each_with_index {|line,index|
          ini.puts line.sub(Regexp.new(netname), netnames[x])
          raise "inifile #{modname+".ini"} contains a conflicting network directive at line #{index}" if line =~ /^network/
        }

        ini.puts "[General]"
        ini.puts "network = #{netnames[x]}"


        1.upto(runcount) do |i|
          vectorfile = c+ i.to_s + ".vec"
          #Use same scalarfile for a config
          scalarfile = c + ".sca"
    
          ini.puts "[Run #{i}]"
          ini.puts "output-vector-file = #{vectorfile}"
          ini.puts "output-scalar-file = #{scalarfile}"


          #write distjobs file
          jobfile.puts "./#{exename} -f #{c}.ini -r #{i}"
        end
      }

      #write ned files
      File.open(c+".ned", "w"){|f|
        nedfile.each_with_index {|line, index|
          if index == delayLine + 2            
            f.puts line.sub(/\d+(e-*\d+)*/,configsOrig[x]+"e-3")
            raise "line does not appear to be a delay " if line !~ /delay/
          else
            mod = line.sub(Regexp.new(netname),netnames[x])
            f.puts mod.sub(Regexp.new(modname[0,modname.size-1]),c)
          end 

        }
      }
  }}


  end#run
  
end#GenConfig

app = GenConfig.new.run


##Unit test for this class/module
require 'test/unit'

class TC_GenConfig < Test::Unit::TestCase
  def test_quoteString
    #assert_equal("\"quotedString\"",
    #             quoteString("quotedString"),
    #             "The quotes should surround the quotedString")
  end
end

if $0 != __FILE__ then
  ##Fix Ruby debugger to allow debugging of test code
  require 'test/unit/ui/console/testrunner'
  Test::Unit::UI::Console::TestRunner.run(TC_GenConfig)
end

