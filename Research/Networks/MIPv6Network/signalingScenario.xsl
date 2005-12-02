<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
<xsl:output indent="yes"/>

  <xsl:template match="/">
    <!--
     | Output the Doctype in the result based on
     | the DOCTYPE comment we preprocessed into the document
     +-->

    <!-- For convenience, get a literal quote sign in a variable -->
    <xsl:variable name="q">"</xsl:variable>

    <!-- Get the DOCTYPE comment in a variable -->
    <xsl:variable name="d"
  select="//comment()[contains(.,'DOCTYPE')][1]"/>

    <!-- Get the "uri" part of the doctype comment -->
    <xsl:variable name="e"
  select="substring-after(normalize-space($d),
	'SYSTEM ')"/>

    <!-- Strip off the quotes from the "uri" -->
    <xsl:variable name="f"
    select="substring-before(substring-after($e,$q),$q)"/>


    <!-- Output the <!DOCTYPE -->
    <xsl:value-of disable-output-escaping="yes"
          select="concat('&#60;','!DOCTYPE ',name(/*[1]),
                  ' SYSTEM',$q,$f,$q,'>')"/>
    <xsl:apply-templates 
         select="@*|*|processing-instruction()|comment()"/>
         
    <xsl:apply-templates select="@signalingEnhance"/>
    <xsl:apply-templates select="RandomMovement"/>
        
  </xsl:template>

  <!--
   | Identity Transformation. XT doesn't seem to support the
   | more terse "@*|node()" at present, so this is the long form.
   +-->
  <xsl:template 
	match="@*|*|processing-instruction()|comment()">
    <xsl:copy>
      <xsl:apply-templates
select="@*|*|processing-instruction()|comment()"/>
    </xsl:copy>
  </xsl:template>

  <!-- Suppress printing our little trick in the output -->
  <xsl:template match="//comment()
	[contains(.,'DOCTYPE')][1]"/>


  <xsl:template match="@signalingEnhance">
    <xsl:choose>          
      <xsl:when test="$signaling='Indir'">
       <xsl:attribute name="signalingEnhance">
         <xsl:value-of select="'None'"/>
       </xsl:attribute>
      </xsl:when>
      <xsl:when test="$signaling='Dir'">
       <xsl:attribute name="signalingEnhance">
         <xsl:value-of select="'Direct'"/>
       </xsl:attribute>
      </xsl:when>
      <xsl:when test="$signaling='Cell'">
       <xsl:attribute name="signalingEnhance">
         <xsl:value-of select="'CellResidency'"/>
       </xsl:attribute>
      </xsl:when>      
     </xsl:choose>          
  </xsl:template>

  <xsl:template match="RandomMovement">
    <xsl:element name="RandomMovement">
      <xsl:attribute name="RWNodeName"><xsl:value-of select="@RWNodeName"/></xsl:attribute>
      <xsl:attribute name="RWMoveKind"><xsl:value-of select="@RWMoveKind"/></xsl:attribute>
      <xsl:attribute name="RWMinX"><xsl:value-of select="@RWMinX"/></xsl:attribute>
      <xsl:attribute name="RWMaxX"><xsl:value-of select="@RWMaxX"/></xsl:attribute>
      <xsl:attribute name="RWMinY"><xsl:value-of select="@RWMinY"/></xsl:attribute>
      <xsl:attribute name="RWMaxY"><xsl:value-of select="@RWMaxY"/></xsl:attribute>
      <xsl:attribute name="RWMoveInterval"><xsl:value-of select="@RWMoveInterval"/></xsl:attribute>
      <xsl:attribute name="RWDistance"><xsl:value-of select="@RWDistance"/></xsl:attribute>
      <xsl:attribute name="RWPauseTime"><xsl:value-of select="@RWPauseTime"/></xsl:attribute>      
      <xsl:attribute name="RWStartTime"><xsl:value-of select="@RWStartTime"/></xsl:attribute>      
    <xsl:choose>
      <xsl:when test="@RWNodeName='mn1'">
        <xsl:attribute name="RWMinSpeed"><xsl:value-of select="$mnSpeed + 0.1"/></xsl:attribute>      
        <xsl:attribute name="RWMaxSpeed"><xsl:value-of select="$mnSpeed + 1"/></xsl:attribute>        
      </xsl:when>      
      <xsl:when test="@RWNodeName='mn2'">
        <xsl:attribute name="RWMinSpeed"><xsl:value-of select="$cnSpeed + 0.1"/></xsl:attribute>      
        <xsl:attribute name="RWMaxSpeed"><xsl:value-of select="$cnSpeed + 1"/></xsl:attribute>                
      </xsl:when>
      <xsl:otherwise>
        <xsl:attribute name="RWMinSpeed"><xsl:value-of select="@RWMinSpeed"/></xsl:attribute>      
        <xsl:attribute name="RWMaxSpeed"><xsl:value-of select="@RWMaxSpeed"/></xsl:attribute>        
      </xsl:otherwise>
    </xsl:choose>
    </xsl:element>    

  </xsl:template> 

</xsl:stylesheet>

