# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Max Beckett
# Creation Date: 9/11/2015
# 

class Configuration(object):
    def __init__(self):
        self._log_timing = False

    @property
    def log_timing(self):
        return self._log_timing

    @log_timing.setter
    def log_timing(self, toggle):
        self._log_timing = toggle
