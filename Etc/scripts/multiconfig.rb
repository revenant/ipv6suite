#! /usr/bin/env ruby
#
#  Author: Johnny Lai
#  Copyright (c) 2006 
#
# =DESCRIPTION
#
# Generates config files for runs based on scenario description in terms of
# factors and their associated levels. The filenames for each specific scenario
# is codified as BaseName_f1_f2_f3_...fn.{xml,ini,ned} where f1,f2,f3,..fn are a
# specific level within their respective factors (thus there are n factors here)
# and will always be ordered as such unless the description of factors and
# levels are changed. The vector file produced will have run number appended
# i.e. filename ends like so _#{runnumber}.vec.
# 
#
# =REVISION HISTORY
#  INI 2006-08-23 Changes
#

#Put a directory into load path with other custom ruby libs
$LOAD_PATH<<`echo $HOME`.chomp + "/lib/ruby"
#Put current dir in load path in case this module requires other files
$LOAD_PATH<<File.dirname(__FILE__)

require 'optparse'
require 'pp'
require 'General'

$test = false

DELIM = "_"

class Action
  include General #for quoteValue
  
  def initialize(symbol, attribute, value)
    @symbol = symbol
    @attribute = attribute
    @value = value
  end
  
  attr_accessor :symbol, :value , :attribute
  
  #returns index for matching first line 
  def searchForLine(nedfile, regexp)
    nedfile.each_with_index {|line,index|
      if regexp.match(line) then
        return index
      end
    }
    0
  end
  
end

#TODO: Proper setting of XML values by inserting parameter if not exist into the
#proper XPath Query Regexp and validating the document
class SetAction < Action
  def initialize(symbol, attribute, value)
    super(symbol, attribute, value)
    @value = quoteValue(@value) if @symbol == :xml
  end
  def apply(line, index = nil, file = nil, level = nil)
    if line =~ /#{@attribute}\s*=\s*(\S+)/       
      if @symbol == :xml
        line.sub!(/#{@attribute}\s*?=\s*?([[:alnum:]"]+)/, "#{@attribute}=#{@value}")
#      elsif @symbol == :inifile or @symbol == :runtime
#          line = "#{@attribute}=#{@value}"
      else
        #replace the whole value even if space separated
        line.sub!(/#{@attribute}\s*?=(\s*?(\S+))+/, "#{@attribute}=#{@value}")            
      end     
    end
  end
end


class ToggleAction < SetAction
  def initialize(symbol, attribute, value)
    @testedValue = false
    @symbol = symbol
    if @symbol == :xml 
      if value
        @value = "on"
      else
        @value = "off"
      end
    else
      if value
        @value = 'true'
      else
        @value = 'false'
      end
    end
    super(@symbol, attribute, @value)
  end
  
  def apply(line, index = nil, file = nil, level = nil)
    #Regexp.new("\S+") will not work as \S is interpreted differently there
    #and \b is backspace not word boundary like in regexp literal
    if line =~ /#{@attribute}=(\S+)/
      if not @testedValue
        if $1.include?('"') and not @symbol == :xml
          raise "error detected xml boolean value in line and yet Action defined as non xml"
        end
        @testedValue = true
      end
      return nil if $1 =~/#{@value}/
      super(line, index, file, level)
    end
    line
  end
end #end class ToggleAction

class SetFactorChannelAction < Action
  def initialize(symbol, channel, attribute, values)
    super(symbol, attribute, values)
    @channel = channel
  end
  
  def apply(line = nil, index = nil, file = "used", level = "used")
    raise "dud level passed in! as not in channels=" + @values.to_s if not @value.include? level
    #? for non greedy i.e. quit as soon as found first matching pattern instead
    #of match as much as possible
    
    return nil if not (line == nil or line == 0)
    file.sub!(Regexp.new(/channel\s+?#{@channel}\s*?\n.*?\n*?\s*?(#{@attribute}\s*?[[:alnum:]-]+?;)\n/m)) { |match|
      previous = $1.dup
      match.sub(/#{previous}/, "#{@attribute} #{level}e-3;")
    }
  end
end

class ReplaceStringAction < Action
  def initialize(symbol, search, replace)
    super(symbol, search, replace)
  end
  def apply(line = used, index = nil, file = "used", level = nil)
    if @symbol == :ned
      file.gsub!(/#{@attribute}/m, "#{@value}")
    else
      line.sub!(/#{@attribute}/, "#{@value}")
    end
  end
end

#
# TODO: Add Description Here
#
class MultiConfigGenerator
  VERSION       = "0.2"
  REVISION_DATE = "<2006-08-27>"
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
    @check = nil

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
  # {{{ Option processing #
  
  #
  # Processes command line arguments
  #
  def get_options
    ARGV.options { |opt|
      opt.banner = version
      opt.banner = "Usage: ruby #{File.basename __FILE__} [options] basename"
      opt.separator ""
      opt.separator "Specific options:"
      
      #Try testing with this 
      #ruby __FILE__ -x -c -s test
      opt.on("-x", "parse arguments and show Usage") {|@quit|}
      
      opt.on("--verbose", "-v", "print intermediate steps to STDERR"){|@verbose|}
      
      opt.on("-r", "--runCount x", Integer, "How many runs to generate for each scenario"){|@runCount|}
      
      opt.on("-c", "--check logfile", String, "Checks output of runs (requires -r option)"){|@check| }
      
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
      
      opt.on_tail("Where basename stands for the set of files basename.{ned,xml,ini} ",
                  "that describes the simulation scenario")
      #Samples end
      
      opt.parse!
    } or  exit(1);
    
    
    raise ArgumentError, "No basename specified", caller[0] if ARGV.size < 1 and not $test
    
    if @quit
      pp self
      (print ARGV.options; exit) 
    end
    
  end
  
  # }}}
  def readConfigs
  
#    factors = ["scheme", "dnet", "dmap", "ar", "error"]
    factors = ["scheme", "dnet", "dmap", "ar"]
    levels = {}
    levels[factors[0]] = ["hmip", "mip", "eh"]
    levels[factors[1]] = ["50", "100", "200", "500"]
    levels[factors[2]] = ["2", "20", "50"]
    levels[factors[3]] = ["y", "n"]
#    levels[factors[4]] = ["0", "1pc"]
    [factors, levels]
  end
  
  def processActions(factors, levels)
    #read from file but for now define here
    @actions={}
    @actions["mip"] = [ToggleAction.new(:xml, 'hierarchicalMIPv6Support', false)]
    
    ##Generate hmip xml: use eh version and add following to crv
    ## mobileIPv6Support="on" mobileIPv6Role="HomeAgent" hierarchicalMIPv6Support="on" map="on"
    ##
    ## this becomes the hmip Action a class of its own to change crv
    ##make the crv the map for hmip by
    ## choosing the iface that is connected to internet as the map iface and make
    ## sure it has a global addr if not assign one (ppp3) check its ip addr if not 301 start make one
    ## on all other ifaces adv that as map entry by adding
    ## AdvSendAdvertisements="on" HMIPAdvMAP="on"
    ## then
    ## another action
    ##change map="off" for everything else i.e. ar*
    
    ## do it by create XML aware regexp and ifa
    @actions["hmip"] = [ToggleAction.new(:xml, 'hierarchicalMIPv6Support', true)]
    @actions["eh"] = [ToggleAction.new(:xml, 'hierarchicalMIPv6Support', true)]
    
    #Look into factors actions before specific levels
    @actions["dnet"]  = [SetFactorChannelAction.new(:ned, "EHCompInternetCable", "delay", levels["dnet"])]
    @actions["dmap"]  = [SetFactorChannelAction.new(:ned, "EHCompIntranetCable", "delay", levels["dmap"])]
    
    @actions["ar"] = {}
    @actions["ar"]["y"] = [ToggleAction.new(:ini, 'linkUpTrigger', true), 
                           SetAction.new(:xml, "MaxFastRAS", 10),
                           ToggleAction.new(:xml, "optimisticDAD", true),
                           SetAction.new(:xml, "HostMaxRtrSolDelay", 0)]
    @actions["ar"]["n"] = [ToggleAction.new(:ini, 'linkUpTrigger', false), 
                           SetAction.new(:xml, "MaxFastRAS", 0),
                           ToggleAction.new(:xml, "optimisticDAD", false),
                           SetAction.new(:xml, "HostMaxRtrSolDelay", 1)]
#    @actions["error"] = {}
#    @actions["error"]["0"] = [SetAction.new(:ini, "errorRate", 0)]
#    @actions["error"]["1pc"] = [SetAction.new(:ini, "errorRate", 0.01)]
  end
  
  # Similar to above or  test case setup fn but in future do 
  # Read from file and have 
  # factor0 = [level0, level1, level2, ... leveln]
  # factor1 = [level0, level1, level2, ... leveln], ...
  # then have the reading bit build up the hash
  def generateConfigNames(factors, levels)
    stack = nil
    final = []
    for f in factors do  
      if stack.nil? 
        stack = levels[f]
        next
      end
      index = 0;
      for c in stack do
        for l in levels[f] do
          final[index] = "#{c}#{DELIM}#{l}"
          index = index + 1
        end
      end
      stack = final.deep_clone
    end
    final
  end #end generateConfigNames
  
  def configCount(factors, levels)
    length = 0
    for f in factors      
      size = levels[f].size
      if length == 0
        length = size
      else
        length = size * length
      end
    end
    length
  end
  
  def  checkRunResults(factors, levels, runCount, basename)
    vecFileCount = runCount * configCount(factors, levels)
    expectedCount = Dir["#{basename}*_*.vec"].size
    puts "Found #{vecFileCount}/#{expectedCount} vector files"
    return if expectedCount == vecFileCount
        
    puts "Counts do not match! Make sure you specified correct run count and basename."
    puts "Runs finished according to log "+ `grep Run #{check} |grep end|wc`
    puts "Runs started according to log " + `grep Run #{check} |grep Prepar|wc`
     
    configs = generateConfigNames(factors, levels)
    vecFiles = Hash.new
    configs.each{|c|
      vecFiles[c] = Dir["*_#{c}*.vec"].size
    }
    sorted = vecFiles.sort{|a,b| a[1] <=> b[1]}
    p sorted
    sorted.each{|s|
      puts s[0] + " generated "  + s[1].to_s
    }
  end
    
  #
  # Launches the application
  #  MultiConfigGenerator.new.run
  #
  def run

    def netName(modulename)
      netname = modulename.sub(/^([A-Z]+?[a-z]*?[A-Z]+)/) {|first|
        last = first.length-2
        #this is not same as next line !!!!! :[] why
        #first[0..last].downcase!
        first[0..last] = first[0..last].downcase
        first
      }
      netname = netname + "Net"
    end

    #returns networksNames and filenames (module name of network in ned)
    def formFilenames(configs, basemodulename)
      filenames = configs.map{|i|
        basemodulename + DELIM + i
      }
      networkNames = filenames.map{|n|
        netName(n)
      }
      [networkNames, filenames]
    end

    basemodname = ARGV.shift
    basenetname = netName(basemodname)
    cwd = `pwd`.chomp
    #opplink will create symbolic link to INET executabe with same name as
    #directory
    exename = `basename #{cwd}`.chomp
    runcount = @runCount.nil??10:@runCount
    
    factors, levels = readConfigs
    processActions(factors, levels)

    if @check
      checkRunResults(factors, levels, @runCount, basemodname)
      exit
    end
    
    final = generateConfigNames(factors, levels)
    networkNames, filenames = formFilenames(final, basemodname)
    
    nedfileOrig = IO.read(basemodname+".ned")
    inifile = IO.readlines(basemodname+".ini")
    
    File.open("jobs.txt","w"){|jobfile|
      filenames.each_with_index {|filename, fileIndex|
      
        #Add some actions at runtime i.e. as we can determine filenames easily here
        
        #write ini files (line oriented)
        File.open(filename + ".ini", "w"){ |writeIniFile|
          inifile.each_with_index {|line2,lineIndex|
            #change network name in ini file to config specific name otherwise
            #will run diff ned network settings!!
            line = line2.dup
            ReplaceStringAction.new(:ini, basenetname, networkNames[fileIndex]).apply(line)
            
            SetAction.new(:runtime, %|IPv6routingFile|,  %|xmldoc("#{filename}.xml")|).apply(line)

            SetAction.new(:ini, %|preload-ned-files|, %|@../../../nedfiles.lst #{filename}.ned|).apply(line)

            # {{{ Pass a block into custom fn with this in it so we can do one for ini and one for xmlfile
            
            configLevels = final[fileIndex].split(DELIM)
            configLevels.each_with_index do |l,idx|
              me = factors[idx]

              if @actions.include? me
                if not @actions[me].class == Hash
                  iniactions = @actions[me].select {|item|
                    item.symbol == :ini
                  }
                else                  
                  iniactions = @actions[me][l].select{|items|
                    items.symbol == :ini
                  }
                end
                for a in iniactions do
                  #apply file actions only on lineIndex == 0
                  a.apply(line, lineIndex, inifile, l)
                end
                next #skip level specific actions
              end
              
              #do level specific actions
              next if not @actions.include?(l)
              iniactions = @actions[l].select{|items|
                items.symbol == :ini
              }
              for a in iniactions do
                a.apply(line, lineIndex, inifile, l)
              end
            end
            
            # }}}
            raise "inifile #{basemodname}.ini contains a conflicting network directive at line #{lineIndex}" if line =~ /^network/                            
            writeIniFile.puts line
          } #end read inifile line by line
          
          # {{{ specify ned network to use, appends runs at end and put job line #
          
          writeIniFile.puts "[General]"
          writeIniFile.puts "network = #{networkNames[fileIndex]}"
     
          1.upto(runcount) do |runIndex|
            
            vectorfile = filename + DELIM + runIndex.to_s + ".vec"         
            scalarfile = filename + DELIM + runIndex.to_s + ".sca"
            
            writeIniFile.puts "[Run #{runIndex}]"
            writeIniFile.puts "output-vector-file = #{vectorfile}"           
            writeIniFile.puts "output-scalar-file = #{scalarfile}"

            #write distjobs file
            jobfile.puts "./#{exename} -f #{filename}.ini -r #{runIndex}"
          end
          
          # }}}
          
        } #end writeIniFile
        
        nedfile = nedfileOrig.dup
        
        #write ned files (stream oriented)
        File.open(filename + ".ned", "w"){|writeNedFile|
          line = lineIndex = nil
          
          # {{{ ned block
          
          configLevels = final[fileIndex].split(DELIM)
          configLevels.each_with_index do |l,idx|
            me = factors[idx]
            if @actions.include? me
              if not @actions[me].class == Hash
                iniactions = @actions[me].select {|item|
                  item.symbol == :ned
                }
              else                  
                iniactions = @actions[me][l].select{|items|
                  items.symbol == :ned
                }
              end
              for a in iniactions do
                #apply file actions only on lineIndex == 0
                a.apply(line, lineIndex, nedfile, l)
              end
              next #skip level specific actions
            end
            
            #do level specific actions
            next if not @actions.include?(l)
            iniactions = @actions[l].select{|items|
              items.symbol == :ned
            }
            for a in iniactions do
              a.apply(line, lineIndex, nedfile, l)
            end
          end
          
          # }}}
          
          ReplaceStringAction.new(:ned, basemodname, filenames[fileIndex]).apply(line, lineIndex, nedfile, nil)
          ReplaceStringAction.new(:ned, basenetname, networkNames[fileIndex]).apply(line, lineIndex, nedfile, nil)
          
          writeNedFile.print nedfile
        }
        
        scheme = final[fileIndex].split(DELIM)[factors.index("scheme")]
        if (scheme == "hmip")
          xmllines = IO.readlines(basemodname+scheme+".xml")
        else
          xmllines = IO.readlines(basemodname+".xml")
        end
        #write xml files
        File.open(filename + ".xml", "w"){|writeXmlFile|
          xmllines.each_with_index {|line, lineIndex|
            xmlfile = nil
            # {{{ xml block
            
            configLevels = final[fileIndex].split(DELIM)
            configLevels.each_with_index do |l,idx|
              me = factors[idx]                         
              if @actions.include? me
                if not @actions[me].class == Hash
                  iniactions = @actions[me].select {|item|
                    item.symbol == :xml
                  }
                else                  
                  iniactions = @actions[me][l].select{|items|
                    items.symbol == :xml
                  }
                end
                for a in iniactions do
                  #apply file actions only on lineIndex == 0
                  a.apply(line, lineIndex, xmlfile, l)
                end
                next #skip level specific actions
              end
              
              #do level specific actions
              next if not @actions.include?(l)
              iniactions = @actions[l].select{|items|
                items.symbol == :xml
              }
              for a in iniactions do
                a.apply(line, lineIndex, xmlfile, l)
              end
            end
            
            # }}}            
            writeXmlFile.puts line
          }# end read xmllines line by line
        } #end writeXmlFile
      }# end each config filename
    }
    
    
  end#run
  
end#MultiConfigGenerator


if $0 == __FILE__ then
  $app = MultiConfigGenerator.new  
  if not $test    
    $app.run 
    exit 
  end

end


# {{{ Unit test #

##Unit test for this class/module
require 'test/unit'

class TC_MultiConfigGenerator < Test::Unit::TestCase
  def test_ToggleActionXML
    line = 'AdvSendAdvertisements="on" HMIPAdvMAP="on" AdvHomeAgent="on">'
    a = ToggleAction.new(:xml, "AdvHomeAgent", false)
    assert_equal("AdvSendAdvertisements=\"on\" HMIPAdvMAP=\"on\" AdvHomeAgent=\"off\">",
                 a.apply(line),
                 "value of xml param AdvHomeAgent should have changed to false")
  end
  def test_ToggleActionIni
    a = ToggleAction.new(:ini, "l2up", false)
    assert_equal("**.l2upTrigger=true",
                 a.apply("**.l2upTrigger=true"),
                 "parameter l2upTrigger should not change to false as we modify l2up only")
    assert_equal("**.l2up=false",
                 a.apply("**.l2up=true"),
                 "parameter matches so value should change to false")
    
  end
  def test_quoteValue
    assert_equal('"me"',
                 Action.new(nil, nil, nil).quoteValue("me"),
                 "quotes should surround value passed int")
  end
  def test_setAction
    line = %|<interface name="eth1" AdvSendAdvertisements="on" HMIPAdvMAP="on" AdvHomeAgent="on" MaxFastRAS="10" MIPv6MaxRtrAdvInterval="0.12" MIPv6MinRtrAdvInterval="0.08" >|
    assert_equal(%|<interface name="eth1" AdvSendAdvertisements="on" HMIPAdvMAP="on" AdvHomeAgent="on" MaxFastRAS="20" MIPv6MaxRtrAdvInterval="0.12" MIPv6MinRtrAdvInterval="0.08" >|,
                 SetAction.new(:xml, "MaxFastRAS", 20).apply(line),
                 "value of MaxFastRAS should be 20 now")
    
    iniline = "**.networkInterface.queueSize = 10"
    assert_equal("**.networkInterface.queueSize=100",
                 SetAction.new(:ini, "queueSize", 100).apply(iniline),
                 "queueSize should now be 100")
    
    xmlline = %|     <interface name="wlan0" HostDupAddrDetectTransmits="1" HostMaxRtrSolDelay="0">|
    assert_equal(%|     <interface name="wlan0" HostDupAddrDetectTransmits="1" HostMaxRtrSolDelay="1">|,
                 SetAction.new(:xml, "HostMaxRtrSolDelay", 1).apply(xmlline), 
                 "be careful of missing > in xml docs")
    
    ini = %|ehCompNet.*.IPv6routingFile =xmldoc("EHComphmip.xml")|
    assert_equal(%|ehCompNet.*.IPv6routingFile=xmldoc("EHComphmip_50_2_y.xml")|,
                 SetAction.new(:runtime, %|IPv6routingFile|,  %|xmldoc("EHComphmip_50_2_y.xml")|).apply(ini),
                 "does not preserve space around the = sign so better not put them in base doc")
    
    ini = %|preload-ned-files=@../../../nedfiles.lst *.ned|
    assert_equal(%|preload-ned-files=@../../../nedfiles.lst EHComp_eh_500_50_n.ned|,
                 SetAction.new(:ini, %|preload-ned-files|, %|@../../../nedfiles.lst EHComp_eh_500_50_n.ned|).apply(ini),
                 "")
    
  end
  
  def test_SetFactorCableAction
    lines = <<END
import
    "WorldProcessor",
    "Router6",
    "UDPNode",
    "WorldProcessor",
    "WirelessAccessPoint",
    "WirelessMobileNode";




channel EHCompIntranetCable
    datarate 100e6;
    delay 10e-3;
endchannel

channel EHCompInternetCable
    datarate 100e6;
    delay 100e-3;
endchannel

channel EHCompEdgeCable
    datarate 100e6;
    delay 1e-4;
endchannel 
END
    expected = "import\n    \"WorldProcessor\",\n    \"Router6\",\n    \"UDPNode\",\n    \"WorldProcessor\",\n    \"WirelessAccessPoint\",\n    \"WirelessMobileNode\";\n\n\n\n\nchannel EHCompIntranetCable\n    datarate 100e6;\n    delay 10e-3;\nendchannel\n\nchannel EHCompInternetCable\n    datarate 100e6;\n    delay 50e-3;\nendchannel\n\nchannel EHCompEdgeCable\n    datarate 100e6;\n    delay 1e-4;\nendchannel \n"    
    assert_equal(expected,
                 SetFactorChannelAction.new(:ned, "EHCompInternetCable", "delay", @levels["dnet"]).apply(nil, nil, lines, 50),
                 "better be equal with multiline regexp")
    
    expected = "import\n    \"WorldProcessor\",\n    \"Router6\",\n    \"UDPNode\",\n    \"WorldProcessor\",\n    \"WirelessAccessPoint\",\n    \"WirelessMobileNode\";\n\n\n\n\nchannel EHCompIntranetCable\n    datarate 100e6;\n    delay 345e-3;\nendchannel\n\nchannel EHCompInternetCable\n    datarate 100e6;\n    delay 50e-3;\nendchannel\n\nchannel EHCompEdgeCable\n    datarate 100e6;\n    delay 1e-4;\nendchannel \n"
    assert_equal(expected,
                 #why can't we have named arguments :(
                 #                 SetFactorChannelAction.new(:ned, "EHCompIntranetCable", "delay", @levels["dmap"]).apply(file = lines, level = 345),
                 SetFactorChannelAction.new(:ned, "EHCompIntranetCable", "delay", @levels["dmap"]).apply(nil, nil, file = lines, level = 345),
                 "better be equal with multiline regexp")                 
  end
  
  def test_generateConfigNames
    length = 0
    for f in @factors      
      size = @levels[f].size
      if length == 0
        length = size
      else
        length = size * length
      end
    end
    
    assert_equal(length,
                 @app.generateConfigNames(@factors, @levels).size,
                 "should be equal as number of levels of each factor multiplied together")
    
    
    levels={}
    factors = %w|x y z|
    levels[factors[0]] = %w| a b c|
    levels[factors[1]] = %w| alpha beta|
    levels[factors[2]] = %w| s t u v|
    
    expected = ["a_alpha_s", "a_alpha_t", "a_alpha_u", "a_alpha_v", "a_beta_s", "a_beta_t", "a_beta_u", "a_beta_v", "b_alpha_s", "b_alpha_t", "b_alpha_u", "b_alpha_v", "b_beta_s", "b_beta_t", "b_beta_u", "b_beta_v", "c_alpha_s", "c_alpha_t", "c_alpha_u", "c_alpha_v", "c_beta_s", "c_beta_t", "c_beta_u", "c_beta_v"]
    assert_equal(expected,
                 @app.generateConfigNames(factors, levels),
                 "should be equal as number of levels of each factor multiplied together")
    
    assert_equal(24,
                 @app.configCount(factors, levels),
                 "should be equal as I calculated manually in head")
  end
  
  def test_ReplaceStringAction
    sampleNed = <<END
channel EHCompIntranetCable
    datarate 100e6;
    delay 10e-3;
endchannel

channel EHCompInternetCable
    datarate 100e6;
    delay 100e-3;
endchannel

channel EHCompEdgeCable
    datarate 100e6;
    delay 1e-4;
endchannel

module EHComp
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=264,31;i=bwgen_s";
        mn: MobileNode;
            parameters:
            gatesizes:
                wlin[1],
                wlout[1];
            display: "p=40,92;i=laptop3";
        cn: UDPNode;
            parameters:
            //Complains about invalid type D for this
            //                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=407,41;i=pc";
        ha: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=72,56;i=router";
        ar4.out[1] --> apd.in[0];
        ar4.in[1] <-- apd.out[0];

        ar.out[2] --> EHCompEdgeCable --> ar2.in[2];
        ar.in[2] <-- EHCompEdgeCable <-- ar2.out[2];

        ar2.out[3] --> EHCompEdgeCable --> ar3.in[2];
        ar2.in[3] <-- EHCompEdgeCable <-- ar3.out[2];

        ar3.out[3] --> EHCompEdgeCable --> ar4.in[2];
        ar3.in[3] <-- EHCompEdgeCable <-- ar4.out[2];
    display: "p=10,10;b=674,426";
endmodule

network ehCompNet : EHComp
endnetwork
END

    expected = <<END
channel EHCompeh_500_20_nIntranetCable
    datarate 100e6;
    delay 10e-3;
endchannel

channel EHCompeh_500_20_nInternetCable
    datarate 100e6;
    delay 100e-3;
endchannel

channel EHCompeh_500_20_nEdgeCable
    datarate 100e6;
    delay 1e-4;
endchannel

module EHCompeh_500_20_n
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=264,31;i=bwgen_s";
        mn: MobileNode;
            parameters:
            gatesizes:
                wlin[1],
                wlout[1];
            display: "p=40,92;i=laptop3";
        cn: UDPNode;
            parameters:
            //Complains about invalid type D for this
            //                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=407,41;i=pc";
        ha: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=72,56;i=router";
        ar4.out[1] --> apd.in[0];
        ar4.in[1] <-- apd.out[0];

        ar.out[2] --> EHCompeh_500_20_nEdgeCable --> ar2.in[2];
        ar.in[2] <-- EHCompeh_500_20_nEdgeCable <-- ar2.out[2];

        ar2.out[3] --> EHCompeh_500_20_nEdgeCable --> ar3.in[2];
        ar2.in[3] <-- EHCompeh_500_20_nEdgeCable <-- ar3.out[2];

        ar3.out[3] --> EHCompeh_500_20_nEdgeCable --> ar4.in[2];
        ar3.in[3] <-- EHCompeh_500_20_nEdgeCable <-- ar4.out[2];
    display: "p=10,10;b=674,426";
endmodule

network ehCompNet : EHCompeh_500_20_n
endnetwork
END
    ReplaceStringAction.new(:ned, "EHComp", "EHCompeh_500_20_n").apply(nil, nil, sampleNed, nil)
    #    print sampleNed
    assert_equal(expected, 
                 sampleNed,
                 "")
    
  end
  
  def setup
    @app = MultiConfigGenerator.new
    @factors = ["scheme", "dnet", "dmap", "ar"]
    
    @levels = {}
    @levels[@factors[0]] = ["hmip", "mip", "eh"]
    @levels[@factors[1]] = ["20", 50, "200", "500"]
    @levels[@factors[2]] = ["5ms", "10ms", "20ms", 345]
    @levels[@factors[3]] = ["y", "n"]
  end
  
  def teardown
  
  end
  
end #test class

if $test
  ##Fix Ruby debugger to allow debugging of test code
  require 'test/unit/ui/console/testrunner'
  Test::Unit::UI::Console::TestRunner.run(TC_MultiConfigGenerator)
end

# }}}
