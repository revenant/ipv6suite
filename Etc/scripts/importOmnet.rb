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
require 'General'
require 'RImportOmnet'

$test  = false

#
# TODO: Add Description Here
#
class ImportOmnet < RImportOmnet
  include General

  VERSION       = "1.0"
  REVISION_DATE = "28 Aug 2006"
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
    get_options
  end  

  def readConfig

    @factors = ["scheme", "dnet", "dmap", "ar"]

    @levels = {}
    @levels[@factors[0]] = ["hmip", "mip", "eh"]
    @levels[@factors[1]] = ["50", "100", "200", "500"]
    @levels[@factors[2]] = ["2", "20", "50"]
    @levels[@factors[3]] = ["y", "n"]

  end

  def encodedSingleRun(p, vecFile, encodedFactors, run = 0)
    #todo rearrange factors according to @factors
    $defout.old_puts "---Processing vector file " + vecFile if @verbose
    vectors, nodenames = retrieveVectors(vecFile)
    @vectorStarted = Array.new if not defined? @vectorStarted
    #not caching the vectors' safe column names. Too much hassle and makes code
    #look complex. Loaded vectors can differ a lot anyway.

    safeColumnNamesMapping(p ,vectors)
    
    filterVectors(vectors)

    vectors.each_pair { |k,v|
      raise "logical error in determining column name of vector #{k}" if vectors[k].nil?
      nlines = !@restrict.nil? && @restrict.include?(k)?@rsize:0
      #We'd like to have runnumber.vectorname.vectornumber i.e. #{run}.#{v} but as
      #runnumber at the start appears to be a float it is an R syntax error
      onerunframe = %{#{v}}

      columnname = v
      $defout.old_puts columnname if @verbose

      restrictGrep = nlines > 0? "-m #{nlines}":""
      p.puts %{tempscan <- scan(p<-pipe('grep #{restrictGrep} "^#{k}\\\\>" #{vecFile}'), list(trashIndex=0,time=0,#{columnname}=0), nlines = #{nlines})}
      p.puts %{close(p)}
      p.puts %{attach(tempscan)}

      #todo add ordered factor levels for scheme?
      p.puts %{#{onerunframe} <- data.frame( #{expandFactors(encodedFactors, @factors)} , node=#{quoteValue(nodenames[k])}, run=#{run}, time=time, #{columnname}=#{columnname})}
      
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
    }#end vectors.each
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
    readConfig
#    factor(x, @{levels[@factors[i]]}, labels=@{levels[@factors[i]].map{|x| x + "ms"}}, ordered = TRUE)
    while ARGV.size > 0 do
      vecFile = ARGV.shift      
      raise "wrong file type as it does not end in .vec" if File.extname(vecFile) != ".vec"

      encodedFactors = File.basename(vecFile, ".vec").split(DELIM)
      run = encodedFactors.last
      encodedFactors = encodedFactors[1..encodedFactors.size-2]      
      encodedSingleRun(p, vecFile, encodedFactors, run)    
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
    p.puts %{save.image("otherException.Rdata")} if not p.nil?
  ensure
    p.close if p
  end#run
  
end#class ImportOmnet

$app = ImportOmnet.new
$app.run if not $test

exit unless $test



# {{{ ##Unit test for this class/module#
require 'test/unit'

class TC_ImportOmnet < Test::Unit::TestCase
  include RInterface
  include General
  def test_quoteString
    #assert_equal("\"quotedString\"",
    #             quoteString("quotedString"),
    #             "The quotes should surround the quotedString")
    print Dir.pwd
    exit
  end

  def testVectorIndexDifferent
		p = IO.popen(RSlave, "w+")
		$app.readConfig
    @input.each_pair{|f,v|
      vecFile = f
      encodedFactors = File.basename(vecFile, ".vec").split(DELIM)
      run = encodedFactors.last
      encodedFactors = encodedFactors[1..encodedFactors.size-2]      
      $app.encodedSingleRun(p, vecFile, encodedFactors, run)      
    }    
    output = File.join("test.Rdata")
    p.puts %{save.image("#{output}")}
    printRObjects(p, retrieveRObjects(p, "a.transitTimes.cn"))
    assert_equal(12,                 
    						 sizeRObjects(p, "a.transitTimes.cn")["a.transitTimes.cn"][0],
    						 "We should have 12 rows of transitTimes.cn")
    
    #Notice output contains node cn and also transitTimes are from cn this is 
    #not correct. Thus because of the fact that vector indexes do not contain nodename
    #they overwrite each other.
    p.puts %{q("no")}
  rescue Errno::EPIPE => err
    $defout.old_puts err
    $defout.old_puts err.backtrace
    $defout.old_puts "last R command: #{$lastCommand.shift}"
  rescue => err
    $defout.old_puts err
    $defout.old_puts err.backtrace
    p.puts %{save.image("otherException.Rdata")} if not p.nil?
  ensure
    p.close if p    
  end
  
  def setup
    newVectors = ["EHComp_hmip_50_50_y_1.vec", "EHComp_hmip_50_50_n_2.vec"]
    oldVectors = ['EHComp_eh_100_20_n_2.vec', "EHComp_eh_100_20_n_1.vec"]
    @input = Hash.new
    @input[newVectors[0]] = <<END
vector 318  "ehComp_hmip_50_50_yNet.mn.udpApp[0]"  "transitTimes cn"  1
318 8.04282581333 0.0428258133333
318 8.04330212  0.02330212
vector 319  "ehComp_hmip_50_50_yNet.cn.udpApp[0]"  "transitTimes mn"  1
319 8.06035864061 0.0603586406061
319 8.06038952061 0.0403895206061
END
    @input[newVectors[1]] = <<END
vector 318  "ehComp_hmip_50_50_nNet.mn.udpApp[0]"  "transitTimes cn"  1
318 8.04260852  0.04260852
318 8.04338482667 0.0233848266667
vector 319  "ehComp_hmip_50_50_nNet.cn.udpApp[0]"  "transitTimes mn"  1
319 8.06027864061 0.0602786406061
319 8.06030952061 0.0403095206061
319 8.06066731333 0.0206673133333
END
    @input[oldVectors[0]]= <<END
vector 316  "ehComp_eh_100_20_nNet.mn.udpApp[0]"  "transitTimes cn"  1
316 8.08318852  0.08318852
316 8.08396482667 0.0639648266667
316 8.08454113333 0.0445411333333
316 8.10107370667 0.0410737066667
vector 317  "ehComp_eh_100_20_nNet.cn.udpApp[0]"  "transitTimes mn"  1
317 8.12027864061 0.120278640606
317 8.12030952061 0.100309520606
317 8.12034040061 0.0803404006061
END
    @input[oldVectors[1]] = <<END
vector 316  "ehComp_eh_100_20_nNet.mn.udpApp[0]"  "transitTimes cn"  1
316 8.08248852  0.08248852
316 8.08308482667 0.0630848266667
316 8.08416113333 0.0441611333333
316 8.10141370667 0.0414137066667
vector 317  "ehComp_eh_100_20_nNet.cn.udpApp[0]"  "transitTimes mn"  1
317 8.12053864061 0.120538640606
317 8.12056952061 0.100569520606
317 8.12060040061 0.0806004006061
END
    @input.each_pair{|k,v|
      File.open("#{k}", 'w'){|f|
        f.puts("#{v}")
      }
    }
  end
  
  def teardown

  end

end

if $test
  ##Fix Ruby debugger to allow debugging of test code
  require 'test/unit/ui/console/testrunner'
  Test::Unit::UI::Console::TestRunner.run(TC_ImportOmnet)
end
# }}}
