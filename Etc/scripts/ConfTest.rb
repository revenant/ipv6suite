#! /usr/bin/env ruby
#
#  Author: Johnny Lai
#  Copyright (c) 2006 Johnny Lai
#
# =DESCRIPTION
# Run as many replications of command as required until precision is reached
# or runLimit reached.
#
# Sample run line
# ruby `pwd`/ConfTest.rb -g "client1,Ping roundtrip delays" --debug "./HMIPv6Netork -f HMIPv6Sait.ini"
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

    @pattern = nil
    @scalarfile = "omnetpp.sca"
    @runlimit = 100
    @precision = 0.00001
    @auto = false

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

      opt.on("--grep pattern \"network,node,module,scalarname\"", "-g", String,
             "particular scalar to check precision on. Scalar name must be "\
             "according to order specified and separated by commas.") { |nodes|
        createRegex(nodes)
      }
      opt.on("-r runlimit", Integer, "run limit to stop at if precision is not reached"){|@runlimit|}
      opt.on("-p precision", Float, "precision you'd like to reach"){|@precision|}
      opt.on("-a", "infer scalar file name and remove run number indications from command to run"){|@auto|}

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

    raise ArgumentError, "No grep pattern specified!!", caller[0] if not @pattern and not $test or not ARGV.size > 0

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
    @pattern = Regexp.new(@pattern.join(".*"))
  end

  def parseScalarFile(filename)
    n = Array.new
    u = Array.new
    s = Array.new
    File.open(filename,"r"){|f|
      f.each_line{|l|
        if pattern.match(l)
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
    nfactor = n.collect{|denom| (BigDecimal.new(denom.to_s)-1)/denom}
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
    cli = String.new(ARGV[0])
    if @auto
        @scalarfile = ARGV[0].split(' ')[2].split('.')[0] + ".sca"
        cli.gsub!(/-r[0-9]+/,'')
    end
    puts "scalar file is " + @scalarfile if @debug
    1.upto(@runlimit){|rc|
      puts "`#{cli + ' -r' + rc.to_s}`"
      `#{cli + " -r"+rc.to_s}`
      n, u, s = parseScalarFile(@scalarfile)
      pp n, u, s if @debug
      sem = calculateSem(n, u, s)
      ciw = confIntWidth(sem)
      puts "ciw is " + ciw.to_s if @verbose
      if ciw <= 0.00001
        puts "final ciw is " + ciw.to_s
        exit 0
      end
    }
    puts "unable to lock onto required precision. ciw is " + ciw.to_s
    exit(-1)

    rescue => err
      puts err
      puts err.backtrace
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

    assert(sigman(totalArray)/Math.sqrt(ntot) - @cf.calculateSem(n, u, s) < 0.00000001,
           "difference between two results is due to rounding but should be very"\
           "small as I use BigDecimals")
  end

  #Requires a scalar file with name of omnetpp.sca (tested from HMIPv6Sait -r1/2)
  def test_parseScalarFile
    @cf.createRegex("client1,Ping roundtrip delays")
    n, u, s = @cf.parseScalarFile("omnetpp.sca")
    pp n, u, s
    pp @cf.calculateSem(n, u, s)
    pp @cf.confIntWidth(@cf.calculateSem(n, u, s))
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
    sum/(x.size-1)
  end

  def sigma(x)
    Math.sqrt(variance(x))
  end

  def sigman(x)
    m = mean(x)
    sum = 0.0
    x.each { |v| sum += (v-m)**2 }
    Math.sqrt(sum/(x.size))
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
