#! /usr/bin/env ruby
#
#  Author: Johnny Lai
#  Copyright (c) 2005 Johnny Lai
#
# =DESCRIPTION
# ALPHA generate wireless scenario with static routing
# A predefined graph topology is passed in. Then certain nodes are designated as
#Core Routers (CR), Access Routers (AR), and Host. 3 APs are added to each
#AR. see gentopology.rb.  The required no. of MNs are created and assigned
#random positions according to the size of the world calculated from the manual
#AP positioning array (calculateAPPosition)
#
# The topology is traversed to build up a nexthop map. Its traversed again to
# create:
#     ned file with actual connections, manual AP positioning
#     xml file for routing/address assignment/mip config/movement patterns
#     ini file for actual iface types to be instantiated

# =REVISION HISTORY
#  INI 2005-02-01 Changes
#

#Requires rgl and stream from rgl.rubyforge.net to be placed into load path
$LOAD_PATH<<`echo $HOME`.chomp + "/lib/ruby"
$LOAD_PATH<<File.dirname(__FILE__)
require 'optparse'
require 'pp'

$unittest = false
$noINET = false

#
# This class generates all the XML/ini/ned file from a hard coded graph topology.
# Many things are hard coded including many implicit assumptions.
#
class NedFile
  VERSION       = "$Revision: 1.12 $"
  REVISION_DATE = "$Date: 2005/11/30 05:28:18 $"
  AUTHOR        = "Johnny Lai"
  @@IPv6SuiteWithINET = "../../.."
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

  attr_accessor :modulename, :author, :edges

  def initialize(modulename, author = nil)
    @debug    = false
    @verbose  = false
    @quit     = false
    @mnCount  = 50
    @apWidth  = 100
    @runCount = 20
    @mip6 = true
    @hmip = false
    @eh = false
    @ping = false
    @video = false
    @netmaskunique = Array.new
    @configs = false
    @mapprefixunique = Array.new

    self.modulename = modulename

    get_options

    self.author = author ? author: "Johnny Lai"
    self.edges = Array.new

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

      opt.on("-x", "parse arguments and show Usage") {|@quit|}

      opt.on("--mncount", "-m [Integer]", OptionParser::DecimalInteger, "Number of MNs to generate"){|@mnCount|}
      opt.on("--width", "-w [Integer]", OptionParser::DecimalInteger, "Distance multiplier for APs"){|@apWidth|}

      opt.on("--runcount", "-r [Integer]", OptionParser::DecimalInteger, "Number of runs to generate in inifile"){|@runCount|}

      opt.on("--mip6", "-M", "Add MIPv6 options to MN in xml file"){|@mip6|}
      opt.on("--hmip", "-H", "Add HMIP scenario options to CR"){|@hmip|}
      opt.on("--eh", "-E", "Add EH scenario options to ARs"){|@eh|}

      opt.on("--ping", "-p", "Add ping traffic sources for all MNs"){|@ping|}

      opt.on("--video", "-V", "Add video traffic sources for all MNs"){|@video|}

      opt.on("--name", "-n [String]", "Module name for network") {|@modulename| }

#      opt.on("--configname", "-c [String]", "Configuration name for this batch of runs and resulting output files"){|@configname|}
      opt.on("--configsgen", "-c","Generate multiple ini/ned/xml files for specific hardcoded configs"){|@configs|}

      opt.separator ""
      opt.separator "Common options:"

      opt.on("--doc=DIRECTORY", String, "Output rdoc (Ruby HTML documentation) into directory"){|dir|
        system("rdoc -a --webcvs=\"http://localhost/cgi-bin/viewcvs.cgi/\" --tab-width=2 --diagram --inline-source -N -o #{dir} #{__FILE__}")
      }

      opt.on("--doc=DIRECTORY", String, "Output rdoc (Ruby HTML documentation) into directory"){|dir|
        system("rdoc -o #{dir} #{__FILE__}")
      }

      opt.on("--verbose", "-v", "print intermediate steps to STDOUT"){|@verbose|}

      opt.on("--debug", "-X", "print debugging info to STDOUT"){|@debug|}

      opt.on("--unittest", "-u", "Run unit tests"){|$unittest|}

      opt.on_tail("-h", "--help", "What you see right now") do
        puts opt
        exit
      end

      opt.on_tail("--version", "Show version") do
        #puts OptionParser::Version.join('.')
        puts version
        exit
      end

      opt.on_tail("By default ... ",
                  "Subset ... ")
      #Samples end

      opt.parse!
    } or  exit(1);

    if @quit
      pp self
      (print ARGV.options; exit)
    end

  end

  ZERO = ":0000:0000:0000:0000"

  #
  # Creates the graph, writes out xml config
  #
  #
  def run
    #read graph and draw from here
    #require or C-x C-l is good for syntax checking
    require "gentopology.rb"

    adjacency=gets.chomp.split(/[,\s]+/)
    dg=RGL::AdjacencyGraph[3,1, 3,2, 3,4]
    while adjacency.length > 0
    	    foo = adjacency.slice!(0,2)
            dg.add_edge(foo[0].to_i + 4, foo[1].to_i + 4)
    end

    g, @vs = genEHTopology(dg)
    @pos, xcoords, ycoords = calculateAPPositions(@apWidth)
    open("#{modulename}.ned","w") { |x|
    	x.puts generateNed(g, @mnCount,
		[xcoords.max+xcoords.min, ycoords.max+ycoords.min])
    }
    xdoc = xmlHeader
    root = xdoc.root
    global = root.add_element("global")
if $noINET
    global.add_element("WirelessEtherInfo").add_element("WEInfo", Hash["WETxPower", "1.5", "WEAuthenticationTimeout",
                                                        "3000", "WEAssociationTimeout", "3000"])
end
    global.add_element("ObjectMovement").add_element("RandomMovement",
      Hash["RWMinX", xcoords.min.to_s, "RWMaxX", xcoords.max.to_s,
          "RWMinY", ycoords.min.to_s, "RWMaxY", ycoords.max.to_s ,
          *%w|RWMoveInterval 1 RWMinSpeed 3 RWMaxSpeed 5 RWDistance 100 RWPauseTime 1 RWNodeName all RWStartTime 3|])
    #MN entries
    1.upto(@mnCount) do |i|
  #eagerHandover (hands over when different router addr detected from current/primaryHA)
  	hash = Hash["node", "mn#{i}", * %w|mobileIPv6Support on mobileIPv6Role MobileNode routeOptimisation on eagerHandover on |]
	hash["edgeHandoverType"] = "Timed" if @eh
	hash["hierarchicalMIPv6Support"] = "on" if @hmip or @eh

        le = root.add_element("local", hash)
      
      le.add_element("interface", Hash[* %w|name wlan0 HostDupAddrDetectTransmits 1 MaxConsecMissRtrAdv 3|])
    end if @mip6

    ha = g.vertices.detect{|i| i.kind_of? HA}
    haIface = ha.ifaces[0]
    #assuming HA is always at vs[1]
    throw "ifaces should match using both methods to find HA" if not haIface.address == @vs[1].ifaces[0].address
    #everything else
    g.each{|start|
      next if start.kind_of? AP
      le = root.add_element("local", Hash["node", "#{start.nodeName}", * %w|mobileIPv6Support on|])

      le.add_attributes Hash[* %w|routePackets on| ] if start.kind_of? Router

      if start.kind_of? AR or start.kind_of? HA
        le.add_attributes Hash[* %w|mobileIPv6Role HomeAgent |]
        le.add_attributes Hash[* %w|map on hierarchicalMIPv6Support on|] if @eh
      end

      if start.kind_of? CR and @hmip
        le.add_attributes Hash[* %w|hierarchicalMIPv6Support on map on mobileIPv6Role HomeAgent |]
      end 

      ifaces = %w|eth0 ppp0|
      sameMapPrefixPermitted = false
      start.ifaces.each_index {|index|
        iface = start.ifaces[index]
        ie = le.add_element("interface")
        ifname = iface.edge.ifaceType == "EtherModule" ? ifaces[0] : ifaces[1]
        ifname[ifname.length-1] = index.to_s if not $noINET

        ie.add_attributes Hash["name", "#{ifname}"]
        iface.ifname = ifname.dup
        ie.add_element("inetAddr").text = iface.address

        if start.kind_of? Router
          #HA needs to have an adv prefix list otherwise
          #it will reject BUs as it checks hoa against prefix when using assigned HA
          ie.add_attributes Hash[* %w|AdvSendAdvertisements on|] if not iface.remoteNode.kind_of? Router or @hmip or start.kind_of? HA
          #AdvHomeAgent Needs to be on for all ARs if MNs are to take first AR seen as HA
          ie.add_attributes Hash[* %w|AdvHomeAgent on HMIPAdvMAP on |] if (@hmip and start.kind_of? CR) or @eh
            
          ie.add_attributes Hash[* %w|MIPv6MaxRtrAdvInterval 0.12 MIPv6MinRtrAdvInterval 0.08 MaxFastRAS 10|] if iface.remoteNode.kind_of? AP
        
            #We need all ARs to adv. as HA as MN do not have a preconfigured
            #HA. So first HA they see is primary HA will conflict with existing
            #C++ assumption that EH not active at home base


            #Fixed nodes already have addresses assigned acc. to netmask via
            #this script so no need for routers to advertise unless of course
            #hub is used to connect to an AP too

          #HA needs to have an adv prefix list otherwise
          #it will reject BUs as it checks hoa against prefix when using assigned HA
          if not iface.remoteNode.kind_of? Router or @hmip or start.kind_of? HA
            aple = ie.add_element("AdvPrefixList")
            aple.add_element("AdvPrefix", Hash[*%w|AdvOnLinkFlag on AdvRtrAddrFlag on| ] ).text = "#{iface.address}/64"
          end

          if (@eh and not iface.remoteNode.kind_of? Router) or (@hmip and start.kind_of? CR)
            #assuming HA node occurs before map node and HA cannot be a MAP for static assigned homeagent case
            mapIface = start.ifaces.detect {|i| not i.remoteNode.kind_of? AP}
            
            if (not sameMapPrefixPermitted) then
              print "\n#{@mapprefixunique.join("\n")}"

              #When map prefix are not unique it is possible in EH mode to
              #actually have both coa and hoa to have same prefix and so MAP
              #will try to deregister a non existant BCE
         
              while @mapprefixunique.include?(prefix(mapIface.address)) do
                prefix = permutePrefix(prefix(mapIface.address))
                mapIface.address = prefix + ADDRDELIM + suffix(mapIface.address)
              end
              @mapprefixunique<<prefix(mapIface.address) 
              sameMapPrefixPermitted = true
            end

            while prefixMatches(mapIface.address, haIface.address) do
                while @mapprefixunique.include?(prefix(mapIface.address)) do
                  prefix = permutePrefix(prefix(mapIface.address))
                  mapIface.address = prefix + ADDRDELIM + suffix(mapIface.address)
                end
              if not prefixMatches(mapIface.address, haIface.address) then
                @mapprefixunique<<prefix(mapIface.address) 
                break
              end
            end
            ie.add_element("AdvMAPList").add_element("AdvMAPEntry").text = "#{mapIface.address}/64"
#"#{iface.address}"
          end #end eh and hmip+CR
	end #end Router

        #AdvRtrAddrFlag only needed for AP facing ifaces. in fact don't need
        #global addresses assigned to internal router but we'll keep program
        #simple

        ifname.succ!
      } # end each iface
      sameMapPrefixPermitted = false

      next if not start.kind_of? Router
      nhops, bfs, testvalidate = routingTable(g, start )

      re = le.add_element("route")
      self.edges.each{|e|
        u,v = e.source, e.target

        #v/u shouldn't matter? as nexthop should find the nexthop of either
        #which should be the same assuming u/v lie on the shortest path back to
        #node.
        nexthop = nhops[v]

        #It only matters if one of the nodes is in fact us so we actually want
        #to know which iface on us links to that node

        #don't know why this works
        #        nexthop = u == node ?  @nhops[v] : @nhops[u]

        nexthop = v if u == start
        nexthop = u if v == start

        iface = nil
        start.ifaces.each{|iface|
          break if iface.remoteNode == nexthop
        }
        ree = re.add_element("routeEntry", Hash["routeIface", "#{iface.ifname}", "routeDestination", "#{e.netmask}#{ZERO}/64"])
        ree.attributes["routeNextHop"] = "#{nexthop.ifaces[iface.remoteIfIndex].address}" if nexthop.kind_of? Router
        #xml
        #endxmlend
      } #end @n.edges
    } #end each node

    File.open("#{modulename}.xml", "w") do |f|
      xdoc.write($stdout, 1) if @debug
      xdoc.write(f, 0)
    end
    File.open("#{modulename}.ini", "w"){|f| f << generateIni }

#copy ini file to configname
#for each config append this stuff and perhaps make xml config dependent too
#and gen diff ones from here or just copy modulename.xml and search replace
if @configs
  var3 = ""
  ["sait-hmip", "sait-eh", "sait-mip"].each{ |configname|
#    system("cp -p #{modulename}.ini #{configname}.ini")
    var2 = ""
    1.upto(@runCount) do |i|
      vectorfile = configname + i.to_s + ".vec"
      scalarfile = configname + i.to_s + ".sca"
      var2 += <<EOF

[Run #{i}]
output-vector-file = #{vectorfile}
output-scalar-file = #{scalarfile}

EOF
      var3 += <<EOF
./HMIPv6Network -f #{configname}.ini -r #{i}
EOF
    end 

    defaultini = "#{@@IPv6SuiteWithINET}/Etc/default.ini"
    #defaultini.insert(0, "../") if not $noINET

    var2 += <<EOF
include #{defaultini}
EOF
#    File.open("#{configname}.ini", "a"){|f| f << var2}
  }
#  File.open("run.txt", "w"){|f| f<< var3}
end #configs

  end#run

  def calculateAPPositions(width = 100, offset = width/2)

   pos=gets.chomp.split(/[,\s]+/).map {|x| (x.to_f * width).to_i}

  xcoords, ycoords = breakCoords(pos)
  xdisp = xcoords.min.abs
  ydisp = ycoords.min.abs

  pos.each_index do |i|
    if i % 2 == 0 then
      pos[i] = pos[i] + xdisp + offset
    else
      pos[i] = pos[i] + ydisp + offset
    end
  end

  xcoords, ycoords = breakCoords(pos)

  [pos, xcoords, ycoords]
  end

#Break array into an array of x and y coords
def breakCoords(array)
  xcoords = Array.new
  ycoords = Array.new
  array.each_index do |i|
    if i % 2 == 0 then
      xcoords << array[i]
    else
      ycoords << array[i]
    end
  end
  [xcoords, ycoords]
end

  require 'rexml/document'
  include REXML

  def xmlHeader
    dtd = "#{@@IPv6SuiteWithINET}/Etc/netconf2.dtd"
   #dtd.insert(0, "../") if not $noINET
  #create xmldoc header
    xmldecl = <<EOF
<?xml version="1.0" encoding="iso-8859-1"?>
<!DOCTYPE netconf SYSTEM "#{dtd}">
<netconf debugChannel="#{modulename}.log:rcfile:MobileMove::notice:custom:Ping6:Statistic">
</netconf>
EOF
    xmldoc = Document.new xmldecl
  end

  def generateIni
#header
    var = <<EOF
[General]

network = #{netName}
      preload-ned-files=*.ned @#{@@IPv6SuiteWithINET}/nedfiles.lst

total-stack-kb=17535
ini-warnings = no
warnings = yes
sim-time-limit = 202
debug-on-errors = true

[Cmdenv]
express-mode = yes
module-messages = no
event-banners = no

[Tkenv]
breakpoints-enabled = no
animation-speed = 1.0
EOF
    #runs
    var += <<EOF
[Parameters]

EOF
  cns = @vs.select {|k,v| v.kind_of? Host}
  pingAppName = "ping6App"
  pingAppName = "pingApp" if not $noINET
  #select returns an array of matches which in this case is an array of [k,v] (key /value or number/node obj)
  if false 
    #test cn send to a fixed router interface that is an AR
    dest = @vs[9]
    cns.each do |a|
      v = a[1]
      ping = "#{netName}.#{v.nodeName}.#{pingAppName}."
        var += ping + "startTime=" + 5.to_s
        var += "\n"
        var += ping + "deadline=" + 70.to_s
        var += "\n"
        throw "not an AR #{dest}" if not dest.kind_of? AR
        destAddr = dest.ifaces.detect{|i| i.edge.ifaceType =~ /eth/i }.address
        var += ping + "destination=" + %|"#{destAddr}"|
        var += "\n"
        var += ping + "interval=" + 1.to_s
        var += "\n"
    end
  else
    #for mn to send traffic to cns
    1.upto(@mnCount) do |i|
      dest=cns[rand(cns.size) - 1][1]
      destAddr = dest.ifaces.detect{|iface| iface.edge.ifaceType !~ /eth/i }.address
      destAddr = dest.nodeName if not $noINET
      
      if @ping
        ping = "#{netName}.mn#{i}.#{pingAppName}."
        var = iniPingTraffic(var, ping, destAddr, 10, 0.5)
      end

      var << "\n"

      if @video
        video = "#{netName}.mn#{i}."
        var = iniVideoTraffic(var, video, destAddr, 8)
      end
    end

    var << "\n"
    var += ";;Videostream server"
    var << "\n"
    cns.each do |a|
      cn = a[1]
      vidserv = "#{netName}.#{cn.nodeName}.udpApps[0]."
      vsapp = vidserv + "udpApp."
      var += "#{netName}.#{cn.nodeName}.numOfUDPApps = 1"
      var << "\n"
      var += vidserv + %|UDPAppName = "UDPVideoStreamSvr"|
      var << "\n"
      var += vsapp + "server = true"
      var << "\n"
      var += vsapp + "videoSize = 1e8"
      var << "\n"
      var += vsapp + "UDPPort = 3088"
      var << "\n"
    end
    
    var += <<EOF
;general udp characteristics
#{netName}.**.minWaitInt = .01
#{netName}.**.maxWaitInt = .01
#{netName}.**.minPacketLen = 1000
#{netName}.**.maxPacketLen = 1000 ; should not be bigger than 16000 bytes

;general ping characteristics
;#{netName}.mn**.networkLayer.proc.ICMP.icmpv6Core.icmpRecordRequests = true
;#{netName}.mn**.networkLayer.proc.ICMP.icmpv6Core.replyToICMPRequests = false
;#{netName}.mn**.networkLayer.proc.ICMP.icmpv6Core.icmpRecordStart = 155
;#{netName}.cn**.ping6App.interval = 0.01
;#{netName}.cn**.ping6App.packetSize = 1000

EOF

 end
  xmldoc = %|"#{modulename}.xml"|
    if not $noINET
      xmldoc.insert(0, "xmldoc(") 
      xmldoc = xmldoc+")"
    end
    var += <<EOF

#{netName}.**.IPv6routingFile = #{xmldoc}
#{netName}.mn**.linkLayers[*].NWIName="WirelessEtherModule"
EOF

if not $noINET
  var += <<EOF
#{netName}.**.networkInterface.authenticationTimeout = 3000
#{netName}.**.networkInterface.associationTimeout = 3000
#{netName}.**.networkInterface.txPower = 1.5
#{netName}.**.networkInterface.linkUpTrigger = true
#{netName}.**.networkInterface.linkDownTrigger = true
#{netName}.**.networkLayer.proc.mobility.homeAgent = "#{@vs[1].ifaces[0].address}"
;;Can't use name yet as resolver complains about unable to resolve at initialise
;#{netName}.**.networkLayer.proc.mobility.homeAgent = "ha1"

EOF
end

  #Hardcoded channels
  channels=gets.chomp.split(/[,\s]+/).map {|x| x.to_i}
              
  chidx = 0
  1.upto(@vs.size) do |i|
    v = @vs[i]
    next unless v.kind_of? AP
    throw "Unable to use hard coded channels array for bigger sim " if chidx == channels.length
    var += <<EOF
#{netName}.#{v.nodeName}.chann=#{channels[chidx]}
EOF
    chidx += 1
  end

    self.edges.each do |e|
      u,v = e.source, e.target
      if e.ifaceType =~ /eth/i
        unif = u.ifaces.detect {|i| i.edge == e }
        vnif = v.ifaces.detect {|i| i.edge == e }
        var += %|#{netName}.#{u.nodeName}.linkLayers[#{unif.ifIndex}].NWIName="EtherModule"\n|
        var += %|#{netName}.#{v.nodeName}.linkLayers[#{vnif.ifIndex}].NWIName="EtherModule"\n|

      end
    end

    var += <<EOF
#{netName}.*ap*.wirelessAccessPoint.networkInterface.nwiXmlConfig=xmldoc("#{modulename}.xml","netconf/global/WirelessEtherInfo/WEInfo")
#{netName}.mn*.linkLayers[0].networkInterface.nwiXmlConfig=xmldoc("#{modulename}.xml","netconf/global/WirelessEtherInfo/WEInfo")
;#{netName}.*.ds[*].networkInterface.nwiXmlConfig=xmldoc("#{modulename}.xml","netconf/global/WirelessEtherInfo/WEInfo")
#{netName}.mn*.mobilityManager.MobilityName="MobilityRandomWP"
#{netName}.mn*.mobilityManager.mobilityHandler.moveXmlConfig=xmldoc("#{modulename}.xml","netconf/global/ObjectMovement/RandomMovement")

;everything else is assumed to be PPP Interface
*.cn.linkLayers[0].NWIName="IPv6PPPInterface"
*.cr*.linkLayers[*].NWIName="IPv6PPPInterface"
*.ar*.linkLayers[*].NWIName="IPv6PPPInterface"
*.cn*.linkLayers[*].NWIName="IPv6PPPInterface"
*.ha*.linkLayers[0].NWIName="IPv6PPPInterface"
EOF

    if not @configs
      1.upto(@runCount) do |i|
    vectorfile = modulename + i.to_s + ".vec"
    scalarfile = modulename + i.to_s + ".sca"
    var += <<EOF

[Run #{i}]
output-vector-file = #{vectorfile}
output-scalar-file = #{scalarfile}

EOF
  end 

      defaultini = "#{@@IPv6SuiteWithINET}/Etc/default.ini"
      #defaultini.insert(0, "../") if not $noINET

var += <<EOF
include #{defaultini}
EOF
    end #not @configs
  end #generateIni

  def generateNed(*g)
    ned = generateHeader
    ned += generateImports
    ned += generateChannels
    ned += generateModule(*g)
  end

  #pingClients is usually something like
  #"#{netName}.mn#{i}.ping6App."
  def iniPingTraffic(inistr, pingClients, destAddr, starttime, interval = 1)
    inistr << "\n"
    inistr += pingClients + "startTime=" + "uniform(#{starttime}, #{starttime.succ})"
    inistr << "\n"
    if $noINET
      deadline = "deadline="
      destination = "destination="
    else
      deadline = "stopTime="
      destination = "destAddr="
    end
    inistr += pingClients + deadline + 700.to_s
    inistr << "\n"
    inistr += pingClients + destination + %|"#{destAddr}"|
    inistr << "\n"
    inistr += pingClients + "interval=" + interval.to_s
    inistr << "\n"
  end

  #video is like     
  #"#{netName}.mn#{i}."
  def iniVideoTraffic(iniStr, node, destAddr, starttime)
    iniStr += ";;Videostream"
    iniStr << "\n"
    video = node + "udpApps[0]."
    vidapp = video + "udpApp."
    iniStr += node + "numOfUDPApps = 1"
    iniStr << "\n"
    iniStr += vidapp + "UDPPort = 3088"
    iniStr << "\n"
    iniStr += video + %|UDPAppName = "UDPVideoStream"|
    iniStr << "\n"
    iniStr += vidapp + "startTime = " + "uniform(#{starttime}, #{starttime.succ})"
    iniStr << "\n"
    iniStr += vidapp + %|UDPServerAddress = "#{destAddr}"|
    iniStr << "\n"
  end

  def generateHeader(customMessage="Do Not Edit, this file is generated by #{File.basename(__FILE__)} at #{Time.now}.\n//Edit at your own peril.")
    var = <<EOF
// -*- Ned -*-
// Copyright (C) #{Time.now.year} #{author}
//
// #{customMessage}
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

//
// @file   #{modulename}.ned


EOF
  end

  def generateImports
    var = <<EOF
import
    "WorldProcessor",
    "Router6",
    "UDPNode",
    "WorldProcessor",
    "WirelessAccessPoint",
    "WirelessMobileNode";

EOF
  end

def generateChannels
  var = <<EOF
//Do not modify as it does nothing if used between ethernet interfaces. 
//Look at ethernet.cc for BANDWIDTH constant and alter that.
channel #{modulename}IntranetCable
    datarate 100e6
    delay 10e-3
endchannel

channel #{modulename}InternetCable
    datarate 1000e6
    delay 100e-3
endchannel

EOF
end

def generateModule(g, mnCount = 10, dim = [500, 500])
  var = <<EOF

module #{modulename}
  submodules:
    worldProcessor: WorldProcessor;
      display: "i=bwgen_s";
EOF

#for each MN require generate MN e.g.
  1.upto mnCount do |i|
    var += <<EOF
    mn#{i}: MobileNode;
    gatesizes:
        wlin[1],
        wlout[1];
    display: "p=#{rand(dim[0])},#{rand(dim[1])};i=laptop3";
EOF
  end


  #Only way to iterate in order as vs is a hash
  #TODO needs to be g.each if running test as @vs assigned in run method only
  index = 0
  index2 = 0
  1.upto(@vs.size) do |i|
    v = @vs[i]
    deg = g.out_degree(v)
    v.ifaces = Array.new(deg)
    var += <<EOF
    #{v.nodeName}: #{v.className};
        gatesizes:
            in[#{deg}],
            out[#{deg}];
EOF
    var += "        "
    if v.kind_of? AP
      raise "Index of #{index} is invalid as pos.size=#{@pos.size}" if index >= @pos.size
      var +=  %|display: "i=#{v.icon};p=#{@pos[index]},#{@pos[index + 1]}";|
      puts "AP #{v.name} is at #{@pos[index]},#{@pos[index + 1]}";
      index += 2
    elsif v.kind_of? AR
      var +=  %|display: "i=#{v.icon};p=#{@pos[index2]},#{@pos[index2 + 1]}";|
      puts "AR #{v.name} is at #{@pos[index2]},#{@pos[index2 + 1]}";
      index2 += 2
    else
     var +=  %|display: "i=#{v.icon}";|
    end
    var +="\n\n"
  end

#assign netmasks here

#Do network Topology connections
  var += " connections nocheck:\n"
#  g.edges.each{|e|
#    u, v = e[0], e[1]
  g.each_edge{ |u,v|
    nodea = u.nodeName
    nodeb = v.nodeName
    cable = (u.instance_of? Host or v.instance_of? Host or u.instance_of? HA or v.instance_of? HA) ? "InternetCable" : "IntranetCable"

    var += <<EOF
    #{nodea}.out[#{u.ifaces.index(nil)}] --> #{modulename}#{cable} --> #{nodeb}.in[#{v.ifaces.index(nil)}];
    #{nodea}.in[#{u.ifaces.index(nil)}] <-- #{modulename}#{cable} <-- #{nodeb}.out[#{v.ifaces.index(nil)}];
EOF

    #Store the array of edges since we use them inside ifaces.
    #edges actually creates the array of edges each time we call it :(
    e = Link.new(u, v)

    #conn between AP and routers can use Ethernet only. Between routers/fixed hosts PPP can be used
    e.ifaceType = (u.instance_of? AP or v.instance_of? AP) ? "EtherModule" : "IPv6PPPInterface"
    u.ifaces[u.ifaces.index(nil)] = ui = NetworkInterface.new(u, e)

    v.ifaces[v.ifaces.index(nil)] = vi = NetworkInterface.new(v, e)
    #assign addresses here base on netmask with random ifID
    e.netmask = createNetmask(e)

    ui.address = e.netmask + ADDRDELIM + createIfaceId(u, e)
    vi.address = e.netmask + ADDRDELIM + createIfaceId(v, e)

    self.edges << e
    #address is useless for APs
    #modify C++ code to do reverse interpolation from address to ifID
  }
var += <<EOF
  display: "p=1,1;b=#{dim[0]},#{dim[1]}";
endmodule

network #{netName}: #{modulename}
endnetwork
EOF
end

def netName
  "#{modulename[0,1].downcase + modulename[1..-1]}Net"
end

SIZEOFLONG = 32
ADDRDELIM = ":"

def toHex(num)
  "%x" % num
end

def quadForm(hexstr, length = 16)
  throw "hex string #{hexstr} is not padded out to a multiple of 4 hex digits" if hexstr.length % 4  != 0
  s = hexstr.dup
  1.upto(s.length/4 - 1){ |i|
    s.insert(4*i + i - 1, ADDRDELIM)
  }
  #normalise to reduce unnecessary 0s
  s.gsub!(/0{4,4}/,"0").gsub!(/:0+([^0:])/, ':\1')
  s
end

SIMPLE = true

def padWithZero(value, width, fromFront = false)
  value.reverse! if fromFront
  while value.length < width
    value << "0"
  end
  value.reverse! if fromFront
end

def createNetmask(edge)
  if SIMPLE
    #prefix of 30f and source is left 16 bits

    #Without reverse for node 16 the hex number is 10 but when added is no
    #different from node 1 however reverse produces 01. Hopefully this will
    #guarantee uniqueness of netmask for other numbers too.
    prefix = "30f" + toHex(edge.source.name).reverse
    
    padWithZero(prefix, 8)
    throw "prefix #{prefix} greater than 8" if prefix.length > 8
    suffix = toHex(edge.target.name)
    padWithZero(suffix, 8, true)
    throw "suffix #{suffix} greater than 8" if suffix.length > 8
    if @netmaskunique.include?(prefix+suffix)
      throw "netmask not unique #{quadForm(prefix + suffix)}"
    else
      @netmaskunique<<(prefix+suffix)
    end
    return quadForm(prefix + suffix)
  end

# Can do distance from root hierarchical routing i.e. 2 hops from root node is
# level 1 and 3 hops is level 2 etc.

end

#Use node name and iface index to generate ifaceId since this is guaranteed
#unique in sim
#can use ifaceType if we want to gen PPP or ethernet type IfIds
def createIfaceId(node, edge)
  i = nil
  node.ifaces.each{|i|
    break if i.edge == edge
  }
  index = node.ifaces.index(i);

  prefix = toHex(node.name)
  padWithZero(prefix, 8)
  suffix = toHex(index)
  padWithZero(suffix, 8, true)
  quadForm(prefix + suffix)
end

def createMACAddress
  high = "%x"  % (rand(2**SIZEOFLONG-1) & 0xFFFFFF)
  low = "%x"  % (rand(2**SIZEOFLONG-1) & 0xFFFFFF)
end


def createIPv6Address
  extreme  = "%x"  % rand(2**SIZEOFLONG-1)
  extreme.insert(4, ADDRDELIM)
  high = "%x"  % rand(2**SIZEOFLONG-1)
  high.insert(4, ADDRDELIM)
  normal = "%x"  % rand(2**SIZEOFLONG-1)
  normal.insert(4, ADDRDELIM)
  low = "%x"  % rand(2**SIZEOFLONG-1)
  low.insert(4, ADDRDELIM)
  return extreme + ADDRDELIM + high + ADDRDELIM + normal + ADDRDELIM + low
end

end#NedFile

def prefixMatches(lhs, rhs)
  prefix(lhs).eql? prefix(rhs)
end

def suffix(addr)
  prefix(addr.reverse).reverse
end

def prefix(addr)
  addr.scan(/[^:]+:/).collect[0,4].join.chop!
end

def permutePrefix(s)
  pos = rand(s.size-1)

  #do not want to overwrite 30f global prefix
  pos = pos + 3 if (pos <= 2) 

  # do not want to overwrite ADDRDELIM otherwise not valid address
  while s[pos].eql? NedFile::ADDRDELIM[0] do
     pos = rand(s.size-1)
    pos = pos + 3 if (pos <= 2) #do not want to overwrite 30f global prefix
  end

  e = toHex(rand(15))
  while s[pos].eql? e[0]
    e = toHex(rand(15))                   
  end
  s[pos] = e
  s
end
#main
#don't want different numbers from rand every time we run
srand(0)
n = NedFile.new("TestMe")
n.run
