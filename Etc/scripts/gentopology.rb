#! /usr/bin/env ruby
#
#  Author: Johnny Lai
#  Copyright (c) 2005 Johnny Lai
#
# =DESCRIPTION
# Part of NedFile.rb
#
# =REVISION HISTORY
#  INI 2005-02-01 Changes
#

require 'rgl/adjacency'
require 'rgl/dot'

#Ruby to Ned type mappings
NedType = Struct.new( "NedType", :className, :instanceName, :icon)

class Vertex
attr_accessor :name, :ifaces

  def to_s
    "#{self.class} #{@name}"
  end

  def nodeName
    Vertex.nedTypes[self.class].instanceName + self.name.to_s
  end

  def className
    Vertex.nedTypes[self.class].className
  end

  def icon
    Vertex.nedTypes[self.class].icon
  end

  def inspect
    #turn off otherwise loops forever because our structure is circular
    print self.class, " ", name, " ifsize=", ifaces.length
  end
end

class AP < Vertex
end
class Host < Vertex
end

class Router < Vertex
end
class CR < Router
end
class AR < Router
end
class HA < Router
end

class Vertex

  @@nedTypes = nil

  def Vertex.nedTypes
    unless @@nedTypes 
      @@nedTypes = Hash.new
      @@nedTypes[AR] = NedType.new("Router6", "ar", "router")
      @@nedTypes[CR] = NedType.new("Router6", "cr", "router")
      @@nedTypes[AP] = NedType.new("AccessPoint", "ap", "switch1_s")
      @@nedTypes[Host] = NedType.new("UDPNode", "cn", "pc")
      @@nedTypes[HA] = NedType.new("Router6", "ha", "router")
    end
    @@nedTypes
  end

  def initialize(name)
    self.name = name
    ifaces = nil
  end
end

class NetworkInterface

  #edge is what links us to the adjacent neighbour. 
  # edge[0] is source vertex & edge[1] is dest vertex although graph is undirected so it
  #depends on the perspective you want to take.

  #identify remote index by traversing edge and looking at its ifaces to see
  #which one has that edge. This would only break down if we have more than one
  #connection between a pair of nodes. To solve that we'd need to store the
  #actual destIface too and use it to infer the index.

  attr_accessor :node, :edge, :address, :ifname

  def initialize(node, edge, address = nil)
    self.edge = edge
    self.node = node
    self.address = address
  end

  def ifIndex
    node.ifaces.index(self)
  end

  def remoteIfIndex
    ifaces = remoteNode.ifaces
    ifaces.each_index { |i| 
      return i if ifaces[i].edge.equals? self.edge
    }    
  end

  def remoteNode
    edge[0] != node ? edge[0] : edge[1]
  end

  #redefining as two NetworkInterfaces may have exactly same components when
  #address is not assigned yet which happens a lot when two ifaces are on same
  #link or if more than one link connects a pair of nodes

  def ==(other) 
    self.equals?(other)
  end

  def equals?(other)
    self.object_id == other.object_id
  end
end


class Link < RGL::Edge::UnDirectedEdge
  attr_accessor :netmask, :ifaceType
  def initialize(source, target, netmask = nil, ifaceType = nil)
    @source =  source 
    @target = target
    self.netmask = netmask
    self.ifaceType = ifaceType
  end

  def ==(other) 
    self.equals?(other)
  end

  def equals?(other)
    self.object_id == other.object_id
  end
end

=begin
Adds 3 APs to and converts a normal RGL graph into one which uses AP/AR/CR as vertexes
=end
def genEHTopology(dg)

vs = Hash.new
g=RGL::AdjacencyGraph.new

dg.each{|v|
	case v
          when 3 
            vs[v] = CR.new(v)
          when 2 
            vs[v] = Host.new(v)
          when 1
            vs[v] = HA.new(v)
          else
            vs[v] = AR.new(v)
	    vs[v+dg.size-3] = AP.new(v+dg.size-3)
	    g.add_edge(vs[v], vs[v+dg.size-3])
	end
}

dg.each_edge {|u,v| g.add_edge(vs[u],vs[v]) }

#vs.each_key{|n|
#  v = vs[n]
#
##g.each {|v|
##    puts v.name
#  if v.instance_of? AR
#    #3.times do 
#     ap = AP.new(vs.size + 1)
#      vs[vs.size + 1] = ap
#      g.add_vertex(ap)
#      g.add_edge(v, ap)
#    #end    
#  end
#}

#g.write_to_graphic_file
[g, vs]
end

#Generate a nexthop map for node belonging to graph g
def routingTable(g, node)

  require 'rgl/traversal.rb'
  bfs = RGL::BFSIterator.new(g,node)
  bfs.attach_distance_map
  parents=Hash.new
  #Parent of node is itself in order to stop the backtrack recursion
  parents[node] = node

  teststr = ""
  bfs.set_tree_edge_event_handler {|u,v|
    teststr << "#{u.name} -> #{v.name}\n"
    #existance of following line modifies the MN connection order to the ARs !
#    puts "#{u.name} -> #{v.name}"
    parents[v] = u
  }
  bfs.set_to_end
  #Backtrack from each dest v to find nexthop of node to v
  nexthops = Hash.new
  g.each{|v|
    nexthop = v
    #   print "#{nexthop.name}"
    while (parents[nexthop] != parents[parents[nexthop]])
      nexthop = parents[nexthop]
      #   print "->#{nexthop.name}"
    end
    nexthops[v] = nexthop
  }

=begin
# Not needed since the host should have an address that is derived from its link
# and we have routes to every link
  g.each{|v|
    # print " distance from #{node}=#{bfs.distance_to_root(v)}\n"

    #proper dest routes to next hop neighbours that are hosts only not AP or router.
    if bfs.distance_to_root(v) == 1 and v.class == Host
      i = 0
      node.ifaces.each{|i|
        break if i.remoteNode == nexthops[v]
      }

      print "dest=#{v.nodeName}:#{} nextHop=#{nexthop.nodeName}:#{nexthop.ifaces[i.remoteIfIndex].address} via #{i.ifIndex}\n"
    end

  }
=end

  [nexthops, bfs, teststr]
end
