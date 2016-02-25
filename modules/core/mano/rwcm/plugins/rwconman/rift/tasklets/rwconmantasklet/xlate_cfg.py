#!/usr/bin/env python3

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#


'''
This script will go through the input conffiguration template and convert all the matching "regular expression" and "strings"
specified in xlate_cp_list & xlate_str_list with matching IP addresses passed in as dictionary to this script.

-i Configuration template
-o Output final configuration complete with IP addresses
-x Xlate(Translate dictionary in string format
-t TAGS to be translated

'''

import sys
import getopt
import ast
import re
import yaml
import netaddr

from inspect import getsourcefile
import os.path

def main(argv=sys.argv[1:]):
    cfg_template = None
    cfg_file = None
    xlate_dict = None
    try:
        opts, args = getopt.getopt(argv,"i:o:x:")
    except getopt.GetoptError:
        print("Check arguments {}".format(argv))
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-i':
            cfg_template = arg
        elif opt in ("-o"):
            cfg_file = arg
        elif opt in ("-x"):
            xlate_arg = arg

    # Read TAGS from yaml file
    # Read the translation tags from yaml file
    yml_dir = os.path.dirname(os.path.abspath(getsourcefile(lambda:0)))
    tags_input_file = os.path.join(yml_dir, 'xlate_tags.yml')
    with open(tags_input_file, "r") as ti:
        xlate_tags = yaml.load(ti.read())

    try:
        xlate_dict = ast.literal_eval(xlate_arg)
        print("xlate script: -i {} -o {} -x {} -t {}".format(cfg_template, cfg_file, repr(xlate_dict), xlate_tags))
        try:
            with open(cfg_template, 'r') as r:
                try:
                    with open(cfg_file, 'w') as w:
                        # Traslate
                        try:
                            # For each line
                            for line in r:
                                if line.startswith("#"):
                                    # Skip comment lines
                                    continue
                                #print("Line : ", line)
                                # For each Connection Point translation to IP
                                for cp_string in xlate_tags['xlate_cp_list']:
                                    match = re.search(cp_string, line)
                                    if match is not None:
                                        # resolve IP address using Connection Point dictionary
                                        resolved_ip = xlate_dict[match.group(1)]
                                        if resolved_ip is None:
                                            print("No matching CP found: ", match.group(1))
                                            exit(2)
                                        else:
                                            line = line[:match.start()] + resolved_ip + line[match.end():]
                                            #print("1.write line: ", line)

                                # For each colon(:) separated tag, i.e. 2 inputs in a tag.
                                for ucp_string in xlate_tags['xlate_colon_list']:
                                    #print("Searching :", ucp_string)
                                    match = re.search(ucp_string, line)
                                    if match is not None:
                                        #print("match :", match.group())
                                        # resolve IP address using Connection Point dictionary for specified member (unique) index
                                        ucp_str_list = match.group(1).split(':')
                                        #print("matched = {}, split list = {}".format(match.group(1), ucp_str_list))
                                        if len(ucp_str_list) != 2:
                                            print("Invalid TAG in the configuration: ", match.group(1))
                                            exit(2)

                                        # Unique Connection Point translation to IP
                                        if ucp_string.startswith('<rw_unique_index:'):
                                            member_vnf_index = int(ucp_str_list[0])
                                            resolved_ip = xlate_dict[member_vnf_index][ucp_str_list[1]]
                                            #print("member_vnf_index = {}, resolved_ip = {}", member_vnf_index, resolved_ip)
                                            if resolved_ip is None:
                                                print("For Unique index ({}), No matching CP found: {}", ucp_str_list[0], ucp_str_list[1])
                                                exit(2)
                                            else:
                                                line = line[:match.start()] + resolved_ip + line[match.end():]
                                                #print("1.write line: ", line)

                                        # Traslate given CP address & mask into netaddr
                                        if ucp_string.startswith('<rw_connection_point:masklen'):
                                            resolved_ip = xlate_dict[ucp_str_list[0]]
                                            masklen = ucp_str_list[1]
                                            if resolved_ip is None:
                                                print("No matching CP found: ", ucp_str_list[0])
                                                exit(2)
                                            if int(masklen) <= 0:
                                                print("Invalid mask length: ", masklen)
                                                exit(2)
                                            else:
                                                # Generate netaddr
                                                ip_str = resolved_ip + '/' + masklen
                                                #print("ip_str:", ip_str)
                                                ip = netaddr.IPNetwork(ip_str)
                                                
                                                if ucp_string.startswith('<rw_connection_point:masklen_broadcast'):
                                                    # Traslate given CP address & mask into broadcast address
                                                    addr = ip.broadcast
                                                if ucp_string.startswith('<rw_connection_point:masklen_network'):
                                                    # Traslate given CP address & mask into network address
                                                    addr = ip.network
                                                    
                                                #print("2.write line: ", line)
                                                line = line[:match.start()] + str(addr) + line[match.end():]
                                                #print("3.write line: ", line)

                                # For each connection point to tuple replacement
                                for cp_string in xlate_tags['xlate_cp_to_tuple_list']:
                                    match = re.search(cp_string, line)
                                    if match is not None:
                                        # resolve IP address using Connection Point dictionary
                                        resolved_ip = xlate_dict[match.group(1)]
                                        if resolved_ip is None:
                                            print("No matching CP found: ", match.group(1))
                                            exit(2)
                                        else:
                                            #print("2.write line: ", line)
                                            line = line[:match.start()] + match.group(1) + ':'  + resolved_ip + line[match.end():]
                                            #print("3.write line: ", line)

                                # For each direct replacement (currently only management IP address for ping/pong)
                                for replace_tag in xlate_tags['xlate_str_list']:
                                    replace_string = replace_tag[1:-1]
                                    line = line.replace(replace_tag, xlate_dict[replace_string])
                                    #print("4.write line: ", line)

                                # Finally write the modified line to the new config file
                                w.write(line)
                        except Exception as e:
                            print("Error ({}) on line: {}".format(str(e), line))
                            exit(2)
                except Exception as e:
                    print("Failed to open for write: {}, error({})".format(cfg_file, str(e)))
                    exit(2)
        except Exception as e:
            print("Failed to open for read: {}, error({})".format(cfg_template, str(e)))
            exit(2)
        print("Wrote configuration file", cfg_file)
    except Exception as e:
        print("Could not translate dictionary, error: ", str(e))

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(str(e))
