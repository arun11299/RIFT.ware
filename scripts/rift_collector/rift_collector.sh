#!/bin/bash

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

# WW

#workdir=/var/tmp/riftcollector
workdir=/var/tmp/riftcollector$(date "+%Y%m%d%H%M") 
compressed_file=/var/tmp/riftcollector$(date "+%Y%m%d%H%M").tgz
logdir=${workdir}/rawlog
rm -rf $logdir
mkdir -p $logdir
mkdir -p $logdir/etc
report=${workdir}/report.txt
rawlog=${workdir}/rawlog.txt

rm -f $report
rm -f $rawlog


declare YELLOW='\033[33m'
declare RED='\033[31m'
declare NORMAL='\033[00;00m'
declare GREEN='\033[32m'

function msgrawlog()
{
    _msg=$@
    echo "$_msg" >> $rawlog
}

function msgreport()
{
    _msg=$@
    echo "$_msg" >> $report
}


function warn()
{
    _msg=$@
    echo -e "${YELLOW}  -Warn: ${_msg}${NORMAL}"
    #log1 "$(date "+%Y/%m/%d %H:%M:%S") -Warn: $_msg"
}

function info()
{
    _msg=$@
    echo -e "${NORMAL}  -Info: ${_msg}"
    #log1 "$(date "+%Y/%m/%d %H:%M:%S") -Info: ${_msg}"
}

function okinfo()
{
    _msg=$@
    echo -e "${GREEN}  -Info: ${_msg}${NORMAL}"
    #log1 "$(date "+%Y/%m/%d %H:%M:%S") -Info: ${_msg}"
}


function die {
    echo >&2 "$@"
    exit 1
}


function file_collect()
{
    _file=$1
    if [ -e $_file ]; then
        cp -a $_file ${logdir}/etc
    fi
} 


function cmd_collect()
{
    _cmd=$1
    _parser=$2
    _ofile=$(echo "$_cmd" | sed -e "s/ /_/g" -e "s/\"//g" -e "s/\'//g" )
    msgrawlog "#############################"
    msgrawlog "#  $_cmd "
    msgrawlog "#############################"
    $_cmd > $logdir/$_ofile.txt 2>&1 
    cat $logdir/$_ofile.txt >> $rawlog
    if [[ ( -n $_parser ) && ( -f $_parser ) ]]; then
        msgreport "####################################"
        msgreport "# Summary of \"$_cmd\" output by $_parser" 
        msgreport "####################################"
        cat $logdir/$_ofile.txt | ./$_parser >> $report
    fi 
}

#
# rpt_collect collect simple output to report file directly
#
function rpt_collect()
{
    _cmd=$1
    msgreport "#############################"
    msgreport "#  $_cmd "
    msgreport "#############################"
    eval $(echo $_cmd) >> $report 2>&1 

}



function bios_settings()
{
    case $1 in
    Intel)
        get_intel_bios_settings
        ;;
    *)
        echo "BIOS collecting for $1 is not supported"
        ;;
    esac    
}

function get_intel_bios_settings()
{
   if [ -x /usr/bin/syscfg/syscfg ]; then
     rm -f $logdir/bios_settings.ini
     /usr/bin/syscfg/syscfg /s $logdir/bios_settings.ini >/dev/null 2>&1
   else
        warn "No syscfg installed"
   fi
}

function get_pic_nic_info()
{
    _pci_nics=$(ls -ltr /sys/class/net/ | grep pci | awk '{print "/sys/class/net/" $NF}' | sort -V)
    
    msgreport "###########################"
    msgreport "#  PCI NIC info" 
    msgreport "###########################"
    for i in $_pci_nics; do
        _nic=$(echo $i | tr "/" " " | awk '{print $NF}')
        cmd_collect "ethtool -i $_nic"
        _driver=$(ethtool -i $_nic | grep driver | awk '{print $NF}')
        cmd_collect "ethtool $_nic"
        _speed=$(ethtool $_nic | grep Speed | awk '{print $NF}')
        _duplex=$(ethtool $_nic | grep Duplex | awk '{print $2}')
        _link=$(ethtool $_nic | grep "Link detected:" |  awk '{print $NF}')
        
        msgreport "$_nic;$_driver;$_speed;$_duplex;$_link"
    done

}

function collect_netns()
{
    for i in $(ip netns list); do
        cmd_collect "ip netns exec $i ifconfig"
        cmd_collect "ip netns exec $i route -n"
    done       

}

function collect_installed_pkg()
{

    case DIST in
        Ubuntu)
            cmd_collect  "dpkg -l" 
            ;;
        Fedora)
            cmd_collect  "rpm -qa"
            ;;
        *)
            ;;
    esac
    cmd_collect "pip2 list"
    cmd_collect "pip3 list"
}

####################################
# Main code start from here


VENDOR=$(dmidecode -s bios-vendor | awk '{print $1}')
DIST=$(lsb_release -i | awk '{print $NF}')
# BIOS

info "Getting BIOS info" 
cmd_collect  "dmidecode -t bios" bios.awk
cmd_collect  "dmidecode" 


info "Getting RAM info" 
cmd_collect  "dmidecode -t memory"  ram.awk
info "Getting NUMA info"
cmd_collect  "lscpu" lscpu.awk
# NIC
info "Getting nic info"
cmd_collect "lspci"
rpt_collect "lspci | grep Ether"
get_pic_nic_info

# Network
info "Getting Networking info "
cmd_collect "ifconfig"
cmd_collect "ifconfig -a"
cmd_collect "route -n"
cmd_collect "brctl show"
cmd_collect "ovs-vsctl show"
cmd_collect "ip netns list"
collect_netns
# OS, Kernel

info "Getting OS info "
rpt_collect "uname -a"
rpt_collect "lsb_release  -a"
rpt_collect "systemctl -t service"
rpt_collect "cat /proc/cmdline"
rpt_collect "cat /proc/meminfo"
collect_installed_pkg

# Openstack
info "Getting Openstack info "
rpt_collect "nova --version"
rpt_collect "openstack --version"
rpt_collect "neutron --version"

file_collect "/etc/nova"
file_collect "/etc/neutron"
file_collect "/etc/keystone"
file_collect "/etc/cinder"
file_collect "/etc/glance"

info "Compress reports"
tar -czf $compressed_file $workdir 2>/dev/null
info "Please send $compressed_file to your riftio contact."

echo "Done"
