#! /usr/bin/env ruby
#
#  Author: Johnny Lai
#  Copyright (c) 2006 Johnny Lai
#
# =DESCRIPTION
# TODO: Add your description
#
# =REVISION HISTORY
#  INI 2007-05-11
#

#Put a directory into load path with other custom ruby libs
#Put current dir in load path in case this module requires other files
$LOAD_PATH<<File.dirname(__FILE__)
$LOAD_PATH<<File.dirname(File.dirname(__FILE__))+"/scripts"

require 'optparse'
require 'pp'
require 'matrix'
require 'bigdecimal'
require 'bigdecimal/math'

$test  = false


#
# Calculate the confidence interval (default of 95% level) and repeat the
# experiment until interval satisfies the required precision.
#
class ConfTest

  VERSION       = "$Revision$"
  REVISION_DATE = "11 May 2007"
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

  attr_accessor :pattern

  #
  # TODO: Add description here
  #
  def initialize
    @debug    = false
    @verbose  = false
    @quit     = false

    @dir = "."
    #all nodes will be processed unless option passed in to pass subset
    @print = false
    @pattern = nil
    @scalarfile = "omnetpp.sca"

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
      opt.banner = "Usage: ruby #{File.basename __FILE__} [options] {command in quotes}"
      opt.separator ""
      opt.separator "Specific options:"

      opt.on("-x", "parse arguments and show Usage") {|@quit|}

      opt.on("--file scalarfile", "-f", String, "Process the specified scalar file. "\
             "Default is omnetpp.sca"){|@scalarfile|}

      opt.on("--grep pattern \"mn,cn,ha\"", "-g", String, "particular scalar to"\
             " choose from can include nodename, modulename, network name and "\
             "scalar name separated by commas)") { |nodes|
        createRegex(nodes)
      }

      opt.on("--print", "-p", "Print scalar names available for the specified grep pattern"){|@print|}

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

      opt.parse!
    } or  exit(1);

    raise ArgumentError, "No grep pattern specified!!", caller[0] if not @pattern and not $test

    if @quit
      pp self
      (print ARGV.options; exit)
    end

  end

  # }}}

  def createRegex(greppattern)
    #Create a regex that requires all nodes to be present before matching
    @pattern = greppattern.split(",")
    @pattern.map!{|p| "(#{p})+" }
    pattern = Regexp.new(@pattern.join(".*"))
  end

  def parseScalarFile(filename)
    n = Array.new
    u = Array.new
    s = Array.new
    File.open(filename,"r"){|f|
      f.each_line{|l|
        if @pattern.match(l)
            case l
            when /stddev/
              s<<l.split(/[ ]+|\t+/).last.to_f
            when /mean/
              u<<l.split(/[ ]+|\t+/).last.to_f
            when /samples/
              n<<l.split(/[ ]+|\t+/).last.to_i
            end
        end
      }
    }
    [n, u, s]
  end

  def calculateSem(n, u, s)
    #convert s from sample standard to the one with N as denominator
    v = s.collect{|std| BigDecimal.new(std.to_s)**2}
    nfactor = n.collect{|denom| (denom-1)/denom}
    n.each_index{|i| s[i] = Math.sqrt(nfactor[i]*v[i])}

    ntot = n.inject(0){|sum,x| sum + x}
    a = Matrix.row_vector(n)
    b = Matrix.column_vector(u)


    utot = ((a*b)[0,0])/ntot

    v = s.collect{|std| BigDecimal.new(std.to_s)**2}
    u2 = u.collect{|mean| BigDecimal.new(mean.to_s)**2}

    vtot = (a*(Matrix.column_vector(v)+Matrix.column_vector(u2)))[0,0]/ntot-utot**2
    sem = Math.sqrt(vtot/ntot)
  end

  def confIntWidth(sem)
    #p is taken as 0.95
    #ciw <- qnorm((1+p)/2)* sem
    1.96 * sem
  end

  #
  # Launches the application
  #  ConfTest.new.run
  #
  def run
    @nodes = Regexp.new(@nodes.join("|")) if @nodes.class != Regexp
    1.upto(runLimit){|rc|
      puts "#{rc.to_s} running'`#{ARGV[0]}`'" if @debug

      n, u, s = parseScalarFile(@scalarfile)
      pp n, u, s if @debug
      sem = calculateSem(n, u, s)
      ciw = confIntWidth(sem)
      puts "ciw is " + ciw.to_s if @debug
      if ciw <= precision + 0.00001
        puts "ciw is " + ciw.to_s
        exit 0
      end
    }
    exit(-1)

    rescue => err
      puts err
      puts err.backtrace
      puts "file processed at the time was #{file}" if not file.nil?

  end#run

end#class ConfTest

if $0 == __FILE__ then
  $app = ConfTest.new
  if not $test
    $app.run
    exit
  end

end

# {{{ Unit Test #

##Unit test for this class/module
require 'test/unit'

class TC_ConfTest < Test::Unit::TestCase
  def test_calculateSem
    set1 = [1,2,3,4,5,6,7,8,9,10,6]
    set2 = [40, 23.6, 39.6, 50.2, 23.8]
    set3 = [1, 3, 5, 7, 20, 18]
    set4 = [5, 3, 2000]
    n = [11, 5, 6, 3]
    u = [mean(set1), mean(set2), mean(set3), mean(set4)]
    s = [sigma(set1), sigma(set2), sigma(set3), sigma(set4)]
    totalArray = set1 + set2 + set3 + set4
    ntot = n.inject{|sum, x| sum + x}
    #assert_equal(sigma(totalArray)/Math.sqrt(ntot), @cf.calculateSem(n, u, s),
   #   "my mathematical sem based on groups should equal the total one")

    assert(sigma(totalArray)/Math.sqrt(ntot) - @cf.calculateSem(n, u, s) < 0.00000001, "difference between two results is due to rounding but very small as I use BigDecimals")
    #assert_equal("\"quotedString\"",
    #             quoteString("quotedString"),
    #             "The quotes should surround the quotedString")
  end

  def test_parseScalarFile
    @cf.pattern = "mn|rtp"
    @cf.parseScalarFile("omnetpp.sca")
  end

  def mean(x)
    sum=BigDecimal.new("0")
    x.each { |v| sum += v; }
    sum/x.size
  end
  def variance(x)
    m = mean(x)
    sum = 0.0
    x.each { |v| sum += (v-m)**2 }
    sum/(x.size)
  end

  def sigma(x)
    Math.sqrt(variance(x))
  end

  def setup
    @cf = ConfTest.new
  end

  def teardown

  end

end #end class TC_ConfTest

if $test
  ##Fix Ruby debugger to allow debugging of test code
  require 'test/unit/ui/console/testrunner'
  Test::Unit::UI::Console::TestRunner.run(TC_ConfTest)
end

# }}}
