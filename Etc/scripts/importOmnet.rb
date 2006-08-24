#! /usr/bin/env ruby
#
#  Author: Johnny Lai
#  Copyright (c) 2006 Johnny Lai
#
# =DESCRIPTION
# TODO: Add your description
#
# =REVISION HISTORY
#  INI 2006-08-24 Changes
#

#Put a directory into load path with other custom ruby libs
$LOAD_PATH<<`echo $HOME`.chomp + "/lib/ruby"
#Put current dir in load path in case this module requires other files
$LOAD_PATH<<File.dirname(__FILE__)

require 'optparse'
require 'pp'
require 'RImportOmnet'

$test  = false

module General
  #Splice (interleave) 2 arrays together in order passed in
  def splice(a, b)
    resultArray = Array.new(a.size + b.size)
    factor = 2 # as 2 arrays a and b
    0.upto(resultArray.size - 1) { |i|
      if i%factor == 0
        resultArray[i] = a[i/factor]
      else
        resultArray[i] = b[i/factor]
      end
    }
    resultArray
  end

  def removeLastComponentFrom(string, sep=/\./)
    string.reverse.split(sep, 2)[1].reverse
  end

end
#
# TODO: Add Description Here
#
class ImportOmnet < RImportOmnet

  VERSION       = "$Revision$"
  REVISION_DATE = "24 Aug 2006"
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
    super
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

  # {{{ option processing
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

  end
  # }}}

  DELIM = "_"

  def readConfigs

    @factors = ["scheme", "dnet", "dmap", "ar"]

    @levels = {}
    @levels[@factors[0]] = ["hmip", "mip", "eh"]
    @levels[@factors[1]] = ["50", "100", "200", "500"]
    @levels[@factors[2]] = ["2", "20", "50"]
    @levels[@factors[3]] = ["y", "n"]

  end

  def encodedSingleRun(p, vecFile, encodedFactors, run = 0)
    def expandFactors(encodedFactors)
      c = splice(@factors, encodedFactors)
      e = []
      Hash[*c].to_a.each{|i|
        e.push(i.join(%|"="|))
      } 
      %|"#{e.join(",")}"|
    end

    vectors, separated = retrieveVectors(vecFile)
    @vectorStarted = Array.new if not defined? @vectorStarted
    #not caching the vectors' safe column names. Too much hassle and makes code
    #look complex. Loaded vectors can differ a lot anyway.

    columnNames = safeColumnNames(p, vectors)
    p a if @debug
    i = 0 

    vectors.each_pair { |k,v|
      # does not update value in hash only iterator v
      #v = a[i] 
      vectors[k] = columnNames[i]
      i+=1
      raise "different sized containers when assigning safe column names" if vectors.keys.size != a.size
    }

    @filter = nil
    unless self.filter.nil?
      newIndices = vectors.keys & self.filter
      vectors.delete_if{|k,v|
        not newIndices.include? k
      }
    end

    vectors.each_pair { |k,v|
      raise "logical error in determining column name of vector #{k}" if vectors[k].nil?
      nlines = !@restrict.nil? && @restrict.include?(k)?@rsize:0
      #We'd like to have runnumber.vectorname.vectornumber i.e. #{run}.#{v} but as
      #runnumber at the start appears to be a float it is an R syntax error
      onerunframe = %{#{v}}

      #Remove vector index from column names
      columnname = v.sub(%r{[.]\d+$},"")
      ncolumnname = columnname
      #remove nodename from vector name
      ncolumnname = removeLastComponentFrom(columnname) 

      columnname = separated[k][0]
      nodename = separated[k][1]
      restrictGrep = nlines > 0? "-m #{nlines}":""
      p.puts %{tempscan <- scan(p<-pipe('grep #{restrictGrep} "^#{k}\\\\>" #{vecFile}'), list(trashIndex=0,time=0,#{columnname}=0), nlines = #{nlines})}
      p.puts %{close(p)}
      p.puts %{attach(tempscan)}

      p.puts %{#{onerunframe} <- data.frame( #{expandFactors(encodedFactors)} , run=#{run}, time=time, #{columnname}=#{columnname})}
      

      p.puts %{detach(tempscan)}

      #Aggregate runs for same node's vector
      aggframe = %{#{@aggprefix}#{v}} 

      if not @vectorStarted.include? k
        @vectorStarted.push(k)
        p.puts %{#{aggframe} <- #{onerunframe}}
      else
        p.puts %{#{aggframe} <- rbind(#{aggframe} , #{onerunframe})}        
        #        p.puts %{#{aggframe} <- merge(#{aggframe} , #{onerunframe})}

      end
      p.puts %{rm(#{onerunframe})}

      waitForR p
    }
    p.puts %{rm(tempscan, p)}
  end

  #
  # Launches the application
  #  ImportOmnet.new.run
  #
  def run

    p = IO.popen(RSlave, "w+")

    modname = "EHComp"
    #Will use read config for factor names to be used in data frame columns
    readConfigs
#    factor(x, @{levels[@factors[i]]}, labels=@{levels[@factors[i]].map{|x| x + "ms"}}, ordered = TRUE)
    while ARGV.size > 0 do
      vecFile = ARGV.shift
      raise "wrong file type as it does not end in .vec" if File.extname(vecFile) != ".vec"
      vecFile = %|EHComp_eh_200_50_n_2.vec|
      encodedFactors = File.basename(vecFile, ".vec").split(DELIM)
      run = encodedFactors.last
      encodedFactors = encodedFactors[1..encodedFactors.size-2]
      
      encodedSingleRun(p, vecFile, encodedFactors, run)
    end

  end#run
  
end#class ImportOmnet

$app = ImportOmnet.new
$app.run if not $test

exit unless $test



##Unit test for this class/module
require 'test/unit'

class TC_ImportOmnet < Test::Unit::TestCase
  def test_quoteString
    #assert_equal("\"quotedString\"",
    #             quoteString("quotedString"),
    #             "The quotes should surround the quotedString")
  end

  def setup

  end
  
  def teardown

  end

end

if $0 != __FILE__ then
  ##Fix Ruby debugger to allow debugging of test code
  require 'test/unit/ui/console/testrunner'
  Test::Unit::UI::Console::TestRunner.run(TC_ImportOmnet)
end #end class TC_ImportOmnet

