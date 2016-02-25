#!/bin/bash

maapi=${CONFD_DIR}/bin/maapi

chassis="1506199"

snmpInPkts=`$maapi --get '/SNMPv2_MIB:SNMPv2-MIB/snmp/snmpInPkts'`
snmpInBadVersions=`$maapi --get '/SNMPv2_MIB:SNMPv2-MIB/snmp/snmpInBadVersions'`
snmpInBadComunityNames=`$maapi --get '/SNMPv2_MIB:SNMPv2-MIB/snmp/snmpInBadCommunityNames'`
snmpInBadComunityUses=`$maapi --get '/SNMPv2_MIB:SNMPv2-MIB/snmp/snmpInBadCommunityUses'`
snmpInASNParseErrs=`$maapi --get '/SNMPv2_MIB:SNMPv2-MIB/snmp/snmpInASNParseErrs'`
snmpSilentDrops=`$maapi --get '/SNMPv2_MIB:SNMPv2-MIB/snmp/snmpSilentDrops'`
snmpProxyDrops=`$maapi --get '/SNMPv2_MIB:SNMPv2-MIB/snmp/snmpProxyDrops'`

snmpEnableAuthenTraps=`$maapi --get '/SNMPv2_MIB:SNMPv2-MIB/snmp/snmpEnableAuthenTraps'`
snmpEngineID=`$maapi --get '/SNMP_FRAMEWORK_MIB:SNMP-FRAMEWORK-MIB/snmpEngine/snmpEngineID'`
snmpEngineBoot=`$maapi --get '/SNMP_FRAMEWORK_MIB:SNMP-FRAMEWORK-MIB/snmpEngine/snmpEngineBoots'`
snmpEngineTime=`$maapi --get '/SNMP_FRAMEWORK_MIB:SNMP-FRAMEWORK-MIB/snmpEngine/snmpEngineTime'`


echo "Chassis: ${chassis}"
echo "${snmpInPkts} SNMP packets input"
echo "${snmpInBadVersions} Bad SNMP version errors"
echo "${snmpInBadComunityNames} Unknown community name"
echo "${snmpInBadComunityUses} Illegal operation for community name supplied"
echo "${snmpInASNParseErrs} Encoding errors"
echo "${snmpSilentDrops} Number of dropped packets"
echo "${snmpProxyDrops} Number of dropped proxy packets"
echo ""

echo "SNMP Authentication traps: ${snmpEnableAuthenTraps}"
echo "SNMP Engine ID: ${snmpEngineID}"
echo "SNMP Engine boots: ${snmpEngineBoot}"
echo "SNMP Engine time: ${snmpEngineTime}"

