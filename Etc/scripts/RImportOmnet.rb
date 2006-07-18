#! /usr/bin/env ruby
#
#  Author: Johnny Lai
#  Copyright (c) 2006 Johnny Lai
#
# =DESCRIPTION
# Imports omnetpp.vec files
# 
# Based on RImportOmnet.rb but simplified a lot
# 

require 'optparse'
require 'pp'

#controls whether R commands are printed on screen. Turn on via -X option
$debug = false
$test  = false

#global is evil but don't know which other way to pass variable between diff
#classes and in diff scopes
$lastCommand = []
class IO
  alias old_puts puts

  # 
  # As we only use pipe to R any other puts is to terminal mainly. puts to R
  # should be done in a put/get combo in order to keep both processes in sync
  # otherwise ruby is much faster than R and may overload input buffer of
  # R. Keeping in sync does not work in practice read inline comments of self.puts
  #
  def puts(str)
    if self.tty?
      self.old_puts str
      return
    end

    $lastCommand.push str

    if $debug
      $defout.old_puts str 
      $defout.flush
    end

    self.old_puts str

    $lastCommand.shift if $lastCommand.size > 2
    if false
      #keeping in sync does not work in practice because we need to read the
      #output of R and we would have swallowed it up here unless we can somehow
      #do gets here and check that its not a newline and reinsert it back in
      #after doing another gets so we get the newline
      self.old_puts %{cat("\\n")}
      self.gets
    end
  end
end

alias old_execute `

#Capture standard error too when executing things via backqoute or %x
def `(cmd)
  old_execute(cmd + " 2>&1")
end

#
# Imports omnetpp.vec files
#
class RImportOmnet
  VERSION       = "$Revision: 1.8 $"
  REVISION_DATE = "$Date: 2005/03/11 18:38:25 $"
  AUTHOR        = "Johnny Lai"

  RSlave = "R --slave --quiet --vanilla --no-readline"
  #Doing **/*.ext would find all files with .ext recursively while with only one
  #* it is like only in one subdirectory deep

  attr_accessor :rdata, :filter

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
  # Initialse option variables in case the actual option is not parsed in 
  #
  def initialize
    @debug    = false 
    @verbose  = false
    @quit     = false
    @filter   = nil
    @rdata    = "test2.Rdata"
    @restrict = nil
    @rsize    = 0 #no restriction on vector length
    @aggprefix = %{a.} #dataframes have this as prefix when aggregating runs
    #Form data frame name from vector file label's nodename 
    @nodename = false 
    @aggFrames = nil #store aggframe names to remove them 
    @printVectors = false #Print only vector names and quit

    get_options

  rescue => err
    STDERR.puts usage
    STDERR.puts "\n" if err
    STDERR.puts err  if err
    STDERR.puts err.backtrace if err
    exit 1
  end

  #
  # Returns usage string
  #
  def usage
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
      opt.separator "Usage: #{File.basename __FILE__} [options] [omnetpp.vec] "
      opt.separator ""
      opt.separator "Specific options:"

      
      opt.on("-n", "Use vector file label's nodename ",
             "(assuming nodename is second component after network name)",
             "to combine multinode vectors together into one frame with nodename column"){|@nodename|}

      opt.on("--output=filename","-o", String, "Output R data to filename"){|@rdata|}

      # List of arguments.
      opt.on("-f", "--filter x,y,z", Array, "list of vector indices to grab, other indexes are ignored") {|@filter|
        self.filter.uniq!
      }

      opt.on("-c", "Collect all Rdata files in different subdirs into one with directory names as scheme. ",
             "If -a given will aggregate runs first before collecting. Run from the top dir"){|@collect|}

      opt.on("-r", "--restrict x,y,z", Array, "Restrict these vectors to at most size rows"){|@restrict|}

      opt.on("-s", "--size x", Integer, "restrict vectors specified to --restrict to size rows "){|@rsize|}

      opt.on("-p", " Print the vector names and their corresponding numeric indices"){|@printVectors|} 
      opt.separator ""
      opt.separator "Common options:"

      #Try testing with this 
      #ruby __FILE__ -x -c -s test
      opt.on("-x", "parse arguments and show Usage") {|@quit|
        pp self
        (print ARGV.options; exit) 
      }

      opt.on("--doc=DIRECTORY", String, "Output rdoc (Ruby HTML documentation) into directory"){|dir|
        system("rdoc -a --webcvs=\"http://localhost/cgi-bin/viewcvs.cgi/\" --tab-width=2 --diagram --inline-source -N -o #{dir} #{__FILE__}")
	exit
      }

      opt.on("--verbose", "-v", "print intermediate steps to STDOUT"){|@verbose|}

      opt.on("--debug", "-d", "print debugging info to STDOUT"){|@debug| 
        $debug = @debug
      }

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

  end
  
#  private
  #
  # Unused except in unittest.  Retrieve hash of vector index -> label from vec
  # file filename (index is still a numerical string) where label is the vector
  # name in file.
  #
  def retrieveLabels(filename)
    vectors = Hash.new
    
    #foreach less verbose than opening file and reading it
    IO::foreach(filename) {|l|
      case l
      when /^[^\d]/
        #Retrieve vector number
        i = l.split(/\s+/,4)[1]
        #Retrieve vector name
        s = l.split(/\s+/,4)[3]
        #Remove "" and last number
        s = s.split(/["]/,3)[1]
        if not vectors.value?(s)
          vectors[i]=s
        else
          #Retrieve last component of mod path to create unique R column names
          #when identical vector names
          modpath = l.split(/\s+/,4)[2]
          #remove the quote
          mod = modpath.split(".").last.chop!
          vectors[i]="#{mod}.#{s}"
        end
      end
    }
    p vectors if @debug
    return vectors
  end

  #Retrieve hash of vector index -> label from vec file filename (index is still
  #a numerical string) where label is the vector name in file. All labels have
  ##.#{index} appended

  #probably better to return well formed columnname, nodename and index
  #separately rather than encoded in one string
  def retrieveLabelsVectorSuffix(filename)
    vectors = Hash.new
    
    IO::foreach(filename) {|l|
      case l
      when /^[^\d]/
        #Retrieve vector number
        i = l.split(/\s+/,4)[1]
        #Retrieve vector name
        s = l.split(/\s+/,4)[3]
        #Remove "" and last number
        s = s.split(/["]/,3)[1]
        #add nodename to column name
        s += "." + l.split(/\s+/,4)[2].split(/\./)[1] if @nodename
        #add vector index to end
        s += "." + i 
        vectors[i] = s
      end
    }
    p vectors if @debug
    return vectors
  end

  #
  # Retrieve array of object names that match the R regular expression in pattern
  #
  def retrieveRObjects(p, pattern = "")
    p.puts %{cat(ls(pat="#{pattern}"),"\\n")}
    arrayCode =  %{%w[#{p.gets.chomp!}]}
    eval(arrayCode)
  end
  
  #
  # array is returned from retrieveRObjects
  def printRObjects(p, array)
    results = Array.new
    array.each {|e|
      p.puts %{#{e}\n}
      results.push(p.gets.chomp!)
    }
    results.each{|e|
      p e
    }
  end
  #
  # Form safe column names from omnetpp vector names
  # p is pipe to R 
  # vectors is hash of index -> vector name produced from retrieveLabels
  def safeColumnNames(p, vectors)
    p.puts %{l<-c("#{vectors.values.join("\",\"")}")}
    p.puts %{l<-make.names(l)}
    #newline is needed otherwise gets stalls. Will get an extra empty element
    #but does not matter as we don't use it
#    p.old_puts %{cat(l,sep="\\",\\"","\\n")}
#    arrayCode =  %{["#{p.gets.chomp!}"]}
    p.puts %{cat(l,sep=" ","\\n")}
    arrayCode =  %{%w[#{p.gets.chomp!}]}
    p.puts %{rm(l)}
    #Eval only only does expressions and not statements?
    eval(arrayCode)
  end

  #pause ruby so R can finish its operations
  def waitForR(rpipe)
    rpipe.puts %{cat("\\n")}
    rpipe.gets
  end

  # Read data from OMNeT++ vecFile and convert to dataframe using vector name as
  # frame name prefix and n as suffix. n is the run number.  Vector names
  # may be duplicated so the last component of module path is used too if this
  # happens. I guess the best way is to append the vector number to frame name
  # (assuming users don't run half the runs and add new vectors and then run
  # rest of variants). To totally prevent any of this stuff happening we should
  # freeze vector numbers and make sure vector names are unique across whole sim
  # which may be unrealistic?
  def singleRun(p, vecFile, n, scheme)
    vectors = retrieveLabelsVectorSuffix(vecFile)
    @vectorStarted = Array.new if not defined? @vectorStarted
 
    #not caching the vectors' safe column names. Too much hassle and makes code
    #look complex. Loaded vectors can differ a lot anyway.

    a = safeColumnNames(p, vectors)
    p a if @debug
    i = 0 
    vectors.each_pair { |k,v|
      # does not update value in hash only iterator v
      #v = a[i] 
      vectors[k] = a[i]
      i+=1
      raise "different sized containers when assigning safe column names" if vectors.keys.size != a.size
    }

    unless self.filter.nil?
      newIndices = vectors.keys & self.filter
      vectors.delete_if{|k,v|
        not newIndices.include? k
      }
    end

    vectors.each_pair { |k,v|
      raise "logical error in determining common elements" if vectors[k].nil?
      nlines = !@restrict.nil? && @restrict.include?(k)?@rsize:0
      #We'd like to have runnumber.vectorname.vectornumber i.e. #{n}.#{v} but as
      #runnumber at the start appears to be a float it is an R syntax error
      onerunframe = %{#{v}.#{n}}
      #Remove vector index from column names
      columnname = v.sub(%r{[.]\d+$},"")
      ncolumnname = columnname
      #remove nodename from vector name
      ncolumnname = removeLastComponentFrom(columnname) if @nodename
      restrictGrep = nlines > 0? "-m #{nlines}":""
      p.puts %{tempscan <- scan(p<-pipe('grep #{restrictGrep} "^#{k}\\\\>" #{vecFile}'), list(trashIndex=0,time=0,#{columnname}=0), nlines = #{nlines})}
      p.puts %{close(p)}
      p.puts %{attach(tempscan)}

      p.puts %{#{onerunframe} <- data.frame(scheme = "#{scheme}", run=#{n}, time=time, #{ncolumnname}=#{columnname})}

      nodename = v.split(/\./)[-2] #second last component is nodename
#       $defout.print "nodenmae is ", nodename

      #encode nodename into data frame now rather than later to save hassle of
      #aggregating frames

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

  public

  def printVectorNames(vecFile)
    v = retrieveLabelsVectorSuffix(vecFile) 
    v.each_pair { |k,name|
      $defout.old_puts %{#{k} #{name}}
    }
  end

  def removeLastComponentFrom(string, sep=/\./)
    string.reverse.split(sep, 2)[1].reverse
  end

  def vectorname(rvectorname)
    v = rvectorname.sub(@aggprefix, "")
    #remove index
    v = removeLastComponentFrom(v)
    #remove nodename
    removeLastComponentFrom(v)
  end

  def mergeVectorsWithRScript
    pairs = %w( a.BAck.recv.mn.311 a.BU.sent.mn.312 a.BBAck.recv.mn.316 a.BBU.sent.mn.315 a.L2.Up.mn.318 a.L2.Down.mn.319 a.LBAck.recv.mn.314 a.LBU.sent.mn.313)
    p pairs
    var = ""
    0.upto(pairs.size/2 - 1) do |i|
      lhs = pairs[i]
      rhs = pairs[i+1]
      i = i + 2
p lhs, rhs
      if rhs == "a.BBAck.recv.mn.316"
        var += "#{rhs}[#{rhs}$time > 1] #ignore initial link up trigger on sim startup\n"
      end
      var += <<TARGET
if (dim(#{lhs})[1] == dim(#{rhs})[1])
        {
          diff = #{lhs}$time - #{rhs}$time
          attach(#{rhs})
          #{lhs} = cbind(#{lhs}, #{vectorname(rhs)}, diff)
          detach(#{rhs})
          remove(#{rhs})
          dimnames(#{lhs})[[2]]
        }
TARGET
    end
    var
  end

  #
  # Launches the application
  #  RImportOmnet.new.run
  #
  def run
    return if $test

    p = IO.popen(RSlave, "w+")
    raise ArgumentError, "No vector file specified", caller[0] if ARGV.size < 1




    if @printVectors
      printVectorNames(ARGV[0])
      return
    end

    modname = "EHAnalysism"
    
    while ARGV.size > 0 do
      vecFile = ARGV.shift
      raise "wrong file type as it does not end in .vec" if File.extname(vecFile) != ".vec"
      encodedScheme = File.basename(vecFile, ".vec").sub(modname,"")
      encodedScheme =~ /(\d+)$/
      n = $1
      scheme = $` 
      singleRun(p, vecFile, n, scheme)

    end

    output = File.join(File.dirname(vecFile), @rdata)
    p.puts %{save.image("#{output}")}     

    p.puts %{q("no")}

  rescue Errno::EPIPE => err
    $defout.old_puts err
    $defout.old_puts err.backtrace
    $defout.old_puts "last R command: #{$lastCommand.shift}"
  rescue => err
    $defout.old_puts err
    $defout.old_puts err.backtrace
    p.puts %{save.image("otherException.Rdata")}
  ensure
	  p.close if p
  end#run
  
end#RImportOmnet

#main
app = RImportOmnet.new.run if not $test

exit unless $test

##Unit test for this class/module
require 'test/unit'

class TC_RImportOmnet < Test::Unit::TestCase
  def test_safeColumnNames
    a=@imp.safeColumnNames(@p, Hash[1,"bad names",5,"compliant.column-name"])   
    assert_equal(a, %w{compliant.column.name bad.names}, "Column names should not differ!")
  end

  def test_retrieveRObjects
    @p.puts %{s.me <- data.frame}
    @p.puts %{s.you <- data.frame}
    @p.puts %{s.us.youtoo <- data.frame}
    @p.puts %{u.us.youtoo <- data.frame}
    @p.puts %{t.us.youtoo <- data.frame}
    @p.puts %{z.us.youtoo <- data.frame}
    @p.puts %{super.us.youtoo <- data.frame}
    a = @imp.retrieveRObjects(@p, "^s[.].+")
    assert_equal(a, %w{s.me s.us.youtoo s.you}, "list of matching R objects differ!")
  end

  TestFileName = "test.vec"
  #Test read of sample vector file and also filtering
  def test_singleRun
    #truncate w+
    File.open(TestFileName, "w+"){|f|
      (var = <<TARGET).gsub!(/^\s+/, '')
vector 3  "saitEHCalNet.mn.networkLayer.proc.ICMP.nd"  "Movement Detection Latency"  1
3 143.61245 143.61245
vector 6  "saitEHCalNet.mn.linkLayers[0].networkInterface"  "IEEE 802.11 HO Latency"  1
6 143.627909  0.253460132
vector 5  "saitEHCalNet.mn.networkLayer.proc.mobility"  "handoverLatency"  1
5 143.714402  0.339952951
vector 0  "saitEHCalNet.mn.networkLayer.proc.ICMP.icmpv6Core"  "pingEED"  1
0 155.000993  0.0109926666
0 155.010813  0.010812523
0 155.020673  0.010672523
0 155.030873  0.010872523
0 155.040773  0.010772523
0 155.050853  0.010852523
0 155.060413  0.010412523
0 155.070613  0.010612523
TARGET
       f.print var
    }

     #vanilla singleRun
    @imp.rdata = TestFileName.gsub(/\.vec$/, ".Rdata")
    @imp.singleRun(@p, TestFileName, 3, "dudscheme")
    @p.puts  %{save.image("#{@imp.rdata}")}
    # Race conditions exist so sync to R first otherwise may raise missing file exception
    @imp.waitForR(@p)
    raise "expected Rdata file #{@imp.rdata} not found in #{Dir.pwd}" if not (File.file? @imp.rdata)

    @p.puts %{rm(list=ls())}
    @p.puts %{load("#{@imp.rdata}")}   
    e = %w{a.handoverLatency.5 a.IEEE.802.11.HO.Latency.6 a.Movement.Detection.Latency.3 a.pingEED.0}
    r = @imp.retrieveRObjects(@p)
    assert_equal(e, r, "singleRun test: list of matching R objects differ!")
    File.delete(@imp.rdata)

    #Filter test
    @p.puts %{rm(list=ls())}
    #required to reset state of past read vectors otherwise it expects to join
    #them to existing vectors (removed in previous line)
    @imp = RImportOmnet.new
    @imp.filter = %w{3 6 5}
    @imp.singleRun(@p, TestFileName, 4,"noscheme")
    @p.puts  %{save.image("#{@imp.rdata}")}
    @imp.waitForR(@p)
    raise "expected Rdata file #{@imp.rdata} not found in #{Dir.pwd}" if not (File.file? @imp.rdata)

    @p.puts %{rm(list=ls())}
    @p.puts %{load("#{@imp.rdata}")}
    e = %w{a.handoverLatency.5 a.IEEE.802.11.HO.Latency.6 a.Movement.Detection.Latency.3 }
    r = @imp.retrieveRObjects(@p)
    assert_equal(e, r, "filter test: list of matching R objects differ!")
    File.delete(@imp.rdata, TestFileName)

    #problem with this is we do not know if pipe has more things to read from

    @imp.printRObjects(@p, r)

  end

  def test_duplicateVectorNames

    File.open(TestFileName, "w+"){|f|
      (var = <<TARGET).gsub!(/^\s+/, '')
vector 3  "saitEHCalNet.mn.networkLayer.proc.ICMP.nd"  "Movement Detection Latency"  1
3 143.61245 143.61245
vector 6  "saitEHCalNet.mn.linkLayers[0].networkInterface"  "IEEE 802.11 HO Latency"  1
6 143.627909  0.253460132
vector 5  "saitEHCalNet.mn.networkLayer.proc.mobility"  "handoverLatency"  1
5 143.714402  0.339952951
vector 0  "saitEHCalNet.mn.networkLayer.proc.ICMP.icmpv6Core"  "pingEED"  1
0 155.000993  0.0109926666
vector 2  "saitEHCalNet.mn.networkLayer.proc.ICMP.icmpv6Core"  "handoverLatency"  1
2 265.010862  1.19022885
vector 1  "saitEHCalNet.mn.networkLayer.proc.ICMP.duddup"  "handoverLatency"  1
TARGET
      f.print var
    }
     e = Hash["3", "Movement Detection Latency", "6", "IEEE 802.11 HO Latency", "5", "handoverLatency", "0", "pingEED", "2", "icmpv6Core.handoverLatency", "1", "duddup.handoverLatency"]     
     h = @imp.retrieveLabels(TestFileName)
     #becomes an array of arrays
     el = e.sort
     hl = h.sort
     assert_equal(el, hl, "dupVectorNames retrieveLabels:  hash differs")
     e = %w{Movement.Detection.Latency IEEE.802.11.HO.Latency handoverLatency pingEED icmpv6Core.handoverLatency duddup.handoverLatency}
     r=@imp.safeColumnNames(@p, h)
     e = e.sort
     r = r.sort
     assert_equal(e, r, "dupVectorNames safeColumns: list of matching R objects differ!")

     e = Hash["3", "Movement Detection Latency.3", "6", "IEEE 802.11 HO Latency.6", "5", "handoverLatency.5", "0", "pingEED.0", "2", "handoverLatency.2", "1", "handoverLatency.1"]
     r=@imp.retrieveLabelsVectorSuffix(TestFileName)
     assert_equal(e.sort, r.sort, "dupVectorNames retrieveLabelsVectorSuffix failed")
  end

  def test_vectorname
    assert_equal("BAck.recv", @imp.vectorname("a.BAck.recv.mn.311"))
    assert_equal("BBAck.recv", @imp.vectorname("a.BBAck.recv.mn.316"))
    assert_equal("BU.sent", @imp.vectorname("a.BU.sent.mn.312"))
    assert_equal("L2.Up", @imp.vectorname("a.L2.Up.mn.318"))
#a.BBAck.recv.mn.316 a.BBU.sent.mn.315  a.L2.Down.mn.319 a.LBAck.recv.mn.314 a.LBU.sent.mn.313
  end

  def test_merge
    puts @imp.mergeVectorsWithRScript
  end

  if false
  def test_collect    
    Dir.chdir(File.expand_path(%{~/src/phantasia/master/output/saiteh})){
      `ruby #{__FILE__} -o testcoll.Rdata -a -c -f 3,6,5 -O supertest.Rdata`
      assert_equal(0, $?.exitstatus, "collect with  aggregate mode failed")
      `mv supertest.Rdata baselineCollWithAgg.Rdata`
      assert_equal( 0, $?.exitstatus, "mv not successful")
      `ruby #{__FILE__} -c -f 3,6,5 -O supertest.Rdata -o testcoll.Rdata`
      assert_equal(0, $?.exitstatus, "collect mode failed")
      `diff supertest.Rdata baselineCollWithAgg.Rdata`
      assert_equal(0, $?.exitstatus, "files differ when doing collect with/out aggregate mode")
    } if ENV["USER"] == "jmll"
  end

  def testEquation
    equation = "3 + 5"
    #substitute indices with vector names
    equation ~ "\d"

# L2.Up.318.0[2:5,3]-L2.Down.319.0[,3]

#    v = retrieveLabelsVectorSuffix(vecFile) 
    #match (\s+\d+\s+)\
  end

  end

  def setup
    @p = IO.popen( RImportOmnet::RSlave, "w+")
    @imp = RImportOmnet.new
  end
  
  def teardown
    @p.close
  end

end

if $0 != __FILE__ then
  ##Fix Ruby debugger to allow debugging of test code
  require 'test/unit/ui/console/testrunner'
  Test::Unit::UI::Console::TestRunner.run(TC_RImportOmnet)
end
