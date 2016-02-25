<?xml version="1.0"?>

<!-- Same functionality as show_snmp.sh but implemented using xslt -->

<xsl:stylesheet version="1.0"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		xmlns:config="http://tail-f.com/ns/config/1.0"
		xmlns:SNMPv2_MIB="http://tail-f.com/ns/mibs/SNMPv2-MIB/200210160000Z"
		xmlns:SNMP_FRAMEWORK_MIB="http://tail-f.com/ns/mibs/SNMP-FRAMEWORK-MIB/200210140000Z">


  <xsl:output method="text"/>
  <xsl:strip-space elements="*"/>

  <xsl:variable name="nl">
<xsl:text>
</xsl:text>
  </xsl:variable>



  <xsl:template match="/">
    <xsl:variable name="snmpInPkts"
		  select="/config:config/SNMPv2_MIB:SNMPv2-MIB/SNMPv2_MIB:snmp/SNMPv2_MIB:snmpInPkts"/>
    <xsl:variable name="snmpInBadVersions"
		  select="/config:config/SNMPv2_MIB:SNMPv2-MIB/SNMPv2_MIB:snmp/SNMPv2_MIB:snmpInBadVersions"/>
    <xsl:variable name="snmpInBadComunityNames"
		  select="/config:config/SNMPv2_MIB:SNMPv2-MIB/SNMPv2_MIB:snmp/SNMPv2_MIB:snmpInBadCommunityNames"/>
    <xsl:variable name="snmpInBadComunityUses"
		  select="/config:config/SNMPv2_MIB:SNMPv2-MIB/SNMPv2_MIB:snmp/SNMPv2_MIB:snmpInBadCommunityUses"/>
    <xsl:variable name="snmpInASNParseErrs"
		  select="/config:config/SNMPv2_MIB:SNMPv2-MIB/SNMPv2_MIB:snmp/SNMPv2_MIB:snmpInASNParseErrs"/>
    <xsl:variable name="snmpSilentDrops"
		  select="/config:config/SNMPv2_MIB:SNMPv2-MIB/SNMPv2_MIB:snmp/SNMPv2_MIB:snmpSilentDrops"/>
    <xsl:variable name="snmpProxyDrops"
		  select="/config:config/SNMPv2_MIB:SNMPv2-MIB/SNMPv2_MIB:snmp/SNMPv2_MIB:snmpProxyDrops"/>
    <xsl:variable name="snmpEnableAuthenTraps"
		  select="/config:config/SNMPv2_MIB:SNMPv2-MIB/SNMPv2_MIB:snmp/SNMPv2_MIB:snmpEnableAuthenTraps"/>
    <xsl:variable name="snmpEngineID"
		  select="/config:config/SNMP_FRAMEWORK_MIB:SNMP-FRAMEWORK-MIB/SNMP_FRAMEWORK_MIB:snmpEngine/SNMP_FRAMEWORK_MIB:snmpEngineID"/>
    <xsl:variable name="snmpEngineBoot"
		  select="/config:config/SNMP_FRAMEWORK_MIB:SNMP-FRAMEWORK-MIB/SNMP_FRAMEWORK_MIB:snmpEngine/SNMP_FRAMEWORK_MIB:snmpEngineBoots"/>
    <xsl:variable name="snmpEngineTime"
		  select="/config:config/SNMP_FRAMEWORK_MIB:SNMP-FRAMEWORK-MIB/SNMP_FRAMEWORK_MIB:snmpEngine/SNMP_FRAMEWORK_MIB:snmpEngineTime"/>
    



    <xsl:text>Chassis: 1506199

</xsl:text>

    <xsl:value-of select="$snmpInPkts"/>
    <xsl:text> SNMP packets input
</xsl:text>
    <xsl:value-of select="$snmpInBadVersions"/>
    <xsl:text> Bad SNMP version errors
</xsl:text>
    <xsl:value-of select="$snmpInBadComunityNames"/>
    <xsl:text> Unknown community name
</xsl:text>
    <xsl:value-of select="$snmpInBadComunityUses"/>
    <xsl:text> Illegal operation for community name supplied
</xsl:text>
    <xsl:value-of select="$snmpInASNParseErrs"/>
    <xsl:text> Encoding errors
</xsl:text>
    <xsl:value-of select="$snmpSilentDrops"/>
    <xsl:text> Number of dropped packets
</xsl:text>
    <xsl:value-of select="$snmpProxyDrops"/>
    <xsl:text> Number of dropped proxy packets
</xsl:text>

    <xsl:value-of select="$nl"/>

    <xsl:text>SNMP Authentication traps: </xsl:text>
    <xsl:value-of select="concat($snmpEnableAuthenTraps, $nl)"/>

    <xsl:text>SNMP Engine ID: </xsl:text>
    <xsl:value-of select="concat($snmpEngineID, $nl)"/>
    <xsl:text>SNMP Engine boots: </xsl:text>
    <xsl:value-of select="concat($snmpEngineBoot, $nl)"/>
    <xsl:text>SNMP Engine time: </xsl:text>
    <xsl:value-of select="concat($snmpEngineTime, $nl)"/>
    
  </xsl:template>

</xsl:stylesheet>
