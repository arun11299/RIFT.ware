# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
import asyncio
import logging
import uuid
import sys
import time

from enum import Enum
from collections import deque
from collections import defaultdict

from gi.repository import (
    RwNsrYang,
    NsrYang,
    RwVlrYang,
    VnfrYang,
    RwVnfrYang,
    RwNsmYang,
    RwDts as rwdts,
    RwTypes,
    ProtobufC,
)

import rift.mano.config_agent
import rift.tasklets

from . import rwnsm_conman as conman
from . import cloud
from . import publisher
from . import xpath
from . import rwnsm_conagent as conagent
from . import config_value_pool
from . import rwvnffgmgr


class NetworkServiceRecordState(Enum):
    """ Network Service Record State """
    INIT = 101
    VL_INIT_PHASE = 102
    VNF_INIT_PHASE = 103
    VNFFG_INIT_PHASE = 104
    RUNNING = 105
    TERMINATE = 106
    TERMINATE_RCVD = 107
    VL_TERMINATE_PHASE = 108
    VNF_TERMINATE_PHASE = 109
    VNFFG_TERMINATE_PHASE = 110
    TERMINATED = 111
    FAILED = 112


class ConfigAgentState(Enum):
    """ Config Agent Status """
    INIT=0
    CONFIGURING = 1
    CONFIGURED = 2
    FAILED = 3

class NetworkServiceRecordError(Exception):
    """ Network Service Record Error """
    pass


class NetworkServiceDescriptorError(Exception):
    """ Network Service Descriptor Error """
    pass


class VirtualNetworkFunctionRecordError(Exception):
    """ Virtual Network Function Record Error """
    pass


class NetworkServiceDescriptorNotFound(Exception):
    """ Cannot find Network Service Descriptor"""
    pass


class NetworkServiceDescriptorRefCountExists(Exception):
    """ Network Service Descriptor reference count exists """
    pass


class NetworkServiceDescriptorUnrefError(Exception):
    """ Failed to unref a network service descriptor """
    pass


class NsrInstantiationFailed(Exception):
    """ Failed to instantiate network service """
    pass


class VnfInstantiationFailed(Exception):
    """ Failed to instantiate virtual network function"""
    pass

class VnffgInstantiationFailed(Exception):
    """ Failed to instantiate virtual network function"""
    pass

class VnfDescriptorError(Exception):
    """Failed to instantiate virtual network function"""
    pass


class VlRecordState(Enum):
    """ VL Record State """
    INIT = 101
    INSTANTIATION_PENDING = 102
    ACTIVE = 103
    TERMINATE_PENDING = 104
    TERMINATED = 105
    FAILED = 106


class VnffgRecordState(Enum):
    """ VNFFG Record State """
    INIT = 101
    INSTANTIATION_PENDING = 102
    ACTIVE = 103
    TERMINATE_PENDING = 104
    TERMINATED = 105
    FAILED = 106


class VnffgRecord(object):
    """ Vnffg Records class"""
    def __init__(self, dts, log, loop, vnffgmgr, nsr, nsr_name, vnffgd_msg, sdn_account_name):

        self._dts = dts
        self._log = log
        self._loop = loop
        self._vnffgmgr = vnffgmgr
        self._nsr = nsr
        self._nsr_name = nsr_name
        self._vnffgd_msg = vnffgd_msg
        if sdn_account_name is None:
            self._sdn_account_name = ''
        else:  
            self._sdn_account_name = sdn_account_name

        self._vnffgr_id = str(uuid.uuid4())
        self._vnffgr_rsp_id = list()
        self._vnffgr_state = VnffgRecordState.INIT

    @property
    def id(self):
        """ VNFFGR id """
        return self._vnffgr_id

    @property
    def state(self):
        """ state of this VNF """
        return self._vnffgr_state

    def fetch_vnffgr(self):
        """
        Get VNFFGR message to be published
        """
        
        if self._vnffgr_state == VnffgRecordState.INIT:
            vnffgr_dict = {"id": self._vnffgr_id,
                           "nsd_id": self._nsr.nsd_id,
                           "vnffgd_id_ref": self._vnffgd_msg.id,
                           "vnffgd_name_ref": self._vnffgd_msg.name,
                           "sdn_account": self._sdn_account_name,
                            "operational_status": 'init',
                    }
            vnffgr = NsrYang.YangData_Nsr_NsInstanceOpdata_Nsr_Vnffgr.from_dict(vnffgr_dict)
        elif self._vnffgr_state == VnffgRecordState.TERMINATED:
            vnffgr_dict = {"id": self._vnffgr_id,
                           "nsd_id": self._nsr.nsd_id,
                           "vnffgd_id_ref": self._vnffgd_msg.id,
                           "vnffgd_name_ref": self._vnffgd_msg.name,
                           "sdn_account": self._sdn_account_name,
                            "operational_status": 'terminated',
                    }
            vnffgr = NsrYang.YangData_Nsr_NsInstanceOpdata_Nsr_Vnffgr.from_dict(vnffgr_dict)
        else:
            try:
                vnffgr = self._vnffgmgr.fetch_vnffgr(self._vnffgr_id)            
            except Exception:
                self._log.exception("Fetching VNFFGR for VNFFG with id %s failed", self._vnffgr_id)
                self._vnffgr_state = VnffgRecordState.FAILED
                vnffgr_dict = {"id": self._vnffgr_id,
                           "nsd_id": self._nsr.nsd_id,
                           "vnffgd_id_ref": self._vnffgd_msg.id,
                           "vnffgd_name_ref": self._vnffgd_msg.name,
                           "sdn_account": self._sdn_account_name,
                            "operational_status": 'failed',
                    }
                vnffgr = NsrYang.YangData_Nsr_NsInstanceOpdata_Nsr_Vnffgr.from_dict(vnffgr_dict)
        
        return vnffgr

    @asyncio.coroutine
    def vnffgr_create_msg(self):
        """ Virtual Link Record message for Creating VLR in VNS """
        vnffgr_dict = {"id": self._vnffgr_id,
                       "nsd_id": self._nsr.nsd_id,
                       "vnffgd_id_ref": self._vnffgd_msg.id,
                       "vnffgd_name_ref": self._vnffgd_msg.name,
                       "sdn_account": self._sdn_account_name,
                    }
        vnffgr = NsrYang.YangData_Nsr_NsInstanceOpdata_Nsr_Vnffgr.from_dict(vnffgr_dict)
        for rsp in self._vnffgd_msg.rsp:
            vnffgr_rsp = vnffgr.rsp.add()
            vnffgr_rsp.id = str(uuid.uuid4())
            vnffgr_rsp.name = self._nsr.name + '.' + rsp.name
            self._vnffgr_rsp_id.append(vnffgr_rsp.id)
            vnffgr_rsp.vnffgd_rsp_id_ref =  rsp.id
            vnffgr_rsp.vnffgd_rsp_name_ref = rsp.name
            for rsp_cp_ref in rsp.vnfd_connection_point_ref:
                vnfd =  [self._nsr._vnfds[vnfd_id] for vnfd_id in self._nsr._vnfds.keys() if vnfd_id == rsp_cp_ref.vnfd_id_ref]
                if len(vnfd) > 0 and vnfd[0].has_field('service_function_type'):
                    self._log.debug("Service Function Type for VNFD ID %s is %s",rsp_cp_ref.vnfd_id_ref, vnfd[0].service_function_type)
                else:
                    self._log.error("Service Function Type not available for VNFD ID %s; Skipping in chain",rsp_cp_ref.vnfd_id_ref)
                    continue
               
                vnfr_cp_ref =  vnffgr_rsp.vnfr_connection_point_ref.add()
                vnfr_cp_ref.member_vnf_index_ref = rsp_cp_ref.member_vnf_index_ref
                vnfr_cp_ref.hop_number = rsp_cp_ref.order
                vnfr_cp_ref.vnfd_id_ref =rsp_cp_ref.vnfd_id_ref
                vnfr_cp_ref.service_function_type = vnfd[0].service_function_type
                for nsr_vnfr in self._nsr.vnfrs.values():
                   if (nsr_vnfr.vnfd.id == vnfr_cp_ref.vnfd_id_ref and
                      nsr_vnfr.member_vnf_index == vnfr_cp_ref.member_vnf_index_ref):
                       vnfr_cp_ref.vnfr_id_ref = nsr_vnfr.id
                       vnfr_cp_ref.vnfr_name_ref = nsr_vnfr.name
                       vnfr_cp_ref.vnfr_connection_point_ref = rsp_cp_ref.vnfd_connection_point_ref
                       
                       vnfr = yield from self._nsr.fetch_vnfr(nsr_vnfr.xpath)
                       self._log.debug(" Received VNFR is %s", vnfr)
                       while vnfr.operational_status != 'running':
                           self._log.info("Received vnf op status is %s; retrying",vnfr.operational_status)
                           if vnfr.operational_status == 'failed':
                               self._log.error("Fetching VNFR for  %s failed", vnfr.id)
                               raise NsrInstantiationFailed("Failed NS %s instantiation due to VNFR %s failure" % (self.id, vnfr.id))
                           yield from asyncio.sleep(2, loop=self._loop)
                           vnfr = yield from self._nsr.fetch_vnfr(nsr_vnfr.xpath)
                           self._log.debug("Received VNFR is %s", vnfr)
                           
                       vnfr_cp_ref.connection_point_params.mgmt_address =  vnfr.mgmt_interface.ip_address
                       for cp in vnfr.connection_point:
                           if cp.name == vnfr_cp_ref.vnfr_connection_point_ref:
                               vnfr_cp_ref.connection_point_params.port_id = cp.connection_point_id
                               vnfr_cp_ref.connection_point_params.name = self._nsr.name + '.' + cp.name
                               for vdu in vnfr.vdur:
                                   for ext_intf in vdu.external_interface: 
                                       if ext_intf.name == vnfr_cp_ref.vnfr_connection_point_ref:
                                           vnfr_cp_ref.connection_point_params.vm_id =  vdu.vim_id
                                           self._log.debug("VIM ID for CP %s in VNFR %s is %s",cp.name,nsr_vnfr.id, 
                                                            vnfr_cp_ref.connection_point_params.vm_id)  
                                           break
                               
                               vnfr_cp_ref.connection_point_params.address =  cp.ip_address
                               vnfr_cp_ref.connection_point_params.port = 50000
                       for vdu in vnfr.vdur: 
                           pass
        self._log.info("VNFFGR msg to be sent is %s", vnffgr)
        return vnffgr

    @asyncio.coroutine
    def instantiate(self, xact):
        """ Instantiate this VNFFG """

        self._log.info("Instaniating VNFFGR with  vnffgd %s xact %s",
                         self._vnffgd_msg, xact)
        vnffgr_request = yield from self.vnffgr_create_msg()
    
        try: 
            vnffgr = self._vnffgmgr.create_vnffgr(vnffgr_request,self._vnffgd_msg.classifier)            
        except Exception:
            self._log.exception("VNFFG instantiation failed")
            self._vnffgr_state = VnffgRecordState.FAILED
            raise NsrInstantiationFailed("Failed NS %s instantiation due to VNFFGR %s failure" % (self.id, vnffgr_request.id))

        self._vnffgr_state = VnffgRecordState.INSTANTIATION_PENDING

        if vnffgr.operational_status == 'failed':
            self._log.error("NS Id:%s VNFFG creation failed for vnffgr id %s", self.id, vnffgr.id)
            self._vnffgr_state = VnffgRecordState.FAILED
            raise NsrInstantiationFailed("Failed NS %s instantiation due to VNFFGR %s failure" % (self.id, vnffgr.id))

        self._log.info("Instantiated VNFFGR :%s",vnffgr)
        self._vnffgr_state = VnffgRecordState.ACTIVE

        self._log.info("Invoking update_nsr_state to update NSR state for NSR ID: %s", self._nsr.id) 
        yield from self._nsr.update_nsr_state()

    def vnffgr_in_vnffgrm(self):
        """ Is there a VNFR record in VNFM """
        if (self._vnffgr_state == VnffgRecordState.ACTIVE or
                self._vnffgr_state == VnffgRecordState.INSTANTIATION_PENDING or
                self._vnffgr_state == VnffgRecordState.FAILED):
            return True

        return False


    @asyncio.coroutine
    def terminate(self, xact):
        """ Terminate this VNFFGR """
        if not self.vnffgr_in_vnffgrm():
            self._log.error("Ignoring terminate request for id %s in state %s",
                            self.id, self._vnffgr_state)
            return

        self._log.info("Terminating VNFFGR id:%s", self.id)
        self._vnffgr_state = VnffgRecordState.TERMINATE_PENDING

        self._vnffgmgr.terminate_vnffgr(self._vnffgr_id)
        
        self._vnffgr_state =  VnffgRecordState.TERMINATED
        self._log.debug("Terminated VNFFGR id:%s", self.id)


class VirtualLinkRecord(object):
    """ Virtual Link Records class"""
    def __init__(self, dts, log, loop, nsr_name, vld_msg, cloud_account_name):

        self._dts = dts
        self._log = log
        self._loop = loop
        self._nsr_name = nsr_name
        self._vld_msg = vld_msg
        self._cloud_account_name = cloud_account_name

        self._vlr_id = str(uuid.uuid4())
        self._state = VlRecordState.INIT

    @property
    def xpath(self):
        """ path for this object """
        return "D,/vlr:vlr-catalog/vlr:vlr[vlr:id = '{}']".format(self._vlr_id)

    @property
    def id(self):
        """ VLR id """
        return self._vlr_id

    @property
    def nsr_name(self):
        """ Get NSR name for this VL """
        return self.nsr_name

    @property
    def vld_msg(self):
        """ Virtual Link Desciptor """
        return self._vld_msg

    @property
    def name(self):
        """
        Get the name for this VLR.
        VLR name is "nsr name:VLD name"
        """
        if self.vld_msg.name == "multisite":
            # This is a temporary hack to identify manually provisioned inter-site network
            return self.vld_msg.name
        else:
            return self._nsr_name + "." + self.vld_msg.name

    @property
    def cloud_account_name(self):
        """ Cloud account that this VLR should be created in """
        return self._cloud_account_name

    @staticmethod
    def vlr_xpath(vlr):
        """ Get the VLR path from VLR """
        return (VirtualLinkRecord.XPATH + "[vlr:id = '{}']").format(vlr.id)

    @property
    def vlr_msg(self):
        """ Virtual Link Record message for Creating VLR in VNS """
        vld_fields = ["short_name",
                      "vendor",
                      "description",
                      "version",
                      "type_yang",
                     "provider_network"]

        vld_copy_dict = {k: v for k, v in self.vld_msg.as_dict().items()
                         if k in vld_fields}
        vlr_dict = {"id": self._vlr_id,
                    "name": self.name,
                    "cloud_account": self.cloud_account_name,
                    }

        vlr_dict.update(vld_copy_dict)

        vlr = RwVlrYang.YangData_Vlr_VlrCatalog_Vlr.from_dict(vlr_dict)
        return vlr

    def create_nsr_vlr_msg(self, vnfrs):
        """ The VLR message"""
        nsr_vlr = NsrYang.YangData_Nsr_NsInstanceOpdata_Nsr_Vlr()
        nsr_vlr.vlr_ref = self._vlr_id

        for conn in self.vld_msg.vnfd_connection_point_ref:
            for vnfr in vnfrs:
                if (vnfr.vnfd.id == conn.vnfd_id_ref and
                        vnfr.member_vnf_index == conn.member_vnf_index_ref):
                    cp_entry = nsr_vlr.vnfr_connection_point_ref.add()
                    cp_entry.vnfr_id = vnfr.id
                    cp_entry.connection_point = conn.vnfd_connection_point_ref

        return nsr_vlr

    @asyncio.coroutine
    def instantiate(self, xact):
        """ Instantiate this VL """

        self._log.debug("Instaniating VLR key %s, vld %s xact %s",
                        self.xpath, self._vld_msg, xact)
        vlr = None
        self._state = VlRecordState.INSTANTIATION_PENDING
        with self._dts.transaction(flags=0) as xact:
            block = xact.block_create()
            block.add_query_create(self.xpath, self.vlr_msg)
            self._log.debug("Executing VL create path:%s msg:%s",
                            self.xpath, self.vlr_msg)
            res_iter = yield from block.execute(now=True)
            for ent in res_iter:
                res = yield from ent
                vlr = res.result

            if vlr is None:
                self._state = VlRecordState.FAILED
                raise NsrInstantiationFailed("Failed NS %s instantiation due to empty response" % self.id)

        if vlr.operational_status == 'failed':
            self._log.debug("NS Id:%s VL creation failed for vlr id %s", self.id, vlr.id)
            self._state = VlRecordState.FAILED
            raise NsrInstantiationFailed("Failed NS %s instantiation due to VL %s failure" % (self.id, vlr.id))

        self._log.info("Instantiated VL with xpath %s and vlr:%s",
                       self.xpath, vlr)
        self._state = VlRecordState.ACTIVE

    def vlr_in_vns(self):
        """ Is there a VLR record in VNS """
        if (self._state == VlRecordState.ACTIVE or
                self._state == VlRecordState.INSTANTIATION_PENDING or
                self._state == VlRecordState.FAILED):
            return True

        return False

    @asyncio.coroutine
    def terminate(self, xact):
        """ Terminate this VL """
        if not self.vlr_in_vns():
            self._log.debug("Ignoring terminate request for id %s in state %s",
                            self.id, self._state)
            return

        self._log.debug("Terminating VL id:%s", self.id)
        self._state = VlRecordState.TERMINATE_PENDING
        block = xact.block_create()
        block.add_query_delete(self.xpath)
        yield from block.execute(flags=0, now=True)
        self._state = VlRecordState.TERMINATED
        self._log.debug("Terminated VL id:%s", self.id)


class VnfRecordState(Enum):
    """ Vnf Record State """
    INIT = 101
    INSTANTIATION_PENDING = 102
    ACTIVE = 103
    TERMINATE_PENDING = 104
    TERMINATED = 105
    FAILED = 106


class VirtualNetworkFunctionRecord(object):
    """ Virtual Network Function Record class"""
    XPATH = "D,/vnfr:vnfr-catalog/vnfr:vnfr"

    def __init__(self, dts, log, loop, vnfd, const_vnfd, nsr_name, cloud_account_name):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._vnfd = vnfd
        self._nsr_name = nsr_name
        self._const_vnfd = const_vnfd
        self._cloud_account_name = cloud_account_name

        self._config_agent = False
        self._config_status = ConfigAgentState.CONFIGURING
        self._mon_params = {}
        self._state = VnfRecordState.INIT
        self._vnfr_id = str(uuid.uuid4())
        self._vnfr = self.vnfr_msg

    @property
    def id(self):
        """ VNFR id """
        return self._vnfr_id

    @property
    def xpath(self):
        """ VNFR xpath """
        return "D,/vnfr:vnfr-catalog/vnfr:vnfr[vnfr:id = '{}']".format(self.id)

    @property
    def mon_param_xpath(self):
        """ VNFR monitoring param xpath """
        return self.xpath + "/vnfr:monitoring-param"

    @property
    def vnfr(self):
        """ VNFR xpath """
        return self._vnfr

    @property
    def vnfd(self):
        """ vnfd """
        return self._vnfd

    @property
    def active(self):
        """ Is this VNF actve """
        return True if self._state == VnfRecordState.ACTIVE else False

    @property
    def state(self):
        """ state of this VNF """
        return self._state

    @property
    def member_vnf_index(self):
        """ Member VNF index """
        return self._const_vnfd.member_vnf_index

    @property
    def nsr_name(self):
        """ NSR name"""
        return self._nsr_name

    @property
    def name(self):
        """ Name of this VNFR """
        return self._nsr_name + "." + self.vnfd.name + "." + str(self.member_vnf_index)

    @staticmethod
    def vnfr_xpath(vnfr):
        """ Get the VNFR path from VNFR """
        return (VirtualNetworkFunctionRecord.XPATH + "[vnfr:id = '{}']").format(vnfr.id)

    @property
    def config_status(self):
        self._log.debug("Map VNFR {} config status {} ({})".
                        format(self.name, self._config_status, self._config_agent))
        if not self._config_agent:
            return 'config_not_needed'
        if self._config_status == ConfigAgentState.CONFIGURED:
            return 'configured'
        if self._config_status == ConfigAgentState.FAILED:
            return 'failed'
        return 'configuring'

    @property
    def vnfr_msg(self):
        """ VNFR message for this VNFR """
        vnfd_fields = ["short_name",
                       "vendor",
                       "description",
                       "version",
                       "type_yang"]
        vnfd_copy_dict = {k: v for k, v in self._vnfd.as_dict().items()
                          if k in vnfd_fields}
        vnfr_dict = {"id": self.id,
                     "vnfd_ref": self.vnfd.id,
                     "name": self.name,
                     "cloud_account": self._cloud_account_name,
                     "config_status": self.config_status,
                     }
        vnfr_dict.update(vnfd_copy_dict)
        vnfr = RwVnfrYang.YangData_Vnfr_VnfrCatalog_Vnfr.from_dict(vnfr_dict)
        vnfr.member_vnf_index_ref = self.member_vnf_index
        vnfr.vnf_configuration.from_dict(self._const_vnfd.vnf_configuration.as_dict())

        if self._vnfd.mgmt_interface.has_field("port"):
            vnfr.mgmt_interface.port = self._vnfd.mgmt_interface.port

        # UI expects the monitoring param field to exist
        vnfr.monitoring_param = []

        self._log.debug("Get vnfr_msg for VNFR {} : {}".
                        format(self.name, vnfr))
        return vnfr

    @property
    def msg(self):
        """ message for this VNFR """
        return self.id

    @asyncio.coroutine
    def update_vnfm(self):
        self._vnfr = self.vnfr_msg
        # Publish only after VNFM has the VNFR created
        if self._config_status != ConfigAgentState.INIT:
            self._log.debug("Send an update to VNFM for VNFR {} with {}".
                            format(self.name, self.vnfr))
            yield from self._dts.query_update(self.xpath,
                                              0,
                                              self.vnfr)

    @asyncio.coroutine
    def set_config_status(self, status, agent=False):
        self._log.debug("Update VNFR {} from {} ({}) to {} ({})".
                        format(self.name, self._config_status,
                               self._config_agent, status, agent))
        if self._config_status != status or self._config_agent != agent:
            self._config_status = status
            self._config_agent = agent
            self._log.debug("Updated VNFR {} status to {} ({})".
                            format(self.name, status, agent))
            yield from self.update_vnfm()

    def is_configured(self):
        if self._config_status == ConfigAgentState.CONFIGURED:
            return True
        return False

    @asyncio.coroutine
    def instantiate(self, nsr, xact):
        """ Instantiate this VL """

        self._log.debug("Instaniating VNFR key %s, vnfd %s, xact %s",
                        self.xpath, self._vnfd, xact)

        self._log.debug("Create VNF with xpath %s and vnfr %s",
                        self.xpath, self.vnfr)

        self._state = VnfRecordState.INSTANTIATION_PENDING

        def find_vlr_for_cp(conn):
            """ Find VLR for the given connection point """
            for vlr in nsr.vlrs:
                for vnfd_cp in vlr.vld_msg.vnfd_connection_point_ref:
                    if (vnfd_cp.vnfd_id_ref == self._vnfd.id and
                            vnfd_cp.vnfd_connection_point_ref == conn.name and
                            vnfd_cp.member_vnf_index_ref == self.member_vnf_index):
                        self._log.debug("Found VLR for cp_name:%s and vnf-index:%d",
                                        conn.name, self.member_vnf_index)
                        return vlr
            return None

        # For every connection point in the VNFD fill in the identifier
        for conn_p in self._vnfd.connection_point:
            cpr = VnfrYang.YangData_Vnfr_VnfrCatalog_Vnfr_ConnectionPoint()
            cpr.name = conn_p.name
            cpr.type_yang = conn_p.type_yang
            vlr_ref = find_vlr_for_cp(conn_p)
            if vlr_ref is None:
                msg = "Failed to find VLR for cp = %s" % conn_p.name
                self._log.debug("%s", msg)
#                raise VirtualNetworkFunctionRecordError(msg)
                continue

            cpr.vlr_ref = vlr_ref.id
            self.vnfr.connection_point.append(cpr)
            self._log.debug("Connection point [%s] added, vnf id=%s vnfd id=%s",
                            cpr, self.vnfr.id, self.vnfr.vnfd_ref)

        yield from self._dts.query_create(self.xpath,
                                          # 0,   # this is sub
                                          0,   # this is sub
                                          self.vnfr)

        self._log.info("Created VNF with xpath %s and vnfr %s",
                       self.xpath, self.vnfr)

        self._log.info("Instantiated VNFR with xpath %s and vnfd %s, vnfr %s",
                       self.xpath, self._vnfd, self.vnfr)

    @asyncio.coroutine
    def update(self, vnfr):
        """ Update this VNFR"""
        curr_vnfr = self._vnfr
        self._vnfr = vnfr
        if vnfr.operational_status == "running":
            if curr_vnfr.operational_status != "running":
                yield from self.is_active()
        elif vnfr.operational_status == "failed":
            yield from self.instantiation_failed()

    @asyncio.coroutine
    def is_active(self):
        """ This VNFR is active """
        self._log.debug("VNFR %s is active", self._vnfr_id)
        self._state = VnfRecordState.ACTIVE

    @asyncio.coroutine
    def instantiation_failed(self):
        """ This VNFR instantiation failed"""
        self._log.error("VNFR %s instantiation failed", self._vnfr_id)
        self._state = VnfRecordState.FAILED

    def vnfr_in_vnfm(self):
        """ Is there a VNFR record in VNFM """
        if (self._state == VnfRecordState.ACTIVE or
                self._state == VnfRecordState.INSTANTIATION_PENDING or
                self._state == VnfRecordState.FAILED):
            return True

        return False

    @asyncio.coroutine
    def terminate(self, xact):
        """ Terminate this VNF """
        if not self.vnfr_in_vnfm():
            self._log.debug("Ignoring terminate request for id %s in state %s",
                            self.id, self._state)
            return

        self._log.debug("Terminating VNF id:%s", self.id)
        self._state = VnfRecordState.TERMINATE_PENDING
        block = xact.block_create()
        block.add_query_delete(self.xpath)
        yield from block.execute(flags=0, now=True)
        self._state = VnfRecordState.TERMINATED
        self._log.debug("Terminated VNF id:%s", self.id)

    @asyncio.coroutine
    def get_monitoring_param(self):
        """ Fetch monitoring params """
        res_iter = yield from self._dts.query_read(self.mon_param_xpath, rwdts.Flag.MERGE)
        monp_list = []
        for ent in res_iter:
            res = yield from ent
            monp = res.result
            if monp.id in self._mon_params:
                if monp.has_field("value_integer"):
                    self._mon_params[monp.id].value_integer = monp.value_integer
                if monp.has_field("value_decimal"):
                    self._mon_params[monp.id].value_decimal = monp.value_decimal
                if monp.has_field("value_string"):
                    self._mon_params[monp.id].value_string = monp.value_string
            else:
                self._mon_params[monp.id] = NsrYang.YangData_Nsr_NsInstanceOpdata_Nsr_VnfMonitoringParam_MonitoringParam.from_dict(monp.as_dict())
            monp_list.append(self._mon_params[monp.id])
        return monp_list


class NetworkServiceStatus(object):
    """ A class representing the Network service's status """
    MAX_EVENTS_RECORDED = 10
    """ Network service Status class"""
    def __init__(self, dts, log, loop):
        self._dts = dts
        self._log = log
        self._loop = loop

        self._state = NetworkServiceRecordState.INIT
        self._events = deque([])

    def record_event(self, evt, evt_desc):
        """ Record an event """
        self._log.debug("Recording event - evt %s, evt_descr %s len = %s",
                        evt, evt_desc, len(self._events))
        if len(self._events) >= NetworkServiceStatus.MAX_EVENTS_RECORDED:
            self._events.popleft()
        self._events.append((int(time.time()), evt, evt_desc))

    def set_state(self, state):
        """ set the state of this status object """
        self._state = state

    def yang_str(self):
        """ Return the state as a yang enum string """
        state_to_str_map = {"INIT": "init",
                            "VL_INIT_PHASE": "vl_init_phase",
                            "VNF_INIT_PHASE": "vnf_init_phase",
                            "VNFFG_INIT_PHASE": "vnffg_init_phase",
                            "RUNNING": "running",
                            "TERMINATE": "terminate",
                            "TERMINATE_RCVD": "vl_terminate_phase",
                            "VL_TERMINATE_PHASE": "vl_terminate_phase",
                            "VNF_TERMINATE_PHASE": "vnf_terminate_phase",
                            "VNFFG_TERMINATE_PHASE": "vnffg_terminate_phase",
                            "TERMINATED": "terminated",
                            "FAILED": "failed"}
        return state_to_str_map[self._state.name]

    @property
    def state(self):
        """ State of this status object """
        return self._state

    @property
    def msg(self):
        """ Network Service Record as a message"""
        event_list = []
        idx = 1
        for entry in self._events:
            event = RwNsrYang.YangData_Nsr_NsInstanceOpdata_Nsr_OperationalEvents()
            event.id = idx
            idx += 1
            event.timestamp, event.event, event.description = entry
            event_list.append(event)
        return event_list


class NetworkServiceRecord(object):
    """ Network service record """
    XPATH = "D,/nsr:ns-instance-opdata/nsr:nsr"

    def __init__(self, dts, log, loop, nsm, nsm_plugin, config_agent_plugins, nsr_cfg_msg,sdn_account_name):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._nsm = nsm
        self._nsr_cfg_msg = nsr_cfg_msg
        self._nsm_plugin = nsm_plugin
        self._config_agent_plugins = config_agent_plugins
        self._sdn_account_name = sdn_account_name

        self._nsd = None
        self._nsr_msg = None
        self._nsr_regh = None
        self._vlrs = []
        self._vnfrs = {}
        self._vnfds = {}
        self._vnffgrs = {} 
        self._param_pools = {}
        self._create_time = int(time.time())
        self._op_status = NetworkServiceStatus(dts, log, loop)
        self._mon_params = defaultdict(NsrYang.YangData_Nsr_NsInstanceOpdata_Nsr_VnfMonitoringParam)
        self._config_status = ConfigAgentState.CONFIGURING
        self._config_update = None
        self._job_id = 0

        # Initalise the state to init
        # The NSR moves through the following transitions
        # 1. INIT -> VLS_READY once all the VLs in the NSD are created
        # 2. VLS_READY - VNFS_READY when all the VNFs in the NSD are created
        # 3. VNFS_READY - READY when the NSR is published

        self.set_state(NetworkServiceRecordState.INIT)

        self.substitute_input_parameters = InputParameterSubstitution(self._log)

    @property
    def nsm_plugin(self):
        """ NSM Plugin """
        return self._nsm_plugin

    @property
    def config_agent_plugins(self):
        """ Config agent plugin list """
        return self._config_agent_plugins

    def set_state(self, state):
        """ Set state for this NSR"""
        self._log.debug("Setting state to %s", state)
        self._op_status.set_state(state)

    @property
    def id(self):
        """ Get id for this NSR"""
        return self._nsr_cfg_msg.id

    @property
    def name(self):
        """ Name of this network service record """
        return self._nsr_cfg_msg.name

    @property
    def nsd_id(self):
        """ Get nsd id for this NSR"""
        return self._nsr_cfg_msg.nsd_ref

    @property
    def cloud_account_name(self):
        return self._nsr_cfg_msg.cloud_account

    @property
    def state(self):
        """State of this NetworkServiceRecord"""
        return self._op_status.state

    def record_event(self, evt, evt_desc, state=None):
        """ Record an event """
        self._op_status.record_event(evt, evt_desc)
        if state is not None:
            self.set_state(state)

    @property
    def active(self):
        """ Is this NSR active ?"""
        return True if self._op_status.state == NetworkServiceRecordState.RUNNING else False

    @property
    def vlrs(self):
        """ VLRs associated with this NSR"""
        return self._vlrs

    @property
    def vnfrs(self):
        """ VNFRs associated with this NSR"""
        return self._vnfrs

    @property
    def vnffgrs(self):
        """ VNFFGRs associated with this NSR"""
        return self._vnffgrs

    @property
    def param_pools(self):
        """ Parameter value pools associated with this NSR"""
        return self._param_pools

    @property
    def nsd(self):
        """ NSD for this NSR """
        return self._nsd

    @property
    def nsd_msg(self):
        return self._nsd.msg

    @property
    def job_id(self):
        ''' Get a new job id for config primitive'''
        self._job_id += 1
        return self._job_id

    @property
    def config_status(self):
        """ Config status for NSR """
        return  self._config_status

    def __str__(self):
        return "NSR(name={}, nsd_id={}, cloud_account={})".format(
                self.name, self.nsd_id, self.cloud_account_name
                )

    @asyncio.coroutine
    def invoke_config_agent_plugins(self, method, *args):
        # Invoke the methods on all config agent plugins registered
        for agent in self._config_agent_plugins:
            try:
                self._log.debug("Invoke %s on %s" % (method, agent))
                yield from agent.invoke(method, *args)
            except Exception:
                self._log.warning("Error invoking %s on %s : %s" %
                                  (method, agent, sys.exc_info()))
                pass

    @asyncio.coroutine
    def instantiate_vls(self, xact):
        """
        This function instantiates VLs for every VL in this Network Service
        """
        self._log.debug("Instantiating %d VLs in NSD id %s", len(self._vlrs),
                        self.id)
        for vlr in self._vlrs:
            yield from self.nsm_plugin.instantiate_vl(self, vlr, xact)
            yield from self.invoke_config_agent_plugins('notify_instantiate_vl', self.id, vlr, xact)

    @asyncio.coroutine
    def create(self):
        """ Create this network service"""
        yield from self.invoke_config_agent_plugins('notify_create_nsr', self.id, self._nsd)
        # Create virtual links  for all the external vnf
        # connection points in this NS
        yield from self.create_vls()
        # Create VNFs in this network service
        yield from self.create_vnfs()
        # Create VNFFG for network service
        yield from self.create_vnffgs()

        self.create_param_pools()

    @asyncio.coroutine
    def create_vnffgs(self):
        """ This function creates VNFFGs for every VNFFG in the NSD
        associated with this NSR"""

        for vnffgd in self.nsd.msg.vnffgd:
            self._log.debug("Found vnffgd %s in nsr id %s", vnffgd, self.id)
            vnffgr = VnffgRecord(self._dts,
                              self._log,
                              self._loop,
                              self._nsm._vnffgmgr,
                              self,
                              self.name,
                              vnffgd,
                              self._sdn_account_name 
                              )
            self._vnffgrs[vnffgr.id] = vnffgr

    @asyncio.coroutine
    def create_vls(self):
        """ This function creates VLs for every VLD in the NSD
        associated with this NSR"""

        for vld in self.nsd.msg.vld:
            self._log.debug("Found vld %s in nsr id %s", vld, self.id)
            vlr = VirtualLinkRecord(self._dts,
                                    self._log,
                                    self._loop,
                                    self.name,
                                    vld,
                                    self.cloud_account_name
                                    )
            self._vlrs.append(vlr)
            yield from self.invoke_config_agent_plugins('notify_create_vls', self.id, vld, vlr)

    def is_vnfr_config_agent_managed(self, vnfr):
        if vnfr._config_agent:
            return True
        for agent in self._config_agent_plugins:
            try:
                if agent.is_vnfr_managed(vnfr.id):
                    return True
            except Exception as e:
                self._log.info("Check if VNFR {} is config agent managed: {}".
                               format(vnfr.name, e))
        return False

    @asyncio.coroutine
    def create_vnfs(self):
        """
        This function creates VNFs for every VNF in the NSD
        associated with this NSR
        """

        self._log.debug("Creating %u VNFs associated with this NS id %s",
                        len(self.nsd.msg.constituent_vnfd), self.id)

        # Fetch the VNFD associated with this VNF
        @asyncio.coroutine
        def fetch_vnfd(vnfd_ref):
            """  Fetch vnfd for the passed vnfd ref """
            return (yield from self._nsm.get_vnfd(vnfd_ref))

        for const_vnfd in self.nsd.msg.constituent_vnfd:
            vnfd = None
            vnfd_id = const_vnfd.vnfd_id_ref
            if vnfd_id in self._vnfds:
                vnfd = self._vnfds[vnfd_id]
            else:
                vnfd = yield from fetch_vnfd(vnfd_id)
                self._vnfds[vnfd_id] = vnfd
            if vnfd is None:
                self._log.debug("NS instantiation failed for NSR id %s"
                                "Cannot find VNF descriptor with VNFD id %s",
                                self.id, vnfd_id)
                err = ("Failed NS instantiation-VNF desc not found:"
                       "nsr id %s, vnfd id %s" % (self.id, vnfd_id))

                raise NetworkServiceRecordError(err)

            vnfr = VirtualNetworkFunctionRecord(self._dts,
                                                self._log,
                                                self._loop,
                                                vnfd,
                                                const_vnfd,
                                                self.name,
                                                self.cloud_account_name,
                                                )
            if vnfr.id in self._vnfrs:
                err = "VNF with VNFR id %s already in vnf list" % (vnfr.id,)
                raise NetworkServiceRecordError(err)

            self._vnfrs[vnfr.id] = vnfr
            self._nsm.vnfrs[vnfr.id] = vnfr

            yield from self.invoke_config_agent_plugins('notify_create_vnfr',
                                                        self.id,
                                                        vnfr)
            if self.is_vnfr_config_agent_managed(vnfr):
                self._log.debug("VNFR %s is in configuring state" % vnfr.name)
                yield from vnfr.set_config_status(ConfigAgentState.INIT, agent=True)
            else:
                self._log.debug("VNFR %s is not config agent" % vnfr.name)
                yield from vnfr.set_config_status(ConfigAgentState.INIT)

            self._log.debug("Added VNFR %s to NSM VNFR list with id %s",
                            vnfr,
                            vnfr.id)

    def create_param_pools(self):
        for param_pool in self.nsd.msg.parameter_pool:
            self._log.debug("Found parameter pool %s in nsr id %s", param_pool, self.id)

            start_value = param_pool.range.start_value
            end_value = param_pool.range.end_value
            if end_value < start_value:
                raise NetworkServiceRecordError(
                        "Parameter pool %s has invalid range (start: {}, end: {})".format(
                            start_value, end_value
                            )
                        )

            self._param_pools[param_pool.name] = config_value_pool.ParameterValuePool(
                    self._log,
                    param_pool.name,
                    range(start_value, end_value)
                    )


    @asyncio.coroutine
    def fetch_vnfr(self, vnfr_path):
        """ Fetch VNFR record """
        vnfr = None
        self._log.debug("Fetching VNFR with key %s while instantiating %s",
                        vnfr_path, self.id)
        res_iter = yield from self._dts.query_read(vnfr_path, rwdts.Flag.MERGE)

        for ent in res_iter:
            res = yield from ent
            vnfr = res.result

        return vnfr

    @asyncio.coroutine
    def instantiate_vnfs(self, xact):
        """
        This function instantiates VNFs for every VNF in this Network Service
        """
        self._log.debug("Instantiating %u VNFs in NS %s",
                        len(self.nsd.msg.constituent_vnfd), self.id)
        for vnf in self._vnfrs.values():
            self._log.debug("Instantiating VNF: %s in NS %s", vnf, self.id)
            yield from self.nsm_plugin.instantiate_vnf(self, vnf, xact)
            vnfr = yield from self.fetch_vnfr(vnf.xpath)
            if vnfr.operational_status == 'failed':
                self._log.debug("Instatiation of VNF %s failed", vnf.id)
                raise VnfInstantiationFailed("Failed to instantiate vnf %s", vnf.id)
            yield from self.invoke_config_agent_plugins('notify_instantiate_vnf', self.id, vnf, xact)

    @asyncio.coroutine
    def instantiate_vnffgs(self, xact):
        """
        This function instantiates VNFFGs for every VNFFG in this Network Service
        """
        self._log.debug("Instantiating %u VNFFGs in NS %s",
                        len(self.nsd.msg.vnffgd), self.id)
        for vnffg in self._vnffgrs.values():
            self._log.debug("Instantiating VNFFG: %s in NS %s", vnffg, self.id)
            yield from vnffg.instantiate(xact) 
            #vnffgr = vnffg.fetch_vnffgr()
            #if vnffgr.operational_status == 'failed':
            if vnffg.state == VnffgRecordState.FAILED:
                self._log.debug("Instatiation of VNFFG %s failed", vnffg.id)
                raise VnffgInstantiationFailed("Failed to instantiate vnffg %s", vnffg.id)

    @asyncio.coroutine
    def publish(self):
        """ This function publishes this NSR """
        self._nsr_msg = self.create_msg()
        self._log.debug("Publishing the NSR with xpath %s and nsr %s",
                        self.nsr_xpath,
                        self._nsr_msg)
        with self._dts.transaction() as xact:
            yield from self._nsm.nsr_handler.update(xact, self.nsr_xpath, self._nsr_msg)
        self._log.info("Published the NSR with xpath %s and nsr %s",
                       self.nsr_xpath,
                       self._nsr_msg)

    @asyncio.coroutine
    def unpublish(self, xact):
        """ Unpublish this NSR object """
        self._log.debug("Unpublishing Network service id %s", self.id)
        yield from self._nsm.nsr_handler.delete(xact, self.nsr_xpath)

    @property
    def nsr_xpath(self):
        """ Returns the xpath associated with this NSR """
        return(
            "D,/nsr:ns-instance-opdata" +
            "/nsr:nsr[nsr:ns-instance-config-ref = '{}']"
            ).format(self.id)

    @staticmethod
    def xpath_from_nsr(nsr):
        """ Returns the xpath associated with this NSR  op data"""
        return (NetworkServiceRecord.XPATH +
                "[nsr:ns-instance-config-ref = '{}']").format(nsr.id)

    @property
    def nsd_xpath(self):
        """ Return NSD config xpath."""
        return(
            "C,/nsd:nsd-catalog" +
            "/nsd:nsd[nsd:id = '{}']"
            ).format(self.nsd_id)

    @asyncio.coroutine
    def instantiate(self, xact):
        """"Instantiates a NetworkServiceRecord.

        This function instantiates a Network service
        which involves the following steps,

        * Fetch the NSD associated with NSR from DTS.
        * Merge the NSD withe NSR config to begin instantiating the NS.
        * Instantiate every VL in NSD by sending create VLR request to DTS.
        * Instantiate every VNF in NSD by sending create VNF reuqest to DTS.
        * Publish the NSR details to DTS

        Arguments:
            nsr:  The NSR configuration request containing nsr-id and nsd_ref
            xact: The transaction under which this instatiation need to be
                  completed

        Raises:
            NetworkServiceRecordError if the NSR creation fails

        Returns:
            No return value
        """

        self._log.debug("Instatiating NS - %s xact - %s", self, xact)

        # Move the state to INIITALIZING
        self.set_state(NetworkServiceRecordState.INIT)

        event_descr = "Instatiation Request Received NSR Id:%s" % self.id
        self.record_event("instantiating", event_descr)

        # Find the NSD
        self._nsd = self._nsm.get_nsd_ref(self.nsd_id)
        event_descr = "Fetched NSD with descriptor id %s" % self.nsd_id
        self.record_event("nsd-fetched", event_descr)

        if self._nsd is None:
            msg = "Failed to fetch NSD with nsd-id [%s] for nsr-id %s"
            self._log.debug(msg, self.nsd_id, self.id)
            raise NetworkServiceRecordError(self)

        self._log.debug("Got nsd result %s", self._nsd)

        # Sbustitute any input parameters
        self.substitute_input_parameters(self._nsd._nsd, self._nsr_cfg_msg)

        # Create the record
        yield from self.create()

        # Publish the NSR to DTS
        yield from self.publish()
        yield from self.invoke_config_agent_plugins('notify_instantiate_ns', self.id)

        @asyncio.coroutine
        def do_instantiate():
            """
                Instantiate network service
            """
            self._log.debug("Instantiating VLs nsr id [%s] nsd id [%s]",
                            self.id, self.nsd_id)

            # instantiate the VLs
            event_descr = ("Instantiating %s external VLs for NSR id %s" %
                           (len(self.nsd.msg.vld), self.id))
            self.record_event("begin-external-vls-instantiation", event_descr)

            self.set_state(NetworkServiceRecordState.VL_INIT_PHASE)

            try:
                yield from self.instantiate_vls(xact)
            except Exception:
                self._log.exception("VL instantiation failed")
                yield from self.instantiation_failed()
                return

            # Publish the NSR to DTS
            yield from self.publish()

            event_descr = ("Finished instantiating %s external VLs for NSR id %s" %
                           (len(self.nsd.msg.vld), self.id))
            self.record_event("end-external-vls-instantiation", event_descr)

            # Move the state to VLS_READY
            self.set_state(NetworkServiceRecordState.VNF_INIT_PHASE)

            self._log.debug("Instantiating VNFs  ...... nsr[%s], nsd[%s]",
                            self.id, self.nsd_id)

            # instantiate the VNFs
            event_descr = ("Instantiating %s VNFS for NSR id %s" %
                           (len(self.nsd.msg.constituent_vnfd), self.id))

            self.record_event("begin-vnf-instantiation", event_descr)

            try:
                yield from self.instantiate_vnfs(xact)
            except Exception:
                self._log.exception("VNF instantiation failed")
                yield from self.instantiation_failed()
                return

            self._log.debug(" Finished instantiating %d VNFs for NSR id %s",
                           len(self.nsd.msg.constituent_vnfd), self.id)

            event_descr = ("Finished instantiating %s VNFs for NSR id %s" %
                           (len(self.nsd.msg.constituent_vnfd), self.id))
            self.record_event("end-vnf-instantiation", event_descr)

            if len(self.vnffgrs) > 0:
                self._log.debug("Instantiating VNFFGRs  ...... nsr[%s], nsd[%s]",
                            self.id, self.nsd_id)

                # instantiate the VNFs
                event_descr = ("Instantiating %s VNFFGS for NSR id %s" %
                           (len(self.nsd.msg.vnffgd), self.id))

                self.record_event("begin-vnffg-instantiation", event_descr)

                try:
                    yield from self.instantiate_vnffgs(xact)
                except Exception:
                    self._log.exception("VNFFG instantiation failed")
                    yield from self.instantiation_failed()
                    return

                self._log.debug(" Finished instantiating %d VNFFGs for NSR id %s",
                           len(self.nsd.msg.vnffgd), self.id)
                event_descr = ("Finished instantiating %s VNFFGDs for NSR id %s" %
                           (len(self.nsd.msg.vnffgd), self.id))
                self.record_event("end-vnffg-instantiation", event_descr)


            # Give the plugin a chance to deploy the network service now that all
            # virtual links and vnfs are instantiated
            try:
                yield from self.nsm_plugin.deploy(self._nsr_msg)
            except Exception:
                self._log.exception("NSM deploy failed")
                yield from self.instantiation_failed()
                return

            self._log.debug("Publishing  NSR...... nsr[%s], nsd[%s]",
                            self.id, self.nsd_id)

            # Publish the NSR to DTS
            yield from self.publish()

            event_descr = ("NSR in running state for NSR id %s" % self.id)
            self.record_event("ns-running", event_descr)

            self._log.debug("Published  NSR...... nsr[%s], nsd[%s]",
                            self.id, self.nsd_id)

        self._loop.create_task(do_instantiate())

    @asyncio.coroutine
    def get_vnfr_config_status(self, vnfr):
        if vnfr.is_configured():
            return ConfigAgentState.CONFIGURED

        # Check if config agent has finished configuring
        status = ConfigAgentState.CONFIGURED
        for agent in self._config_agent_plugins:
            try:
                rc = yield from agent.get_status(vnfr.id)
                #self._log.debug("config status for VNF {} is {}".
                #                format(vnfr.id, rc))
                if rc == 'configuring':
                    status = ConfigAgentState.CONFIGURING
                    break
                elif rc == 'failed':
                    status = ConfigAgentState.FAILED
                    break

            except Exception as e:
                self._log.debug("Error in checking config state for VNF %s, e=%s" % (vnfr.name, e))
                status = ConfigAgentState.CONFIGURING

        yield from vnfr.set_config_status(status, True)
        if status == ConfigAgentState.CONFIGURED:
            # Re-apply initial config
            self._log.debug("VNF active. Apply initial config for vnfr {}".format(vnfr.name))
            yield from self.invoke_config_agent_plugins('apply_initial_config',
                                                        vnfr.name, vnfr)

        return status

    @asyncio.coroutine
    def update_config_status(self):
        ''' Check if all VNFRs are configured '''
        self._log.debug("Check all VNFRs are configured for ns %s" % self.name)

        if self._config_status == ConfigAgentState.CONFIGURED:
            return ConfigAgentState.CONFIGURED

        # Handle reload scenarios
        for vnfr in self._vnfrs.values():
            if self.is_vnfr_config_agent_managed(vnfr):
                yield from vnfr.set_config_status(ConfigAgentState.CONFIGURING, True)
            else:
                # Set non config agent managed to configured
                vnfr._config_status = ConfigAgentState.CONFIGURED

        recheck = True
        while recheck:
            for vnfr in self._vnfrs.values():
                config_status = yield from self.get_vnfr_config_status(vnfr)
                self._log.debug("config status for VNF {} is {}".
                                format(vnfr.name, config_status))
                if config_status !=  ConfigAgentState.CONFIGURED:
                    break
            self._config_status = config_status
            if config_status in [ConfigAgentState.CONFIGURED, ConfigAgentState.FAILED]:
                yield from self.publish()
                return
            else:
                yield from asyncio.sleep(10, loop=self._loop)


    @asyncio.coroutine
    def is_active(self):
        """ This NS is active """
        self._log.debug("Network service %s is active ", self.id)
        self.set_state(NetworkServiceRecordState.RUNNING)

        # Publish the NSR to DTS
        yield from self.publish()
        yield from self._nsm.so_obj.notify_nsr_up(self.id)
        yield from self.invoke_config_agent_plugins('notify_nsr_active', self.id, self._vnfrs)
        self._config_update = self._loop.create_task(self.update_config_status())
        self._log.debug("Created tasklet %s" % self._config_update)

    @asyncio.coroutine
    def instantiation_failed(self):
        """ The NS instantiation failed"""
        self._log.debug("Network service %s instantiation failed", self.id)
        self.set_state(NetworkServiceRecordState.FAILED)

        event_descr = "Instantiation of NS %s failed" % self.id
        self.record_event("ns-failed", event_descr)

        # Publish the NSR to DTS
        yield from self.publish()

    @asyncio.coroutine
    def terminate(self, xact):
        """ Terminate a NetworkServiceRecord."""
        def terminate_vnfrs(xact):
            """ Terminate VNFRS in this network service """
            self._log.debug("Terminating VNFs in network service %s", self.id)
            for vnfr in self.vnfrs.values():
                yield from self.nsm_plugin.terminate_vnf(vnfr, xact)
                yield from self.invoke_config_agent_plugins('notify_terminate_vnf', self.id, vnfr, xact)

        def terminate_vnffgrs(xact):
            """ Terminate VNFFGRS in this network service """
            self._log.debug("Terminating VNFFGRs in network service %s", self.id)
            for vnffgr in self.vnffgrs.values():
                yield from vnffgr.terminate(xact)


        def terminate_vlrs(xact):
            """ Terminate VLRs in this netork service """
            self._log.debug("Terminating VLs in network service %s", self.id)
            for vlr in self.vlrs:
                yield from self.nsm_plugin.terminate_vl(vlr, xact)
                yield from self.invoke_config_agent_plugins('notify_terminate_vl', self.id, vlr, xact)

        self._log.debug("Terminating network service id %s", self.id)

        # Move the state to VNF_TERMINATE_PHASE
        self.set_state(NetworkServiceRecordState.TERMINATE)
        event_descr = "Terminate rcvd for NS Id:%s" % self.id
        self.record_event("terminate-rcvd", event_descr)

        # Move the state to VNF_TERMINATE_PHASE
        self._log.debug("Terminating VNFFGs in NS ID: %s",self.id)
        self.set_state(NetworkServiceRecordState.VNFFG_TERMINATE_PHASE)
        event_descr = "Terminating VNFFGS in NS Id:%s" % self.id
        self.record_event("terminating-vnffgss", event_descr)
        yield from terminate_vnffgrs(xact)

        # Move the state to VNF_TERMINATE_PHASE
        self.set_state(NetworkServiceRecordState.VNF_TERMINATE_PHASE)
        event_descr = "Terminating VNFS in NS Id:%s" % self.id
        self.record_event("terminating-vnfs", event_descr)
        yield from terminate_vnfrs(xact)

        # Move the state to VL_TERMINATE_PHASE
        self.set_state(NetworkServiceRecordState.VL_TERMINATE_PHASE)
        event_descr = "Terminating VLs in NS Id:%s" % self.id
        self.record_event("terminating-vls", event_descr)
        yield from terminate_vlrs(xact)

        yield from self.nsm_plugin.terminate_ns(self, xact)

        # Move the state to TERMINATED
        self.set_state(NetworkServiceRecordState.TERMINATED)
        event_descr = "Terminated NS Id:%s" % self.id
        self.record_event("terminated", event_descr)
        self._loop.create_task(self._nsm.so_obj.notify_nsr_down(self.id))
        yield from self.invoke_config_agent_plugins('notify_terminate_ns', self.id)
        self._log.debug("Checking tasklet %s" % (self._config_update))
        if self._config_update:
            self._config_update.print_stack()
            self._config_update.cancel()
            self._config_update = None

    def enable(self):
        """"Enable a NetworkServiceRecord."""
        pass

    def disable(self):
        """"Disable a NetworkServiceRecord."""
        pass

    def map_config_status(self):
        self._log.debug("Config status for ns {} is {}".
                        format(self.name, self._config_status))
        if self._config_status == ConfigAgentState.CONFIGURING:
            return 'configuring'
        if self._config_status == ConfigAgentState.FAILED:
            return 'failed'
        return 'configured'

    def create_msg(self):
        """ The network serice record as a message """
        nsr_dict = {"ns_instance_config_ref": self.id}
        nsr = RwNsrYang.YangData_Nsr_NsInstanceOpdata_Nsr.from_dict(nsr_dict)
        nsr.cloud_account = self.cloud_account_name
        nsr.name_ref = self.name
        nsr.nsd_name_ref = self.nsd.name
        nsr.operational_events = self._op_status.msg
        nsr.operational_status = self._op_status.yang_str()
        nsr.config_status = self.map_config_status()
        nsr.create_time = self._create_time
        for vnfr_id in self.vnfrs:
            nsr.constituent_vnfr_ref.append(self.vnfrs[vnfr_id].msg)
        for vlr in self.vlrs:
            nsr.vlr.append(vlr.create_nsr_vlr_msg(self.vnfrs.values()))
        for vnffgr in self.vnffgrs.values():
            nsr.vnffgr.append(vnffgr.fetch_vnffgr())
        return nsr

    def all_vnfs_active(self):
        """ Are all VNFS in this NS active? """
        for _, vnfr in self.vnfrs.items():
            if vnfr.active is not True:
                return False
        return True

    @asyncio.coroutine
    def update_nsr_state(self):
        """ Re-evaluate this  NS's state """
        curr_state = self._op_status.state
        new_state = NetworkServiceRecordState.RUNNING
        self._log.info("Received update_nsr_state for nsr: %s, curr-state: %s",self.id,curr_state)
        #Check all the VNFRs are present
        for _, vnfr in self.vnfrs.items():
            if vnfr.state == VnfRecordState.ACTIVE:
                pass
            elif vnfr.state == VnfRecordState.FAILED:
                event_descr = "Instantiation of VNF %s failed" % vnfr.id
                self.record_event("vnf-failed", event_descr)
                new_state = NetworkServiceRecordState.FAILED
                break
            else:
                new_state = curr_state

        # If new state is RUNNIG; check VNFFGRs are also active
        if new_state == NetworkServiceRecordState.RUNNING:
            for _, vnffgr in self.vnffgrs.items():
                self._log.info("Checking vnffgr state for nsr %s is: %s",self.id,vnffgr.state)
                if vnffgr.state == VnffgRecordState.ACTIVE:
                    pass
                elif vnffgr.state == VnffgRecordState.FAILED:
                    event_descr = "Instantiation of VNFFGR %s failed" % vnffgr.id
                    self.record_event("vnffg-failed", event_descr)
                    new_state = NetworkServiceRecordState.FAILED
                    break
                else:
                    self._log.info("VNFFGR %s in NSR %s is still not active; current state is: %s",
                                    vnffgr.id, self.state, vnffgr.state) 
                    new_state = curr_state

        if new_state != curr_state:
            self._log.debug("Changing state of Network service %s from %s to %s",
                            self.id, curr_state, new_state)
            if new_state == NetworkServiceRecordState.RUNNING:
                yield from self.is_active()
            elif new_state == NetworkServiceRecordState.FAILED:
                yield from self.instantiation_failed()

    @asyncio.coroutine
    def get_monitoring_param(self):
        """ Get monitoring params for this network service """
        vnfrs = list(self.vnfrs.values())
        monp_list = []
        for vnfr in vnfrs:
            self._mon_params[vnfr.id].vnfr_id_ref = vnfr.id
            self._mon_params[vnfr.id].monitoring_param = yield from vnfr.get_monitoring_param()
            monp_list.append(self._mon_params[vnfr.id])

        return monp_list


class InputParameterSubstitution(object):
    """
    This class is responsible for substituting input parameters into an NSD.
    """

    def __init__(self, log):
        """Create an instance of InputParameterSubstitution

        Arguments:
            log - a logger for this object to use

        """
        self.log = log

    def __call__(self, nsd, nsr_config):
        """Substitutes input parameters from the NSR config into the NSD

        This call modifies the provided NSD with the input parameters that are
        contained in the NSR config.

        Arguments:
            nsd        - a GI NSD object
            nsr_config - a GI NSR config object

        """
        if nsd is None or nsr_config is None:
            return

        # Create a lookup of the xpath elements that this descriptor allows
        # to be modified
        optional_input_parameters = set()
        for input_parameter in nsd.input_parameter_xpath:
            optional_input_parameters.add(input_parameter.xpath)

        # Apply the input parameters to the descriptor
        if nsr_config.input_parameter:
            for param in nsr_config.input_parameter:
                if param.xpath not in optional_input_parameters:
                    msg = "tried to set an invalid input parameter ({})"
                    self.log.error(msg.format(param.xpath))

                    continue

                self.log.debug(
                        "input-parameter:{} = {}".format(
                            param.xpath,
                            param.value,
                            )
                        )

                try:
                    xpath.setxattr(nsd, param.xpath, param.value)

                except Exception as e:
                    self.log.exception(e)


class NetworkServiceDescriptor(object):
    """
    Network service descriptor class
    """

    def __init__(self, dts, log, loop, nsd):
        self._dts = dts
        self._log = log
        self._loop = loop

        self._nsd = nsd
        self._ref_count = 0

    @property
    def id(self):
        """ Returns nsd id """
        return self._nsd.id

    @property
    def name(self):
        """ Returns name of nsd """
        return self._nsd.name

    @property
    def ref_count(self):
        """ Returns reference count"""
        return self._ref_count

    def in_use(self):
        """ Returns whether nsd is in use or not """
        return True if self.ref_count > 0 else False

    def ref(self):
        """ Take a reference on this object """
        self._ref_count += 1

    def unref(self):
        """ Release reference on this object """
        if self.ref_count < 1:
            msg = ("Unref on a NSD object - nsd id %s, ref_count = %s" %
                   (self.id, self.ref_count))
            self._log.critical(msg)
            raise NetworkServiceDescriptorError(msg)
        self._ref_count -= 1

    @property
    def msg(self):
        """ Return the message associated with this NetworkServiceDescriptor"""
        return self._nsd

    @staticmethod
    def path_for_id(nsd_id):
        """ Return path for the passed nsd_id"""
        return "C,/nsd:nsd-catalog/nsd:nsd[nsd:id = '{}'".format(nsd_id)

    def path(self):
        """ Return the message associated with this NetworkServiceDescriptor"""
        return NetworkServiceDescriptor.path_for_id(self.id)

    def update(self, nsd):
        """ Update the NSD descriptor """
        if self.in_use():
            self._log.error("Cannot update descriptor %s in use", self.id)
            raise NetworkServiceDescriptorError("Cannot update descriptor in use %s" % self.id)
        self._nsd = nsd


class NsdDtsHandler(object):
    """ The network service descriptor DTS handler """
    XPATH = "C,/nsd:nsd-catalog/nsd:nsd"

    def __init__(self, dts, log, loop, nsm):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._nsm = nsm

        self._regh = None

    @property
    def regh(self):
        """ Return registration handle """
        return self._regh

    @asyncio.coroutine
    def register(self):
        """ Register for Nsd create/update/delete/read requests from dts """

        def on_apply(dts, acg, xact, action, scratch):
            """Apply the  configuration"""
            self._log.debug("Got nsd apply cfg (xact:%s) (action:%s)",
                            xact, action)
            if action == rwdts.AppconfAction.INSTALL and xact.id is None:
                self._log.debug("No xact handle.  Skipping apply config")
                return RwTypes.RwStatus.SUCCESS

            return RwTypes.RwStatus.SUCCESS

        @asyncio.coroutine
        def on_prepare(dts, acg, xact, xact_info, ks_path, msg):
            """ Prepare callback from DTS for NSD config """

            self._log.info("NSD config received nsd id %s, msg %s",
                           msg.id, msg)

            fref = ProtobufC.FieldReference.alloc()
            fref.goto_whole_message(msg.to_pbcm())

            if fref.is_field_deleted():
                # Delete an NSD record
                self._log.debug("Deleting NSD with id %s", msg.id)
                if self._nsm.nsd_in_use(msg.id):
                    self._log.debug("Cannot delete NSD in use - %s", msg.id)
                    err = "Cannot delete an NSD in use - %s" % msg.id
                    raise NetworkServiceDescriptorRefCountExists(err)
                self._nsm.delete_nsd(msg.id)
            else:
                if self._nsm.nsd_in_use(msg.id):
                    self._log.debug("Cannot modify an NSD in use - %s", msg.id)
                    err = "Cannot modify an NSD in use - %s" % msg.id
                    raise NetworkServiceDescriptorRefCountExists(err)
                # Create/Update an NSD record
                self._nsm.update_nsd(msg)

            xact_info.respond_xpath(rwdts.XactRspCode.ACK)

        self._log.debug(
            "Registering for NSD config using xpath: %s",
            NsdDtsHandler.XPATH,
            )

        acg_hdl = rift.tasklets.AppConfGroup.Handler(on_apply=on_apply)
        with self._dts.appconf_group_create(handler=acg_hdl) as acg:
            self._regh = acg.register(
                xpath=NsdDtsHandler.XPATH,
                flags=rwdts.Flag.SUBSCRIBER | rwdts.Flag.DELTA_READY,
                on_prepare=on_prepare)


class VnfdDtsHandler(object):
    """ DTS handler for VNFD config changes """
    XPATH = "C,/vnfd:vnfd-catalog/vnfd:vnfd"

    def __init__(self, dts, log, loop, nsm):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._nsm = nsm
        self._regh = None

    @asyncio.coroutine
    def regh(self):
        """ DTS registration handle """
        return self._regh

    @asyncio.coroutine
    def register(self):
        """ Register for VNFD configuration"""

        def on_apply(dts, acg, xact, action, scratch):
            """Apply the  configuration"""
            self._log.debug("Got VNFD apply (xact: %s) (action: %s)(scr: %s)",
                            xact, action, scratch)

        @asyncio.coroutine
        def on_prepare(dts, acg, xact, xact_info, ks_path, msg):
            """ on prepare callback """
            self._log.debug("Got on prepare for VNFD (path: %s) (action: %s)",
                            ks_path.to_xpath(RwNsmYang.get_schema()), msg)
            # RIFT-10161
            fref = ProtobufC.FieldReference.alloc()
            fref.goto_whole_message(msg.to_pbcm())

            if fref.is_field_deleted():
                self._log.debug("Deleting VNFD with id %s", msg.id)
                # ATTN -- Need to handle deletes
                # yield from self._nsm.delete_vnfd(msg.id)
            else:
                yield from self._nsm.update_vnfd(msg)
            xact_info.respond_xpath(rwdts.XactRspCode.ACK)

        self._log.debug(
            "Registering for VNFD config using xpath: %s",
            VnfdDtsHandler.XPATH,
            )
        acg_hdl = rift.tasklets.AppConfGroup.Handler(on_apply=on_apply)
        with self._dts.appconf_group_create(handler=acg_hdl) as acg:
            self._regh = acg.register(
                xpath=VnfdDtsHandler.XPATH,
                flags=rwdts.Flag.SUBSCRIBER | rwdts.Flag.DELTA_READY,
                on_prepare=on_prepare)


class NsrDtsHandler(object):
    """ The network service DTS handler """
    XPATH = "C,/nsr:ns-instance-config/nsr:nsr"

    def __init__(self, dts, log, loop, nsm):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._nsm = nsm
        self._regh = None

    @property
    def regh(self):
        """ Return registration handle """
        return self._regh

    @property
    def nsm(self):
        """ Return the NS manager instance """
        return self._nsm

    @asyncio.coroutine
    def register(self):
        """ Register for Nsr create/update/delete/read requests from dts """

        def on_init(acg, xact, scratch):
            """ On init callback """

        def on_deinit(acg, xact, scratch):
            """ On deinit callback """
            pass

        def on_apply(dts, acg, xact, action, scratch):
            """Apply the  configuration"""
            self._log.debug("Got nsr apply (xact: %s) (action: %s)(scr: %s)",
                            xact, action, scratch)

            def handle_create_nsr():
                """ Handle create nsr requests """
                # Do some validations
                if not msg.has_field("nsd_ref"):
                    err = "NSD reference not provided"
                    self._log.error(err)
                    raise NetworkServiceRecordError(err)

                self._log.info("Creating NetworkServiceRecord %s  from nsd_id  %s",
                               msg.id, msg.nsd_ref)

                nsr = self.nsm.create_nsr(msg)
                return nsr

            @asyncio.coroutine
            def begin_instantiation(nsr):
                """ Begin instantiation """
                self._log.info("Beginning NS instantiation: %s", nsr.id)
                with self._dts.transaction() as xact:
                    yield from self._nsm.instantiate_ns(nsr.id, xact)

            if action == rwdts.AppconfAction.INSTALL and xact.id is None:
                self._log.debug("No xact handle.  Skipping apply config")
                xact = None

            for msg in self.regh.get_xact_elements(xact):
                fref = ProtobufC.FieldReference.alloc()
                fref.goto_whole_message(msg.to_pbcm())

                if fref.is_field_deleted():
                    self._log.error("Ignoring delete in apply - msg:%s", msg)
                    continue

                if msg.id not in self._nsm.nsrs:
                    nsr = handle_create_nsr()
                    self._loop.create_task(begin_instantiation(nsr))

            return RwTypes.RwStatus.SUCCESS

        @asyncio.coroutine
        def on_prepare(dts, acg, xact, xact_info, ks_path, msg):
            """ Prepare calllback from DTS for NSR """

            xpath = ks_path.to_xpath(RwNsrYang.get_schema())
            self._log.debug(
                "Got Nsr prepare callback (xact:%s info: %s, %s:%s)",
                xact, xact_info, xpath, msg)

            @asyncio.coroutine
            def handle_delete_nsr():
                """ Handle delete NSR requests """
                self._log.info("Delete req for  NSR Id: %s received", msg.id)
                # Terminate the NSR instance
                yield from self._nsm.terminate_ns(msg.id, xact_info.xact)

            fref = ProtobufC.FieldReference.alloc()
            fref.goto_whole_message(msg.to_pbcm())

            if fref.is_field_deleted():
                try:
                    yield from handle_delete_nsr()
                except Exception:
                    self._log.exception("Failed to terminate NS:%s", msg.id)

            else:
                # Ensure the Cloud account has been specified if this is an NSR create
                if msg.id not in self._nsm.nsrs:
                    if not msg.has_field("cloud_account"):
                        raise NsrInstantiationFailed("Cloud account not specified in NSR")

            acg.handle.prepare_complete_ok(xact_info.handle)

        self._log.debug("Registering for NSR config using xpath: %s",
                        NsrDtsHandler.XPATH,)

        acg_hdl = rift.tasklets.AppConfGroup.Handler(
                    on_init=on_init,
                    on_deinit=on_deinit,
                    on_apply=on_apply,
                    )
        with self._dts.appconf_group_create(handler=acg_hdl) as acg:
            self._regh = acg.register(xpath=NsrDtsHandler.XPATH,
                                      flags=rwdts.Flag.SUBSCRIBER | rwdts.Flag.DELTA_READY,
                                      on_prepare=on_prepare)


class VnfrDtsHandler(object):
    """ The virtual network service DTS handler """
    XPATH = "D,/vnfr:vnfr-catalog/vnfr:vnfr"

    def __init__(self, dts, log, loop, nsm):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._nsm = nsm

        self._regh = None

    @property
    def regh(self):
        """ Return registration handle """
        return self._regh

    @property
    def nsm(self):
        """ Return the NS manager instance """
        return self._nsm

    @asyncio.coroutine
    def register(self):
        """ Register for vnfr create/update/delete/ advises from dts """

        def on_commit(xact_info):
            """ The transaction has been committed """
            self._log.debug("Got vnfr commit (xact_info: %s)", xact_info)
            return rwdts.MemberRspCode.ACTION_OK

        @asyncio.coroutine
        def on_prepare(xact_info, action, ks_path, msg):
            """ prepare callback from dts """
            xpath = ks_path.to_xpath(RwNsrYang.get_schema())
            self._log.debug(
                "Got vnfr on_prepare cb (xact_info: %s, action: %s): %s:%s",
                xact_info, action, ks_path, msg
                )

            if action == rwdts.QueryAction.CREATE or action == rwdts.QueryAction.UPDATE:
                yield from self._nsm.update_vnfr(msg)
            elif action == rwdts.QueryAction.DELETE:
                schema = VnfrYang.YangData_Vnfr_VnfrCatalog_Vnfr.schema()
                path_entry = schema.keyspec_to_entry(ks_path)
                self._log.debug("Deleting VNFR with id %s", path_entry.key00.id)
                self._nsm.delete_vnfr(path_entry.key00.id)

            xact_info.respond_xpath(rwdts.XactRspCode.ACK, xpath)

        self._log.debug("Registering for VNFR using xpath: %s",
                        VnfrDtsHandler.XPATH,)

        hdl = rift.tasklets.DTS.RegistrationHandler(on_commit=on_commit,
                                                    on_prepare=on_prepare,)
        with self._dts.group_create() as group:
            self._regh = group.register(xpath=VnfrDtsHandler.XPATH,
                                        handler=hdl,
                                        flags=(rwdts.Flag.SUBSCRIBER),)


class NsMonitorDtsHandler(object):
    """ The Network service Monitor DTS handler """
    XPATH = "D,/nsr:ns-instance-opdata/nsr:nsr/nsr:vnf-monitoring-param"

    def __init__(self, dts, log, loop, nsm):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._nsm = nsm

        self._regh = None

    @property
    def regh(self):
        """ Return registration handle """
        return self._regh

    @property
    def nsm(self):
        """ Return the NS manager instance """
        return self._nsm

    @staticmethod
    def vnf_mon_param_xpath(nsr_id, vnfr_id):
        """ VNF monitoring xpath """
        return ("D,/nsr:ns-instance-opdata" +
                "/nsr:nsr[nsr:ns-instance-config-ref = '{}']" +
                "/nsr:vnf-monitoring-param" +
                "[nsr:vnfr-id-ref = '{}']").format(nsr_id, vnfr_id)

    @asyncio.coroutine
    def register(self):
        """ Register for NS monitoring read from dts """

        @asyncio.coroutine
        def on_prepare(xact_info, action, ks_path, msg):
            """ prepare callback from dts """
            xpath = ks_path.to_xpath(RwNsrYang.get_schema())
            if action == rwdts.QueryAction.READ:
                schema = RwNsrYang.YangData_Nsr_NsInstanceOpdata_Nsr.schema()
                path_entry = schema.keyspec_to_entry(ks_path)
                try:
                    monp_list = yield from self._nsm.get_monitoring_param(
                        path_entry.key00.ns_instance_config_ref)
                    for nsr_id, vnf_monp_list in monp_list:
                        for monp in vnf_monp_list:
                            vnf_xpath = NsMonitorDtsHandler.vnf_mon_param_xpath(
                                            nsr_id,
                                            monp.vnfr_id_ref
                                            )
                            xact_info.respond_xpath(rwdts.XactRspCode.MORE,
                                                    vnf_xpath,
                                                    monp)
                except Exception:
                    self._log.exception("##### Caught exception while collection mon params #####")

                xact_info.respond_xpath(rwdts.XactRspCode.ACK)
            else:
                xact_info.respond_xpath(rwdts.XactRspCode.NA)

        hdl = rift.tasklets.DTS.RegistrationHandler(on_prepare=on_prepare,)
        with self._dts.group_create() as group:
            self._regh = group.register(xpath=NsMonitorDtsHandler.XPATH,
                                        handler=hdl,
                                        flags=rwdts.Flag.PUBLISHER,
                                        )


class NsdRefCountDtsHandler(object):
    """ The NSD Ref Count DTS handler """
    XPATH = "D,/nsr:ns-instance-opdata/rw-nsr:nsd-ref-count"

    def __init__(self, dts, log, loop, nsm):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._nsm = nsm

        self._regh = None

    @property
    def regh(self):
        """ Return registration handle """
        return self._regh

    @property
    def nsm(self):
        """ Return the NS manager instance """
        return self._nsm

    @asyncio.coroutine
    def register(self):
        """ Register for NSD ref count read from dts """

        @asyncio.coroutine
        def on_prepare(xact_info, action, ks_path, msg):
            """ prepare callback from dts """
            xpath = ks_path.to_xpath(RwNsrYang.get_schema())

            if action == rwdts.QueryAction.READ:
                schema = RwNsrYang.YangData_Nsr_NsInstanceOpdata_NsdRefCount.schema()
                path_entry = schema.keyspec_to_entry(ks_path)
                nsd_list = yield from self._nsm.get_nsd_refcount(path_entry.key00.nsd_id_ref)
                for xpath, msg in nsd_list:
                    xact_info.respond_xpath(rsp_code=rwdts.XactRspCode.MORE,
                                            xpath=xpath,
                                            msg=msg)
                xact_info.respond_xpath(rwdts.XactRspCode.ACK)
            else:
                raise NetworkServiceRecordError("Not supported operation %s" % action)

        hdl = rift.tasklets.DTS.RegistrationHandler(on_prepare=on_prepare,)
        with self._dts.group_create() as group:
            self._regh = group.register(xpath=NsdRefCountDtsHandler.XPATH,
                                        handler=hdl,
                                        flags=rwdts.Flag.PUBLISHER,)


class NsManagerRPCHandler(object):
    """ The Network service Monitor DTS handler """
    EXEC_NS_CONF_XPATH = "I,/nsr:exec-ns-config-primitive"
    EXEC_NS_CONF_O_XPATH = "O,/nsr:exec-ns-config-primitive"

    GET_NS_CONF_XPATH = "I,/nsr:get-ns-config-primitive-values"
    GET_NS_CONF_O_XPATH = "O,/nsr:get-ns-config-primitive-values"

    def __init__(self, dts, log, loop, nsm):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._nsm = nsm

        self._ns_regh = None
        self._vnf_regh = None
        self._get_ns_conf_regh = None

        self.job_manager = rift.mano.config_agent.ConfigAgentJobManager(dts, log, loop, nsm)

    @property
    def reghs(self):
        """ Return registration handles """
        return (self._ns_regh, self._vnf_regh, self._get_ns_conf_regh)

    @property
    def nsm(self):
        """ Return the NS manager instance """
        return self._nsm

    def prepare_meta(self, rpc_ip):

        try:
            nsr_id = rpc_ip.nsr_id_ref
            nsr = self._nsm.nsrs[nsr_id]
            vnfrs = {}
            for vnf in rpc_ip.vnf_list:
                vnfr_id = vnf.vnfr_id_ref
                vnfrs[vnfr_id] = self._nsm.vnfrs[vnfr_id]

            return nsr, vnfrs
        except KeyError as e:
            raise ValueError("Record not found", str(e))

    def _get_ns_cfg_primitive(self, nsr_id, ns_cfg_name):
        try:
            nsr = self._nsm.nsrs[nsr_id]
        except KeyError:
            raise ValueError("NSR id %s not found" % nsr_id)

        nsd_msg = self._nsm.get_nsd(nsr.nsd_id).msg

        def get_nsd_cfg_prim(name):
            for ns_cfg_prim in nsd_msg.config_primitive:
                if ns_cfg_prim.name == name:
                    return ns_cfg_prim

            raise ValueError("Could not find ns_cfg_prim %s in nsr id %s" % (name, nsr_id))

        ns_cfg_prim_msg = get_nsd_cfg_prim(ns_cfg_name)
        ret_cfg_prim_msg = ns_cfg_prim_msg.deep_copy()

        return ret_cfg_prim_msg

    def _get_vnf_primitive(self, nsr_id, vnf_index, primitive_name):
        try:
            nsr = self._nsm.nsrs[nsr_id]
        except KeyError:
            raise ValueError("NSR id %s not found" % nsr_id)

        nsd_msg = self._nsm.get_nsd(nsr.nsd_id).msg
        for vnf in nsd_msg.constituent_vnfd:
            if vnf.member_vnf_index != vnf_index:
                continue

            for primitive in vnf.vnf_configuration.config_primitive:
                if primitive.name == primitive_name:
                    return primitive

        raise ValueError("Could not find vnf index %s primitive %s in nsr id %s" %
                         (vnf_index, primitive_name, nsr_id))

    @asyncio.coroutine
    def register(self):
        """ Register for NS monitoring read from dts """
        yield from self.job_manager.register()

        @asyncio.coroutine
        def on_ns_config_prepare(xact_info, action, ks_path, msg):
            """ prepare callback from dts exec-ns-config-primitive"""
            assert action == rwdts.QueryAction.RPC
            rpc_ip = msg
            rpc_op = NsrYang.YangOutput_Nsr_ExecNsConfigPrimitive()

            ns_cfg_prim_name = rpc_ip.name
            nsr_id = rpc_ip.nsr_id_ref
            nsr = self._nsm.nsrs[nsr_id]

            nsd_cfg_prim_msg = self._get_ns_cfg_primitive(nsr_id, ns_cfg_prim_name)

            def find_nsd_vnf_prim_param_pool(vnf_index, vnf_prim_name, param_name):
                for vnf_prim_group in nsd_cfg_prim_msg.vnf_primitive_group:
                    if vnf_prim_group.member_vnf_index_ref != vnf_index:
                        continue

                    for vnf_prim in vnf_prim_group.primitive:
                        if vnf_prim.name != vnf_prim_name:
                            continue

                        for pool_param in vnf_prim.pool_parameters:
                            if pool_param.name != param_name:
                                continue

                            try:
                                nsr_param_pool = nsr.param_pools[pool_param.parameter_pool]
                            except KeyError:
                                raise ValueError("Parameter pool %s does not exist in nsr" % vnf_prim.parameter_pool)

                            self._log.debug("Found parameter pool %s for vnf index(%s), vnf_prim_name(%s), param_name(%s)",
                                            nsr_param_pool, vnf_index, vnf_prim_name, param_name)
                            return nsr_param_pool

                self._log.debug("Could not find parameter pool for vnf index(%s), vnf_prim_name(%s), param_name(%s)",
                                vnf_index, vnf_prim_name, param_name)
                return None

            rpc_op.nsr_id_ref = nsr_id
            rpc_op.name = ns_cfg_prim_name

            nsr, vnfrs = self.prepare_meta(rpc_ip)
            rpc_op.job_id = nsr.job_id

            # Give preference to user defined script.
            if nsd_cfg_prim_msg.has_field("user_defined_script"):
                rpc_ip.user_defined_script = nsd_cfg_prim_msg.user_defined_script


                tasks = []
                for config_plugin in self.nsm.config_agent_plugins:
                    task = yield from config_plugin.apply_config(
                            rpc_ip,
                            nsr,
                            vnfrs)
                    tasks.append(task)

                self.job_manager.add_job(rpc_op, tasks)
            else:
                for vnf in rpc_ip.vnf_list:
                    vnf_op = rpc_op.vnf_out_list.add()
                    vnf_member_idx = vnf.member_vnf_index_ref
                    vnfr_id = vnf.vnfr_id_ref
                    vnf_op.vnfr_id_ref = vnfr_id
                    vnf_op.member_vnf_index_ref = vnf_member_idx
                    for primitive in vnf.vnf_primitive:
                        op_primitive = vnf_op.vnf_out_primitive.add()
                        op_primitive.name = primitive.name
                        op_primitive.execution_id = ''
                        op_primitive.execution_status = 'completed'
                        self._log.debug("%s:%s Got primitive %s:%s",
                                        nsr_id, vnf.member_vnf_index_ref, primitive.name, primitive.parameter)

                        nsd_vnf_primitive = self._get_vnf_primitive(
                                nsr_id,
                                vnf_member_idx,
                                primitive.name
                                )
                        for param in nsd_vnf_primitive.parameter:
                            if not param.has_field("parameter_pool"):
                                continue

                            try:
                                nsr_param_pool = nsr.param_pools[param.parameter_pool]
                            except KeyError:
                                raise ValueError("Parameter pool %s does not exist in nsr" % param.parameter_pool)
                            nsr_param_pool.add_used_value(param.value)

                        for config_plugin in self.nsm.config_agent_plugins:
                            yield from config_plugin.vnf_config_primitive(nsr_id,
                                                                          vnfr_id,
                                                                          primitive,
                                                                          op_primitive)

                self.job_manager.add_job(rpc_op)

            # Get NSD
            # Find Config Primitive
            # For each vnf-primitive with parameter pool
            # Find parameter pool
            # Add used value to the pool
            self._log.debug("RPC output: {}".format(rpc_op))
            xact_info.respond_xpath(rwdts.XactRspCode.ACK,
                                    NsManagerRPCHandler.EXEC_NS_CONF_O_XPATH,
                                    rpc_op)

        @asyncio.coroutine
        def on_get_ns_config_values_prepare(xact_info, action, ks_path, msg):
            assert action == rwdts.QueryAction.RPC
            nsr_id = msg.nsr_id_ref
            nsr = self._nsm.nsrs[nsr_id]
            cfg_prim_name = msg.name

            rpc_op = NsrYang.YangOutput_Nsr_GetNsConfigPrimitiveValues()

            ns_cfg_prim_msg = self._get_ns_cfg_primitive(nsr_id, cfg_prim_name)

            # Get pool values for NS-level parameters
            for ns_param in ns_cfg_prim_msg.parameter:
                if not ns_param.has_field("parameter_pool"):
                    continue

                try:
                    nsr_param_pool = nsr.param_pools[ns_param.parameter_pool]
                except KeyError:
                    raise ValueError("Parameter pool %s does not exist in nsr" % ns_param.parameter_pool)

                new_ns_param = rpc_op.ns_parameter.add()
                new_ns_param.name = ns_param.name
                new_ns_param.value = str(nsr_param_pool.get_next_unused_value())


            # Get pool values for NS-level parameters
            for vnf_prim_group in ns_cfg_prim_msg.vnf_primitive_group:
                rsp_prim_group = rpc_op.vnf_primitive_group.add()
                rsp_prim_group.member_vnf_index_ref = vnf_prim_group.member_vnf_index_ref
                if vnf_prim_group.has_field("vnfd_id_ref"):
                    rsp_prim_group.vnfd_id_ref = vnf_prim_group.vnfd_id_ref

                for index, vnf_prim in enumerate(vnf_prim_group.primitive):
                    rsp_prim = rsp_prim_group.primitive.add()
                    rsp_prim.name = vnf_prim.name
                    rsp_prim.index = index
                    vnf_primitive = self._get_vnf_primitive(
                            nsr_id,
                            vnf_prim_group.member_vnf_index_ref,
                            vnf_prim.name
                            )
                    for param in vnf_primitive.parameter:
                        if not param.has_field("parameter_pool"):
                            continue

                        try:
                            nsr_param_pool = nsr.param_pools[param.parameter_pool]
                        except KeyError:
                            raise ValueError("Parameter pool %s does not exist in nsr" % vnf_prim.parameter_pool)

                        vnf_param = rsp_prim.parameter.add()
                        vnf_param.name = param.name
                        vnf_param.value = str(nsr_param_pool.get_next_unused_value())

            self._log.debug("RPC output: {}".format(rpc_op))
            xact_info.respond_xpath(rwdts.XactRspCode.ACK,
                                    NsManagerRPCHandler.GET_NS_CONF_O_XPATH, rpc_op)

        hdl_ns = rift.tasklets.DTS.RegistrationHandler(on_prepare=on_ns_config_prepare,)
        hdl_ns_get = rift.tasklets.DTS.RegistrationHandler(on_prepare=on_get_ns_config_values_prepare,)

        with self._dts.group_create() as group:
            self._ns_regh = group.register(xpath=NsManagerRPCHandler.EXEC_NS_CONF_XPATH,
                                           handler=hdl_ns,
                                           flags=rwdts.Flag.PUBLISHER,
                                           )
            self._get_ns_conf_regh = group.register(xpath=NsManagerRPCHandler.GET_NS_CONF_XPATH,
                                           handler=hdl_ns_get,
                                           flags=rwdts.Flag.PUBLISHER,
                                           )


class NsManager(object):
    """ The Network Service Manager class"""
    def __init__(self, dts, log, loop,
                 nsr_handler, vnfr_handler, vlr_handler, cloud_plugin_selector,vnffgmgr):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._nsr_handler = nsr_handler
        self._vnfr_pub_handler = vnfr_handler
        self._vlr_pub_handler = vlr_handler
        self._vnffgmgr = vnffgmgr

        self._cloud_plugin_selector = cloud_plugin_selector

        self._nsrs = {}
        self._nsds = {}
        self._vnfds = {}
        self._vnfrs = {}

        self._so_obj = conman.ROServiceOrchConfig(log, loop, dts)

        self._dts_handlers = [NsdDtsHandler(dts, log, loop, self),
                              VnfrDtsHandler(dts, log, loop, self),
                              NsMonitorDtsHandler(dts, log, loop, self),
                              NsdRefCountDtsHandler(dts, log, loop, self),
                              NsrDtsHandler(dts, log, loop, self),
                              VnfdDtsHandler(dts, log, loop, self),
                              NsManagerRPCHandler(dts, log, loop, self),
                              self._so_obj]

        self._config_agent_plugins = []

    @property
    def log(self):
        """ Log handle """
        return self._log

    @property
    def loop(self):
        """ Loop """
        return self._loop

    @property
    def dts(self):
        """ DTS handle """
        return self._dts

    @property
    def nsr_handler(self):
        """" NSR handler """
        return self._nsr_handler

    @property
    def so_obj(self):
        """" So Obj handler """
        return self._so_obj

    @property
    def nsrs(self):
        """ NSRs in this NSM"""
        return self._nsrs

    @property
    def nsds(self):
        """ NSDs in this NSM"""
        return self._nsds

    @property
    def vnfds(self):
        """ VNFDs in this NSM"""
        return self._vnfds

    @property
    def vnfrs(self):
        """ VNFRs in this NSM"""
        return self._vnfrs

    @property
    def nsr_pub_handler(self):
        """ NSR publication handler """
        return self._nsr_handler

    @property
    def vnfr_pub_handler(self):
        """ VNFR publication handler """
        return self._vnfr_pub_handler

    @property
    def vlr_pub_handler(self):
        """ VLR publication handler """
        return self._vlr_pub_handler

    @property
    def config_agent_plugins(self):
        """ Config agent plugins"""
        return self._config_agent_plugins

    def set_config_agent_plugin(self, plugin_instance):
        """ Sets the plugin to use for the NSM config agents"""
        self._log.debug("Set NSM config agent plugin instance: %s", plugin_instance)
        if plugin_instance not in self._config_agent_plugins:
            self._config_agent_plugins.append(plugin_instance)

    @asyncio.coroutine
    def register(self):
        """ Register all static DTS handlers """
        for dts_handle in self._dts_handlers:
            yield from dts_handle.register()

    def get_ns_by_nsr_id(self, nsr_id):
        """ get NSR by nsr id """
        if nsr_id not in self._nsrs:
            raise NetworkServiceRecordError("NSR id %s not found" % nsr_id)

        return self._nsrs[nsr_id]

    def create_nsr(self, nsr_msg):
        """ Create an NSR instance """
        if nsr_msg.id in self._nsrs:
            msg = "NSR id %s already exists" % nsr_msg.id
            self._log.error(msg)
            raise NetworkServiceRecordError(msg)

        self._log.info("Create NetworkServiceRecord nsr id %s from nsd_id %s",
                       nsr_msg.id,
                       nsr_msg.nsd_ref)

        nsm_plugin = self._cloud_plugin_selector.get_cloud_account_plugin_instance(
                nsr_msg.cloud_account
            )
        sdn_account_name = self._cloud_plugin_selector.get_cloud_account_sdn_name(nsr_msg.cloud_account)

        nsr = NetworkServiceRecord(self._dts,
                                   self._log,
                                   self._loop,
                                   self,
                                   nsm_plugin,
                                   self._config_agent_plugins,
                                   nsr_msg,
                                   sdn_account_name
                                   )
        self._nsrs[nsr_msg.id] = nsr
        nsm_plugin.create_nsr(nsr_msg, self.get_nsd(nsr_msg.nsd_ref).msg)

        return nsr

    def delete_nsr(self, nsr_id):
        """
        Delete NSR with the passed nsr id
        """
        del self._nsrs[nsr_id]

    @asyncio.coroutine
    def instantiate_ns(self, nsr_id, xact):
        """ Instantiate an NS instance """
        self._log.debug("Instatiating Network service id %s", nsr_id)
        if nsr_id not in self._nsrs:
            err = "NSR id %s not found " % nsr_id
            self._log.error(err)
            raise NetworkServiceRecordError(err)

        nsr = self._nsrs[nsr_id]
        yield from nsr.nsm_plugin.instantiate_ns(nsr, xact)

    @asyncio.coroutine
    def update_vnfr(self, vnfr):
        """Create/Update an VNFR """
        vnfr_state = self._vnfrs[vnfr.id].state
        self._log.debug("Updating VNFR with state %s: vnfr %s", vnfr_state, vnfr)
        yield from self._vnfrs[vnfr.id].update(vnfr)
        nsr = self.find_nsr_for_vnfr(vnfr.id)
        yield from nsr.update_nsr_state()
        return self._vnfrs[vnfr.id]

    def find_nsr_for_vnfr(self, vnfr_id):
        """ Find the NSR which )has the passed vnfr id"""
        for nsr in list(self.nsrs.values()):
            for vnfr in list(nsr.vnfrs.values()):
                if vnfr.id == vnfr_id:
                    return nsr
        return None

    def delete_vnfr(self, vnfr_id):
        """ Delete VNFR  with the passed id"""
        del self._vnfrs[vnfr_id]

    def get_nsd_ref(self, nsd_id):
        """ Get network service descriptor for the passed nsd_id
            with a reference"""
        nsd = self.get_nsd(nsd_id)
        nsd.ref()
        return nsd

    @asyncio.coroutine
    def get_nsr_config(self, nsd_id):
        xpath = "C,/nsr:ns-instance-config"
        results = yield from self._dts.query_read(xpath, rwdts.Flag.MERGE)

        for result in results:
            entry = yield from result
            ns_instance_config = entry.result

            for nsr in ns_instance_config.nsr:
                if nsr.nsd_ref == nsd_id:
                    return nsr

        return None

    @asyncio.coroutine
    def nsd_unref_by_nsr_id(self, nsr_id):
        """ Unref the network service descriptor based on NSR id """
        self._log.debug("NSR Unref called for Nsr Id:%s", nsr_id)
        if nsr_id in self._nsrs:
            nsr = self._nsrs[nsr_id]
            nsd = self.get_nsd(nsr.nsd_id)
            self._log.debug("Releasing ref on NSD %s held by NSR %s - Curr %d",
                            nsd.id, nsr.id, nsd.ref_count)
            nsd.unref()
        else:
            self._log.error("Cannot find NSD for NSR id %s", nsr_id)
            raise NetworkServiceDescriptorUnrefError("No Nsd for nsr id" % nsr_id)

    @asyncio.coroutine
    def nsd_unref(self, nsd_id):
        """ Unref the network service descriptor associated with the id """
        nsd = self.get_nsd(nsd_id)
        nsd.unref()

    def get_nsd(self, nsd_id):
        """ Get network service descriptor for the passed nsd_id"""
        if nsd_id not in self._nsds:
            self._log.error("Cannot find NSD id:%s", nsd_id)
            raise NetworkServiceDescriptorError("Cannot find NSD id:%s", nsd_id)

        return self._nsds[nsd_id]

    def create_nsd(self, nsd_msg):
        """ Create a network service descriptor """
        self._log.debug("Create network service descriptor - %s", nsd_msg)
        if nsd_msg.id in self._nsds:
            self._log.error("Cannot create NSD %s -NSD ID already exists", nsd_msg)
            raise NetworkServiceDescriptorError("NSD already exists-%s", nsd_msg.id)

        nsd = NetworkServiceDescriptor(
                self._dts,
                self._log,
                self._loop,
                nsd_msg,
                )
        self._nsds[nsd_msg.id] = nsd

        return nsd

    def update_nsd(self, nsd):
        """ update the Network service descriptor """
        self._log.debug("Update network service descriptor - %s", nsd)
        if nsd.id not in self._nsds:
            self._log.debug("No NSD found - creating NSD id = %s", nsd.id)
            self.create_nsd(nsd)
        else:
            self._log.debug("Updating NSD id = %s, nsd = %s", nsd.id, nsd)
            self._nsds[nsd.id].update(nsd)

    def delete_nsd(self, nsd_id):
        """ Delete the Network service descriptor with the passed id """
        self._log.debug("Deleting the network service descriptor - %s", nsd_id)
        if nsd_id not in self._nsds:
            self._log.debug("Delete NSD failed - cannot find nsd-id %s", nsd_id)
            raise NetworkServiceDescriptorNotFound("Cannot find %s", nsd_id)

        if nsd_id not in self._nsds:
            self._log.debug("Cannot delete NSD id %s reference exists %s",
                            nsd_id,
                            self._nsds[nsd_id].ref_count)
            raise NetworkServiceDescriptorRefCountExists(
                "Cannot delete :%s, ref_count:%s",
                nsd_id,
                self._nsds[nsd_id].ref_count)

        del self._nsds[nsd_id]

    @asyncio.coroutine
    def get_vnfd(self, vnfd_id):
        """ Get virtual network function descriptor for the passed vnfd_id"""
        if vnfd_id not in self._vnfds:
            self._log.error("Cannot find VNFD id:%s", vnfd_id)
            raise VnfDescriptorError("Cannot find VNFD id:%s", vnfd_id)

        return self._vnfds[vnfd_id]

    @asyncio.coroutine
    def create_vnfd(self, vnfd):
        """ Create a virtual network function descriptor """
        self._log.debug("Create virtual network function descriptor - %s", vnfd)
        if vnfd.id in self._vnfds:
            self._log.error("Cannot create VNFD %s -VNFD ID already exists", vnfd)
            raise VnfDescriptorError("VNFD already exists-%s", vnfd.id)

        self._vnfds[vnfd.id] = vnfd
        return self._vnfds[vnfd.id]

    @asyncio.coroutine
    def update_vnfd(self, vnfd):
        """ Update the virtual network function descriptor """
        self._log.debug("Update virtual network function descriptor- %s", vnfd)
        if vnfd.id not in self._vnfds:
            self._log.debug("No VNFD found - creating VNFD id = %s", vnfd.id)
            yield from self.create_vnfd(vnfd)
        else:
            self._log.debug("Updating VNFD id = %s, vnfd = %s", vnfd.id, vnfd)
            self._vnfds[vnfd.id] = vnfd

    @asyncio.coroutine
    def delete_vnfd(self, vnfd_id):
        """ Delete the virtual network function descriptor with the passed id """
        self._log.debug("Deleting the virtual network function descriptor - %s", vnfd_id)
        if vnfd_id not in self._vnfds:
            self._log.debug("Delete VNFD failed - cannot find vnfd-id %s", vnfd_id)
            raise VnfDescriptorError("Cannot find %s", vnfd_id)

        del self._vnfds[vnfd_id]

    def nsd_in_use(self, nsd_id):
        """ Is the NSD with the passed id in use """
        self._log.debug("Is this NSD in use - msg:%s", nsd_id)
        if nsd_id in self._nsds:
            return self._nsds[nsd_id].in_use()
        return False

    @asyncio.coroutine
    def publish_nsr(self, xact, path, msg):
        """ Publish a NSR """
        self._log.debug("Publish NSR with path %s, msg %s",
                        path, msg)
        yield from self.nsr_handler.update(xact, path, msg)

    @asyncio.coroutine
    def unpublish_nsr(self, xact, path):
        """ Un Publish an NSR """
        self._log.debug("Publishing delete NSR with path %s", path)
        yield from self.nsr_handler.delete(path, xact)

    def vnfr_is_ready(self, vnfr_id):
        """ VNFR with the id is ready """
        self._log.debug("VNFR id %s ready", vnfr_id)
        if vnfr_id not in self._vnfds:
            err = "Did not find VNFR ID with id %s" % vnfr_id
            self._log.critical("err")
            raise VirtualNetworkFunctionRecordError(err)
        self._vnfrs[vnfr_id].is_ready()

    @asyncio.coroutine
    def get_monitoring_param(self, nsr_id):
        """ Get the monitoring params based on the passed ks_path """
        monp_list = []
        if nsr_id is None or nsr_id == "":
            nsrs = list(self._nsrs.values())
            for nsr in nsrs:
                if nsr.active:
                    monp = yield from nsr.get_monitoring_param()
                    monp_list.append((nsr.id, monp))
        elif nsr_id in self._nsrs:
            if self._nsrs[nsr_id].active:
                monp = yield from self._nsrs[nsr_id].get_monitoring_param()
                monp_list.append((nsr_id, monp))

        return monp_list

    @asyncio.coroutine
    def get_nsd_refcount(self, nsd_id):
        """ Get the nsd_list from this NSM"""

        def nsd_refcount_xpath(nsd_id):
            """ xpath for ref count entry """
            return (NsdRefCountDtsHandler.XPATH +
                    "[rw-nsr:nsd-id-ref = '{}']").format(nsd_id)

        nsd_list = []
        if nsd_id is None or nsd_id == "":
            for nsd in self._nsds.values():
                nsd_msg = RwNsrYang.YangData_Nsr_NsInstanceOpdata_NsdRefCount()
                nsd_msg.nsd_id_ref = nsd.id
                nsd_msg.instance_ref_count = nsd.ref_count
                nsd_list.append((nsd_refcount_xpath(nsd.id), nsd_msg))
        elif nsd_id in self._nsds:
            nsd_msg = RwNsrYang.YangData_Nsr_NsInstanceOpdata_NsdRefCount()
            nsd_msg.nsd_id_ref = self._nsds[nsd_id].id
            nsd_msg.instance_ref_count = self._nsds[nsd_id].ref_count
            nsd_list.append((nsd_refcount_xpath(nsd_id), nsd_msg))

        return nsd_list

    @asyncio.coroutine
    def terminate_ns(self, nsr_id, xact):
        """
        Terminate network service for the given NSR Id
        """

        # Terminate the instances/networks assocaited with this nw service
        self._log.debug("Terminating the network service %s", nsr_id)
        yield from self._nsrs[nsr_id].terminate(xact)

        # Unref the NSD
        yield from self.nsd_unref_by_nsr_id(nsr_id)

        # Unpublish the NSR record
        self._log.debug("Unpublishing the network service %s", nsr_id)
        yield from self._nsrs[nsr_id].unpublish(xact)

        # Finaly delete the NS instance from this NS Manager
        self._log.debug("Deletng the network service %s", nsr_id)
        self.delete_nsr(nsr_id)


class NsmRecordsPublisherProxy(object):
    """ This class provides a publisher interface that allows plugin objects
        to publish NSR/VNFR/VLR"""

    def __init__(self, dts, log, loop, nsr_pub_hdlr, vnfr_pub_hdlr, vlr_pub_hdlr):
        self._dts = dts
        self._log = log
        self._loop = loop
        self._nsr_pub_hdlr = nsr_pub_hdlr
        self._vlr_pub_hdlr = vlr_pub_hdlr
        self._vnfr_pub_hdlr = vnfr_pub_hdlr

    @asyncio.coroutine
    def publish_nsr(self, xact, nsr):
        """ Publish an NSR """
        path = NetworkServiceRecord.xpath_from_nsr(nsr)
        return (yield from self._nsr_pub_hdlr.update(xact, path, nsr))

    @asyncio.coroutine
    def unpublish_nsr(self, xact, nsr):
        """ Unpublish an NSR """
        path = NetworkServiceRecord.xpath_from_nsr(nsr)
        return (yield from self._nsr_pub_hdlr.delete(xact, path))

    @asyncio.coroutine
    def publish_vnfr(self, xact, vnfr):
        """ Publish an VNFR """
        path = VirtualNetworkFunctionRecord.vnfr_xpath(vnfr)
        return (yield from self._vnfr_pub_hdlr.update(xact, path, vnfr))

    @asyncio.coroutine
    def unpublish_vnfr(self, xact, vnfr):
        """ Unpublish a VNFR """
        path = VirtualNetworkFunctionRecord.vnfr_xpath(vnfr)
        return (yield from self._vnfr_pub_hdlr.delete(xact, path))

    @asyncio.coroutine
    def publish_vlr(self, xact, vlr):
        """ Publish a VLR """
        path = VirtualLinkRecord.vlr_xpath(vlr)
        return (yield from self._vlr_pub_hdlr.update(xact, path, vlr))

    @asyncio.coroutine
    def unpublish_vlr(self, xact, vlr):
        """ Unpublish a VLR """
        path = VirtualLinkRecord.vlr_xpath(vlr)
        return (yield from self._vlr_pub_hdlr.delete(xact, path))


class ConfigAccountHandler(object):
    def __init__(self, dts, log, log_hdl, loop):
        self._log = log
        self._log_hdl = log_hdl
        self._dts = dts
        self._loop = loop

        self._log.debug("creating config account handler")
        self.config_agent_handler = rift.mano.config_agent.ConfigAgentSubscriber(
            self._dts, self._log, self._log_hdl,
            rift.mano.config_agent.ConfigAgentCallbacks(
                on_add_apply=self.on_config_account_added,
                on_delete_apply=self.on_config_account_deleted,
            )
        )

    def on_config_account_deleted(self, account_name):
        self._log.debug("config account deleted")
        self._log.debug(account_name)

    def on_config_account_added(self, account):
        self._log.debug("config account added")
        self._log.debug(account.as_dict())

    @asyncio.coroutine
    def register(self):
        self.config_agent_handler.register()


class NsmTasklet(rift.tasklets.Tasklet):
    """
    The network service manager  tasklet
    """
    def __init__(self, *args, **kwargs):
        super(NsmTasklet, self).__init__(*args, **kwargs)

        self._dts = None
        self._nsm = None

        self._cloud_plugin_selector = None
        self._config_agent_mgr = None
        self._vnffgmgr = None

        self._nsr_handler = None
        self._vnfr_pub_handler = None
        self._vlr_pub_handler = None

        self._records_publisher_proxy = None

    def start(self):
        """ The task start callback """
        super(NsmTasklet, self).start()
        self.log.info("Starting NsmTasklet")

        self.log.setLevel(logging.DEBUG)

        self.log.debug("Registering with dts")
        self._dts = rift.tasklets.DTS(self.tasklet_info,
                                      RwNsmYang.get_schema(),
                                      self.loop,
                                      self.on_dts_state_change)

        self.log.debug("Created DTS Api GI Object: %s", self._dts)

    def on_instance_started(self):
        """ Task instance started callback """
        self.log.debug("Got instance started callback")

    @asyncio.coroutine
    def init(self):
        """ Task init callback """
        self.log.debug("Got instance started callback")

        self.log.debug("creating config account handler")

        self._nsr_pub_handler = publisher.NsrOpDataDtsHandler(self._dts, self.log, self.loop)
        yield from self._nsr_pub_handler.register()

        self._vnfr_pub_handler = publisher.VnfrPublisherDtsHandler(self._dts, self.log, self.loop)
        yield from self._vnfr_pub_handler.register()

        self._vlr_pub_handler = publisher.VlrPublisherDtsHandler(self._dts, self.log, self.loop)
        yield from self._vlr_pub_handler.register()

        self._records_publisher_proxy = NsmRecordsPublisherProxy(
                self._dts,
                self.log,
                self.loop,
                self._nsr_pub_handler,
                self._vnfr_pub_handler,
                self._vlr_pub_handler,
                )

        # Register the NSM to receive the nsm plugin
        # when cloud account is configured
        self._cloud_plugin_selector = cloud.CloudAccountNsmPluginSelector(
                self._dts,
                self.log,
                self.log_hdl,
                self.loop,
                self._records_publisher_proxy,
                )
        yield from self._cloud_plugin_selector.register()

        self._vnffgmgr = rwvnffgmgr.VnffgMgr(self._dts,self.log,self.log_hdl,self.loop)
        yield from self._vnffgmgr.register()

        self._nsm = NsManager(
                self._dts,
                self.log,
                self.loop,
                self._nsr_pub_handler,
                self._vnfr_pub_handler,
                self._vlr_pub_handler,
                self._cloud_plugin_selector,
                self._vnffgmgr,
                )

        # Register the NSM to receive the nsm config agent plugin
        # when config agent is configured
        self._config_agent_mgr = conagent.NsmConfigAgent(
                self._dts,
                self.log,
                self.loop,
                self._records_publisher_proxy,
                self._nsm.set_config_agent_plugin,
                )
        yield from self._config_agent_mgr.register()
        # RIFT-11780 : Must call NSM register after initializing config plugin
        # During restart, there is race condition which causes the NS creation
        # to occur before even config_plugin is registered.
        yield from self._nsm.register()


    @asyncio.coroutine
    def run(self):
        """ Task run callback """
        pass

    @asyncio.coroutine
    def on_dts_state_change(self, state):
        """Take action according to current dts state to transition
        application into the corresponding application state

        Arguments
            state - current dts state
        """
        switch = {
            rwdts.State.INIT: rwdts.State.REGN_COMPLETE,
            rwdts.State.CONFIG: rwdts.State.RUN,
        }

        handlers = {
            rwdts.State.INIT: self.init,
            rwdts.State.RUN: self.run,
        }

        # Transition application to next state
        handler = handlers.get(state, None)
        if handler is not None:
            yield from handler()

        # Transition dts to next state
        next_state = switch.get(state, None)
        if next_state is not None:
            self.log.debug("Changing state to %s", next_state)
            self._dts.handle.set_state(next_state)
