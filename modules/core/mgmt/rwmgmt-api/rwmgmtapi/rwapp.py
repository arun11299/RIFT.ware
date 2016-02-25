
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import rwmgmtapi.confd
import rwmgmtapi.xml2json


class Ns(object):
    MAP = {
        rwmgmtapi.rwxml.BASE_PREFIX : rwmgmtapi.rwxml.BASE_NS,
        rwmgmtapi.rwxml.FPATH_PREFIX : rwmgmtapi.rwxml.FPATH_NS,
        rwmgmtapi.rwxml.APPMGR_PREFIX : rwmgmtapi.rwxml.APPMGR_NS,
        rwmgmtapi.confd.Ns.QUERY_PREFIX : rwmgmtapi.confd.Ns.QUERY_NS,
        rwmgmtapi.confd.Ns.REST_PREFIX : rwmgmtapi.confd.Ns.REST_NS
    }
