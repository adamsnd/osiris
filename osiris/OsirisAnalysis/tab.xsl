<?xml version="1.0" encoding="utf-8"?>
<!--
#
# ===========================================================================
#
#                            PUBLIC DOMAIN NOTICE
#               National Center for Biotechnology Information
#
#  This software/database is a "United States Government Work" under the
#  terms of the United States Copyright Act.  It was written as part of
#  the author's official duties as a United States Government employee and
#  thus cannot be copyrighted.  This software/database is freely available
#  to the public for use. The National Library of Medicine and the U.S.
#  Government have not placed any restriction on its use or reproduction.
#
#  Although all reasonable efforts have been taken to ensure the accuracy
#  and reliability of the software and data, the NLM and the U.S.
#  Government do not and cannot warrant the performance or results that
#  may be obtained by using this software or data. The NLM and the U.S.
#  Government disclaim all warranties, express or implied, including
#  warranties of performance, merchantability or fitness for any particular
#  purpose.
#
#  Please cite the author in any work or product based on this material.
#
# ===========================================================================
#
#  FileName: tab.xsl
#  Author:   Douglas Hoffman
#  Description:  Export allele info from OSIRIS analysis file to a
#   tab-delimited text file.
#
-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

  <xsl:output method="text"/>

  <xsl:param name="OL" select="yes"/>
  <xsl:param name="UseChannelNr" select="0"/>
  <xsl:variable name="TAB" select="'&#9;'"/>
  <xsl:variable name="LF" select="'&#10;'"/>

  <xsl:template name="OsirisExport">
  <export>
    <name>Spreadsheet</name>
    <file-extension>tab</file-extension>
    <file-extension>txt</file-extension>
    <extension-override>true</extension-override>
    <default-location>*A</default-location>
    <xsl-params>
      <param>
        <name>OL</name>
        <description>Indicate off-ladder alleles</description>
        <type>checkbox</type>
        <checked-value>1</checked-value>
        <unchecked-value>0</unchecked-value>
      </param>
      <param>
        <name>UseChannelNr</name>
        <description>Show channel numbers in column headings</description>
        <type>checkbox</type>
        <checked-value>1</checked-value>
        <unchecked-value>0</unchecked-value>
      </param>
    </xsl-params>
  </export>
  </xsl:template>

  <xsl:template name="GetBool">
    <xsl:param name="s"/>
    <xsl:choose>
      <xsl:when test="$s = ''">
        <xsl:value-of select="0"/>
      </xsl:when>
      <xsl:when test="contains('FfNn0',substring($s,1,1))">
        <!-- check for false or no -->
        <xsl:value-of select="0"/>
      </xsl:when>
      <xsl:when test="contains('YyTt123456789',substring($s,1,1))">
        <!-- check for true or yes -->
        <xsl:value-of select="1"/>
      </xsl:when>
      <xsl:when test="boolean($s) = 'true'">
        <xsl:value-of select="1"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="0"/>
      </xsl:otherwise>
   </xsl:choose>
  </xsl:template>

  <xsl:variable name="bOL">
    <xsl:call-template name="GetBool">
      <xsl:with-param name="s" select="$OL"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="bChannel">
    <xsl:call-template name="GetBool">
      <xsl:with-param name="s" select="$UseChannelNr"/>
    </xsl:call-template>
  </xsl:variable>


  <xsl:template name="writeHeader">
    <xsl:text>Sample</xsl:text>
    <xsl:for-each select="/OsirisAnalysisReport/Heading/Channel">
      <xsl:variable name="chanNr" select="ChannelNr"/>
      <xsl:for-each select="LocusName">
        <xsl:value-of select="$TAB"/>
        <xsl:value-of select="."/>
        <xsl:if test="$bChannel &gt; 0">
          <xsl:value-of select="'-'"/>
          <xsl:value-of select="$chanNr"/>
        </xsl:if>
      </xsl:for-each>
    </xsl:for-each>
    <xsl:value-of select="$LF"/>
  </xsl:template>

  <xsl:template name="writeSample">
    <xsl:param name="sample"/>
    <xsl:variable name="SampleName" select="$sample/Name"/>
    <xsl:value-of select="$SampleName"/>
    <xsl:for-each select="/OsirisAnalysisReport/Heading/Channel">
      <xsl:for-each select="LocusName">
        <xsl:variable name="locusName" select="."/>
        <xsl:value-of select="$TAB"/>
        <xsl:variable name="locus" 
          select="$sample/Locus[LocusName = $locusName]"/>
        <xsl:if test="$locus">
          <xsl:variable name="isAmel">
            <xsl:variable name="LocusU">
              <xsl:value-of select="translate($locus,'amel','AMEL')"/>
            </xsl:variable>
            <xsl:choose>
              <xsl:when test="contains($LocusU,'AMEL')">
                <xsl:value-of select="1"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="0"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:variable>
          <xsl:for-each select="$locus/Allele[not(Disabled = 'true')]">
            <xsl:if test="position() &gt; 1">
              <xsl:text>, </xsl:text>
            </xsl:if>
            <xsl:if test="($bOL &gt; 0) and (OffLadder = 'true')">
              <xsl:text>OL</xsl:text>
            </xsl:if>
            <xsl:choose>
              <xsl:when test="$isAmel = 0">
                <xsl:value-of select="Name"/>
              </xsl:when>
              <xsl:when test="Name = 1">
                <xsl:text>X</xsl:text>
              </xsl:when>
              <xsl:when test="Name = 2">
                <xsl:text>Y</xsl:text>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="Name"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:for-each>
        </xsl:if>
      </xsl:for-each>
    </xsl:for-each>
    <xsl:value-of select="$LF"/>
  </xsl:template>

  <xsl:template match="/">
    <xsl:call-template name="writeHeader"/>
    <xsl:for-each select="/OsirisAnalysisReport/Table/Sample">
      <xsl:call-template name="writeSample">
        <xsl:with-param name="sample" select="."/>
      </xsl:call-template>
    </xsl:for-each>

  </xsl:template>

</xsl:stylesheet>
