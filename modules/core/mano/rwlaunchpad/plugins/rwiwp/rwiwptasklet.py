
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

# Workaround RIFT-6485 - rpmbuild defaults to python2 for
# anything not in a site-packages directory so we have to
# install the plugin implementation in site-packages and then
# import it from the actual plugin.

import rift.tasklets.rwiwptasklet

class Tasklet(rift.tasklets.rwiwptasklet.IwpTasklet):
    pass

# vim: sw=4