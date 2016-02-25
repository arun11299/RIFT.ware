"""
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

@file rwlogger.py
@author Austin Cormier (austin.cormier@riftio.com)
@date 01/04/2014
@brief Python wrapper for RwLog GI

"""
import logging
import os
import sys
import importlib

import mmap
import struct

from gi.repository import RwLog
import six

# We really need to also import rwlog categories as defined in the
# original yang enum, but these are passed as a function argument to
# the rwlog API, and the Gi bindings can't do that yet.


logger = logging.getLogger(__name__)

def get_frame(frames_above=1):
    frame = sys._getframe(frames_above + 1)
    return frame


def get_caller_filename(frame):
    # Expects the caller to be two stack frames above us
    # The first stack from will be from a function in this module
    filename = frame.f_code.co_filename
    return os.path.basename(filename)


def get_caller_lineno(frame):
    # Expects the caller to be two stack frames above us
    # The first stack from will be from a function in this module
    line_number = frame.f_lineno
    return line_number

def get_module_name_from_log_category(log_category):
    """
    Function that converts category name to Python module name
    Eg. rw-generic to RwGenericYang
    """
    words = log_category.split('-')
    words.append('yang')
    return ''.join(word.capitalize() for word in words)

class RwLogger(logging.Handler):
    level_event_cls_map = {
        logging.DEBUG: 'Debug',
        logging.INFO: 'Info',
        logging.WARN: 'Warn',
        logging.ERROR: 'Error',
        logging.CRITICAL: 'Critical',
        }
    
    #Map rw-log.yang defined log severity to Python logging level 
    # Not sure how to get Yang defined enum in Gi and so using enum value 0-7 directly
    log_levels_map= {
        0: logging.CRITICAL,
        1: logging.CRITICAL ,
        2: logging.CRITICAL ,
        3: logging.ERROR ,
        4: logging.WARNING,
        5: logging.INFO,
        6: logging.INFO ,
        7: logging.DEBUG
        }

    def __init__(self, category=None, log_hdl=None, file_name=None):
        """Create an instance of RwLogger

        Arguments:
            category - The category name to include in log events incase of Python logger.
            log_hdl - Use an existing rwlog handle instead of creating a new one.
            file_name - Pass in a filepath to use as the source file instead of
                        detecting it automatically.
        """
        logging.Handler.__init__(self)


        """ Set the default formatter to include a rwlog marker so we know the
        message are being sent to rwlog."""
        self.setFormatter("(rwlog)" + logging.BASIC_FORMAT)

        if file_name is None:
            frame = get_frame()
            file_name = get_caller_filename(frame)

        if category is not None:
            if not isinstance(category, six.string_types):
                raise TypeError("Category should be a string")

        self.category = category

        # GBoxed types don't accept constructors will arguments
        # RwLog.Ctx(file_name) will throw an error, so call
        # new directly
        if not log_hdl:
            log_hdl = RwLog.Ctx.new(file_name)

        self._log_hdl = log_hdl
        self.set_category('rw-generic')

        self._group_id = None

        self._rwlogd_inited = False
        shm_filename = self._log_hdl.get_shm_filter_name()
        self._shm_filename = os.path.join('/dev/shm',shm_filename)

        try:
            self._shm_fd = open(self._shm_filename,'rb')
            self._shm_data=mmap.mmap(self._shm_fd.fileno(),length=0,flags=mmap.MAP_SHARED,prot=mmap.PROT_READ)
        except Exception as e: 
            logger.error("Failed to open shm file: %s with exception %s",self._shm_filename,repr(e))
            print("Failed to open shm file: %s with exception %s",self._shm_filename,repr(e))

        self._log_serial_no = 0
        self._log_severity = 7  # Default sev is debug

    def set_group_id(self, group_id):
        if not isinstance(group_id, int):
            logger.warning("Group id not an int, could not set rwlogger group id")
            return

        self._group_id = group_id


    def set_category(self, category_name):
        """
           Set Log category name to be used.
           Arguments:
              category_name (String): Yang module name that has log notifications. module_name will be used as category_name
           Returns: None 
        """
        try:
            module_name = get_module_name_from_log_category(category_name)
            log_yang_module = importlib.import_module('gi.repository.' + module_name)
            if not log_yang_module:
                logger.error("Module %s is not found to be added as log category for %s", module_name, category_name)
                print("Module %s is not found to be added as log category for %s", module_name, category_name)
                return     
            for level in  RwLogger.level_event_cls_map.values():
                if not hasattr(log_yang_module, level):
                    logger.error("Module %s does not have required log notification for %s", module_name, level)
                    print("Module %s does not have required log notification for %s", module_name, level) 
                    return
            self._log_yang_module = log_yang_module 
            self._log_category_name = category_name

        except Exception as e:
            logger.exception("Caught error %s when trying to set log category (%s)",repr(e), category_name)

    def log_event(self, event, frames_above=0):
        try:
            if not event.template_params.filename:
                frame = get_frame(frames_above + 1)
                event.template_params.filename = get_caller_filename(frame)
                event.template_params.linenumber = get_caller_lineno(frame)

            if self._group_id is not None:
                event.template_params.call_identifier.groupcallid = self._group_id

            pb = event.to_ptr()
            self._log_hdl.proto_send(pb)

        except Exception:
            logger.exception("Caught error when attempting log event (%s)", event)

    def emit(self, record):
        # We check rwlog_shm pid is valid to infer shm is valid
        if self._rwlogd_inited is False:
            self._rwlogd_inited  = True
            rwlogd_pid = struct.unpack('I',self._shm_data[16:20])[0]
            try:
                os.kill(rwlogd_pid,0)
            except ProcessLookupError as e:
                # Rwlogd is not yet initialised;
                self._rwlogd_inited = False
            except Exception:
                logger.exception("Caught error when attempting to check rwlogd pid is present for log record (%s)", record)
            
        if self._rwlogd_inited is True:
            #offset 4-8 in shm holds log serial no and so we check it to know if 
            #there is any change in logging config            
            serial_no = struct.unpack('I',self._shm_data[4:8])[0]
            # If serial no has changed; update severity level
            if serial_no != self._log_serial_no:
                self._log_serial_no = serial_no
                # Currently we direclty get enum value as interger in Gi
                # Need to find out how to get enum type 
                self._log_severity = self._log_hdl.get_category_severity_level(self._log_category_name)

        try:    
            if record.levelno < RwLogger.log_levels_map[self._log_severity]:
                return

            if record.levelno in RwLogger.level_event_cls_map:
                event_cls = RwLogger.level_event_cls_map[record.levelno]
            elif record.levelno < logging.DEBUG:
                event_cls = RwLogger.level_event_cls_map[logging.DEBUG]
            elif record.levelno > logging.CRITICAL:
                event_cls = RwLogger.level_event_cls_map[logging.CRITICAL]
            else:
                logger.error("Could not find a suitable rwlog event mapping")
                return

            event = getattr(self._log_yang_module, event_cls)()
            event.log = record.getMessage()
            if self.category is not None:
                event.category = self.category
            event.template_params.linenumber = record.lineno
            event.template_params.filename = os.path.basename(record.pathname)

            self.log_event(event)

        except Exception:
            logger.exception("Caught error when attempting log record (%s)", record)
