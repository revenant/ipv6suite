# Copyright (C) 2002 by Johnny Lai
# Converts BRITE topology files into Graphviz dot format
# Only actual topological information i.e. nodes and links are converted
# Run with awk -f Brite2dot.awk example.brite > example.dot
BEGIN { nodeStart=0; edgeStart=0; nodeCount=0; edgeCount=0; print "graph G { "}
{
  if (length($0) == 0 || $0 ~/""/)
    next;

  if ($0 == /Topology/)
  {  
    nodeCount = $3;
    print "nodeCount is " nodeCount;
    edgeCount = $5;
    print "edgeCount is " edgeCount;
    next;
  }
    
  if ($0 ~ /Model/)
    next;
  
  if ($0 ~ /Nodes/)
  {
    nodeStart = 1;
    next;
  }
  
  if ($0 ~ /Edges/)
  {
    edgeStart = 1;
    next;
  }
  
  if (nodeStart == 1 && edgeStart == 0)
  {
    print $1 "[label=\"router" $1 "\"];";
  }
  else if (edgeStart == 1)
  {
    print $2 "--" $3;
  }
}
END { print "\}" }


  
