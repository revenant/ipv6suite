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

  def expandFactors(encodedFactors, factors)
    c = splice(factors, encodedFactors)
    e = []
    Hash[*c].to_a.each{|i|
      e.push(i[0] + "=" + quoteValue(i[1]))
    } 
    %|#{e.join(",")}|
  end

  def removeLastComponentFrom(string, sep=/\./)
    string.reverse.split(sep, 2)[1].reverse
  end

  def quoteValue(value)
    "\"#{value}\""
  end

end
#
# TODO: Add Description Here
#
class ImportOmnet < RImportOmnet
  include General

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
    get_options
  end

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
    #todo rearrange factors according to @factors
    $defout.old_puts "---Processing vector file " + vecFile if @verbose
    vectors, nodename = retrieveVectors(vecFile)
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
      p.puts %{#{onerunframe} <- data.frame( #{expandFactors(encodedFactors, @factors)} , node=#{quoteValue(nodename)}, run=#{run}, time=time, #{columnname}=#{columnname})}
      
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
    readConfigs
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
# }}}
