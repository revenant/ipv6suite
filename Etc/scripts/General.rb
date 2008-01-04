#obtained from http://rubyforge.org/snippet/detail.php?type=snippet&id=2
class Object
  def deep_clone
    Marshal.load(Marshal.dump(self))
  end
end

module General  
  TESTDIR="~/src/IPv6SuiteWithINET/Research/Networks/test"

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
    p c
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
  
  #derive a valid C++ expression i.e. no . in ned network names and thus filenames
  def safeLevelLabel(level, invert = false)
    if not invert
      label = level
      label = level.to_s.sub!(/[.]/,"d") if level.kind_of? Float
    else
      throw "label names should be a string" if level.class != String
      label = level.dup
      label.sub!(/d/, '.') if level =~ /^[0-9]+d[0-9]+$/
    end
    label
  end

  def test_safeLevelLabel
    level = 2.341324
    assert_equal("2d341324", safeLevelLabel(level))
    label = "adsf2d341324"
    assert_equal(label, safeLevelLabel(label, true))
    assert_equal("3.1414", safeLevelLabel("3d1414", true))
    assert_equal("3dd1414", safeLevelLabel("3dd1414", true))
  end
end
