# -*- coding: ISO-8859-1 -*-

#obtained from http://rubyforge.org/snippet/detail.php?type=snippet&id=2
class Object
  def deep_clone
    Marshal.load(Marshal.dump(self))
  end
end

module General  

  #returns index for matching first line 
  def searchForLine(linex, regexp)
    lines.each_with_index {|line,index|
      if regexp.match(line) then
        return index
      end
    }
    0
  end
  
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
  
  #Produce a f1="l1", f2="l2", f3="l3", .. where f is in factors and l is in
  #encodedFactors
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