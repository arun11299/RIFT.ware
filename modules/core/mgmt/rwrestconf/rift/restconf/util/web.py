# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Max Beckett
# Creation Date: 9/5/2015
# 

import urllib.parse

def split_url(url):
    is_operation = False
    url_parts = urllib.parse.urlsplit(url)
    whole_url = url_parts[2]
    if whole_url.startswith("/api/operational/"):
        relevant_url = whole_url.split("/api/operational/", 1)[-1]
    elif whole_url.startswith("/api/operations/"):
        is_operation = True
        relevant_url = whole_url.split("/api/operations/", 1)[-1]
    elif whole_url.startswith("/api/running/"):
            relevant_url = whole_url.split("/api/running/", 1)[-1]
    elif whole_url.startswith("/api/running"):
            relevant_url = whole_url.split("/api/running", 1)[-1]
    elif whole_url.startswith("/api/config/"):
            relevant_url = whole_url.split("/api/config/", 1)[-1]
    elif whole_url.startswith("/api/config"):
            relevant_url = whole_url.split("/api/config", 1)[-1]
    else:
        raise ValueError("unknown url component in %s" % whole_url)

    url_pieces = relevant_url.split("/")

    return url_pieces, relevant_url, url_parts[3], is_operation


def is_config(url):
    url_parts = urllib.parse.urlsplit(url)
    whole_url = url_parts[2]
    return whole_url.startswith("/api/running") or whole_url.startswith("/api/config")




