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
  end  

  def readConfig(config = @config)
    require 'multiconfig'
    mcg = MultiConfigGenerator.new
    @factors, @levels = mcg.readConfigs(config)
  end

  def encodedSingleRun(p, vecFile, encodedFactors, run = 0)
    #todo rearrange factors according to @factors    
    vectors, nodenames = retrieveVectors(vecFile)
    @vectorStarted = Array.new if not defined? @vectorStarted

    filterByVectorNames(vectors, nodenames)
    
    #not caching the vectors' safe column names. Too much hassle and makes code
    #look complex. Loaded vectors can differ a lot anyway.
    vectorsOrig = vectors.deep_clone
    safeColumnNamesMapping(p ,vectors)        

    vectors.each_pair { |k,v|
      raise "logical error in determining column name of vector #{k}" if vectors[k].nil?
      nlines = !@restrict.nil? && @restrict.include?(k)?@rsize:0
      #We'd like to have runnumber.vectorname.vectornumber i.e. #{run}.#{v} but as
      #runnumber at the start appears to be a float it is an R syntax error
      onerunframe = %{#{v}}

      columnname = v
      $defout.old_puts "columname is #{columnname}" if @debug
      
      restrictGrep = nlines > 0? "-m #{nlines}":""
      p.puts %{tempscan <- scan(p<-pipe('grep #{restrictGrep} "^#{k}\\\\>" #{vecFile}'), list(trashIndex=0,time=0,#{columnname}=0), nlines = #{nlines}, quiet = TRUE)}
      p.puts %{close(p)}
      p.puts %{attach(tempscan)}

      #todo add ordered factor levels for scheme?
      p.puts %{#{onerunframe} <- data.frame( #{expandFactors(encodedFactors, @factors)} , node=#{quoteValue(nodenames[k])}, run=#{run}, time=time, #{columnname}=#{columnname})}      
      p.puts %{detach(tempscan)}

      #Aggregate runs for same node's vector
      aggframe = %{#{@aggprefix}#{v}} 
      key = uniqueVectorName(vectorsOrig[k], nodenames[k])
      $defout.old_puts "key is #{key}" if @debug
      if not @vectorStarted.include?(key)
        @vectorStarted.push(key)
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
 
  def resetVectorStarted
    @vectorStarted = []
  end
  
  def mergeVectorsWithRScript
    pairs = %w( a.BAck.recv a.BU.sent a.BBAck.recv a.BBU.sent a.L2.Up a.L2.Down a.LBAck.recv a.LBU.sent)
    var = ""
    0.upto(pairs.size/2 - 1) do |i|
      i = i*2
      lhs = pairs[i]
      rhs = pairs[i+1]
      if lhs == "a.L2.Up"
        var += "#{lhs} = #{lhs}[#{lhs}$time > 1,] #ignore initial link up trigger on sim startup\n"
      end
      var += <<TARGET
if (dim(#{lhs})[1] == dim(#{rhs})[1])
        {
          diff = #{lhs}$time - #{rhs}$time
          attach(#{rhs})
          #{lhs} = cbind(#{lhs}, #{rhs[2..rhs.size]}, diff)
          detach(#{rhs})
          remove(#{rhs}, diff)
          dimnames(#{lhs})[[2]]
        }
TARGET
    end    
    var
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
    curCount = 0
    if @allvecFiles 
      modname = ARGV.shift
      if @dir.nil?
        vecFiles = Dir["#{modname}*_*.vec"]
      else
        vecFiles = Dir["#{@dir}/#{modname}*_*.vec"]
      end
      vecCount = vecFiles.size   
    else
      vecCount = ARGV.size
    end

    output = File.join(Dir.pwd, @rdata)
    while curCount != vecCount do
      if @allvecFiles
        vecFile = vecFiles[curCount]
      else
        vecFile = ARGV.shift
      end
      #Crucial to do this for every vec file instead of trying to build a global to use across many
      #as vector indices are not unique across runs
      retrieveVectors(vecFile)
      curCount += 1    
      next if @count and curCount < @count
      raise "wrong file type as it does not end in .vec" if File.extname(vecFile) != ".vec"     
      encodedFactors = File.basename(vecFile, ".vec").split(DELIM)
      run = encodedFactors.last
      encodedFactors = encodedFactors[1..encodedFactors.size-2]      
      $defout.old_puts " encodedFactors are " + encodedFactors.join(",") if @debug
      $defout.old_puts "---Processing vector file " + vecFile + " #{curCount}/#{vecCount}" if @verbose
      if not @printVectors
        encodedSingleRun(p, vecFile, encodedFactors, run)
        p.puts %{save.image("#{output}")} if curCount%3000 == 0
      end
    end
    
    if @printVectors
      self.uniqueVectorNames.each_pair { |k,name|
        $defout.old_puts %{#{k} #{name.join(",")}}
      }
    end

    p.puts %{save.image("#{output}")}     

    p.puts %{q("no")}
  rescue Errno::EPIPE => err
    $defout.old_puts err
    $defout.old_puts err.backtrace    
    $defout.old_puts "last R commands (earliest first): "
    $lastCommand.each{|l|
      $defout.old_puts l  
    } 
  rescue => err
    $defout.old_puts err
    $defout.old_puts err.backtrace
    p.puts %{save.image("otherException.Rdata")} if not p.nil?
    $defout.old_puts `cat /proc/meminfo` 

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
  #include RInterface 
  
  def testVectorIndexDifferent
    p = IO.popen(RInterface::RSlave, "w+")
    $app.readConfig(@yaml)
    $app.filter = nil
    $app.exclude = nil    
    @input.each_pair{|f,v|
      vecFile = f
      encodedFactors = File.basename(vecFile, ".vec").split(RInterface::DELIM)
      run = encodedFactors.last
      encodedFactors = encodedFactors[1..encodedFactors.size-2]      
      $app.encodedSingleRun(p, vecFile, encodedFactors, run)      
    }            
    rdata = "test.Rdata"
    output = File.join("/tmp", rdata)
    p.puts %{save.image("#{output}")}
    assert_equal(["a.transitTimes.cn", "a.transitTimes.mn"],
                 $app.retrieveRObjects(p),
                 "all vectors")
    #printRObjects(p, retrieveRObjects(p, "a.transitTimes.cn"))
    assert_equal(12,                 
                 $app.dimRObjects(p, "a.transitTimes.cn")["a.transitTimes.cn"][0],
                 "We should have 12 rows of transitTimes.cn")
    assert_equal(10,
                 $app.dimRObjects(p,"a.transitTimes.mn")["a.transitTimes.mn"][0],
                 "10 rows for a.transitTimes.mn")    
    
    #Notice output contains node cn and also transitTimes are from cn this is 
    #not correct. Thus because of the fact that vector indexes do not contain nodename
    #they overwrite each other.
    p.puts %{q("no")}
  rescue Errno::EPIPE => err
    $defout.old_puts err
    $defout.old_puts err.backtrace
    $defout.old_puts "last R commands (earliest first): "
    $lastCommand.each{|l|
      $defout.old_puts l  
    } 
  rescue => err
    $defout.old_puts err
    $defout.old_puts err.backtrace
    p.puts %{save.image("otherException.Rdata")} if not p.nil?
  ensure
    p.close if p    
    File.delete(output)
  end
  
  def test_excludeByName
    p = IO.popen(RInterface::RSlave, "w+")    
    $app.readConfig(@yaml)
    $app.exclude = Array.new 
    $app.exclude.push "transitTimes mn:cn"
    $app.filter = nil
    @input.each_pair{|f,v|
      vecFile = f
      encodedFactors = File.basename(vecFile, ".vec").split(RInterface::DELIM)
      run = encodedFactors.last
      encodedFactors = encodedFactors[1..encodedFactors.size-2]      
      $app.encodedSingleRun(p, vecFile, encodedFactors, run)      
    }            
    assert_equal(["a.transitTimes.cn"],
                 $app.retrieveRObjects(p), "should exclude 317 and 319")
    assert_equal(12,
      $app.dimRObjects(p,"a.transitTimes.cn")["a.transitTimes.cn"][0],
      "same result as a.transitTimes.cn above unless dup vectors cause havoc")
  ensure
    p.close if p
  end
  
  def test_filterByIndex
    p = IO.popen(RInterface::RSlave, "w+")    
    $app.readConfig(@yaml)   
    $app.filter = %w{317}     
    $app.exclude = nil  
    @input.each_pair{|f,v|
      vecFile = f
      encodedFactors = File.basename(vecFile, ".vec").split(RInterface::DELIM)
      run = encodedFactors.last
      encodedFactors = encodedFactors[1..encodedFactors.size-2]      
      $app.encodedSingleRun(p, vecFile, encodedFactors, run)
      #remove warning as we tempscan and p disappeared somehow and when we rm them
      #generates these warnings
      p.puts %|rm(last.warning)|
    }            
    assert_equal([6, 8],
                 $app.dimRObjects(p,"a.transitTimes.mn")["a.transitTimes.mn"],
                 "should include 317 only although total of 10 with 319 too if by name \
hence evil of by vector across many files")

    assert_equal(["a.transitTimes.mn"],
                 $app.retrieveRObjects(p), "a.transitTimes.mn only")
                 
                 
  ensure
    p.close if p
  end

  def test_filterByName
    p = IO.popen(RInterface::RSlave, "w+")    
    $app.readConfig(@yaml)   
    $app.filter = Array.new
    $app.filter.push "transitTimes mn:cn"
    $app.exclude = nil  
    @input.each_pair{|f,v|
      vecFile = f
      encodedFactors = File.basename(vecFile, ".vec").split(RInterface::DELIM)
      run = encodedFactors.last
      encodedFactors = encodedFactors[1..encodedFactors.size-2]      
      $app.encodedSingleRun(p, vecFile, encodedFactors, run)      
    }            
    assert_equal(["a.transitTimes.mn"],
                 $app.retrieveRObjects(p),
                 "should include only 317 and 319")
                 
    assert_equal([10, 8],
                 $app.dimRObjects(p,"a.transitTimes.mn")["a.transitTimes.mn"])    

  ensure
    p.close if p
  end

  def testDupVectorIndex
    p = IO.popen(RInterface::RSlave, "w+")
    $app.readConfig(@yaml)
    $app.filter = nil
    $app.exclude = nil    
    @inputDup.each_pair{|f,v|
      vecFile = f
      encodedFactors = File.basename(vecFile, ".vec").split(RInterface::DELIM)
      run = encodedFactors.last
      encodedFactors = encodedFactors[1..encodedFactors.size-2]      
      $app.encodedSingleRun(p, vecFile, encodedFactors, run)      
    }            
    rdata = "test2.Rdata"
    output = File.join("/tmp", rdata)
    p.puts %{save.image("#{output}")}
    assert_equal(["a.transitTimes.cn", "a.transitTimes.mn"],
                 $app.retrieveRObjects(p),
                 "all vectors")
    #printRObjects(p, retrieveRObjects(p, "a.transitTimes.cn"))
    assert_equal(19,                 
                 $app.dimRObjects(p, "a.transitTimes.cn")["a.transitTimes.cn"][0],
                 "We should have 19 rows of transitTimes.cn")
    assert_equal(15,                 
                 $app.dimRObjects(p, "a.transitTimes.mn")["a.transitTimes.mn"][0],
                 "We should have 15 rows of transitTimes.mn")
    
    #Notice output contains node cn and also transitTimes are from cn this is 
    #not correct. Thus because of the fact that vector indexes do not contain nodename
    #they overwrite each other.
    p.puts %{q("no")}
  rescue Errno::EPIPE => err
    $defout.old_puts err
    $defout.old_puts err.backtrace
    $defout.old_puts "last R commands (earliest first): "
    $lastCommand.each{|l|
      $defout.old_puts l  
    } 
  rescue => err
    $defout.old_puts err
    $defout.old_puts err.backtrace
    p.puts %{save.image("otherException.Rdata")} if not p.nil?
  ensure
    p.close if p    
    File.delete(output)
  end

  def test_mergeOutput
#    $defout.old_puts $app.mergeVectorsWithRScript
  end  
  
  def setup
    newVectors = ["EHComp_hmip_50_50_y_1.vec", "EHComp_hmip_50_50_n_2.vec"]
    oldVectors = ['EHComp_eh_100_20_n_2.vec', "EHComp_eh_100_20_n_1.vec"]
    dupVectors = ['EHComp_eh_200_10_n_5.vec']

    @yaml = "test.yaml"
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

    @input[@yaml] = <<END
--- 
- - scheme
  - dnet
  - dmap
  - ar
- dmap: &id001 
    - "2"
    - "20"
    - "50"
  dnet: &id002 
    - "50"
    - "100"
    - "200"
    - "500"
  scheme: 
  - hmip
  - mip
  - eh
  ar: 
  - y
  - n
- hmip: 
  - !ruby/object:ToggleAction 
    attribute: hierarchicalMIPv6Support
    symbol: :xml
    testedValue: false
    value: "\\"on\\""
  dmap: 
  - !ruby/object:SetFactorChannelAction 
    attribute: delay
    channel: EHCompIntranetCable
    symbol: :ned
    value: *id001
  dnet: 
  - !ruby/object:SetFactorChannelAction 
    attribute: delay
    channel: EHCompInternetCable
    symbol: :ned
    value: *id002
  eh: 
  - !ruby/object:ToggleAction 
    attribute: hierarchicalMIPv6Support
    symbol: :xml
    testedValue: false
    value: "\\"on\\""
  ar: 
    n: 
    - !ruby/object:ToggleAction 
      attribute: linkUpTrigger
      symbol: :ini
      testedValue: false
      value: "false"
    - !ruby/object:SetAction 
      attribute: MaxFastRAS
      symbol: :xml
      value: "\\"0\\""
    - !ruby/object:ToggleAction 
      attribute: optimisticDAD
      symbol: :xml
      testedValue: false
      value: "\\"off\\""
    - !ruby/object:SetAction 
      attribute: HostMaxRtrSolDelay
      symbol: :xml
      value: "\\"1\\""
    y: 
    - !ruby/object:ToggleAction 
      attribute: linkUpTrigger
      symbol: :ini
      testedValue: false
      value: "true"
    - !ruby/object:SetAction 
      attribute: MaxFastRAS
      symbol: :xml
      value: "\\"10\\""
    - !ruby/object:ToggleAction 
      attribute: optimisticDAD
      symbol: :xml
      testedValue: false
      value: "\\"on\\""
    - !ruby/object:SetAction 
      attribute: HostMaxRtrSolDelay
      symbol: :xml
      value: "\\"0\\""
  mip: 
  - !ruby/object:ToggleAction 
    attribute: hierarchicalMIPv6Support
    symbol: :xml
    testedValue: false
    value: "\\"off\\""
END

    @inputDup = @input.deep_clone
    #repeat vector index but for totally different vector name
    @inputDup[dupVectors[0]] = <<END
vector 319  "ehComp_eh_200_10_nNet.mn.udpApp[0]"  "transitTimes cn"  1
319 7.06035864061 0.0103586406061
319 7.06038952061 0.0503895206061
319 7.06035864061 0.0103586406061
319 7.06038952061 0.0503895206061
319 7.06035864061 0.0103586406061
319 7.06038952061 0.0503895206061
319 7.06035864061 0.0103586406061
vector 318  "ehComp_hmip_200_10_nNet.cn.udpApp[0]"  "transitTimes mn"  1
318 9.04282581333 0.0828258133333
318 9.04330212  0.05330212
318 9.04330212  0.05330212
318 9.04330212  0.05330212
318 9.04330212  0.05330212
END

    #change dir otherwise may overwrite some results
    Dir.chdir("/tmp")
    @inputDup.each_pair{|k,v|
      File.open("#{k}", 'w'){|f|
        f.puts("#{v}")
      }
    }
    @input.delete(@yaml)
    @inputDup.delete(@yaml)
    $app.uniqueVectorNames = nil
    $app.resetVectorStarted
  end
  
  def teardown
    @inputDup.each_pair{|k,v|
      File.delete(k)
    }
    File.delete(@yaml)
  end

end

if $test
  ##Fix Ruby debugger to allow debugging of test code
  require 'test/unit/ui/console/testrunner'
  result = Test::Unit::UI::Console::TestRunner.run(TC_ImportOmnet)
  if not result.passed?
    exit(result.failure_count) if result.failure_count > 0
    exit(result.error_count)
  end
end
# }}}
