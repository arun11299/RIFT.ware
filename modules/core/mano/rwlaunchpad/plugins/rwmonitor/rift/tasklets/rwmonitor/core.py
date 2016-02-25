
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import asyncio
import collections
import time

from gi.repository import (
        RwVnfrYang,
        RwNsrYang,
        RwDts,
        )

import rift.tasklets


class NfviMetricsAggregator(object):
    def __init__(self,
            tasklet,
            cloud_account=None,
            nfvi_monitor=None,
            ):
        """Create an instance of NfviMetricsAggregator

        Arguments:
            tasklet       - a tasklet object that provides access to DTS,
                            logging, the asyncio ioloop, and monitoring state
            cloud_account - a cloud account
            nfvi_monitor  - an NFVI monitor plugin

        """
        self.tasklet = tasklet
        self.nfvi_monitor = nfvi_monitor
        self.cloud_account = cloud_account

    @property
    def dts(self):
        return self.tasklet.dts

    @property
    def log(self):
        return self.tasklet.log

    @property
    def loop(self):
        return self.tasklet.loop

    @property
    def records(self):
        return self.tasklet.records

    @property
    def polling_period(self):
        return self.tasklet.polling_period

    @asyncio.coroutine
    def request_vdu_metrics(self, vdur):
        try:
            # self.log.debug('request_vdu_metrics: {}'.format(vdur.vim_id))

            # Create uninitialized metric structure
            vdu_metrics = RwVnfrYang.YangData_Vnfr_VnfrCatalog_Vnfr_Vdur_NfviMetrics()

            # No metrics can be collected if the monitor has not been
            # specified.
            if self.nfvi_monitor is None:
                return vdu_metrics

            # Retrieve the NFVI metrics for this VDU
            try:
                _, metrics = yield from self.loop.run_in_executor(
                        self.tasklet.executor,
                        self.nfvi_monitor.nfvi_metrics,
                        self.cloud_account,
                        vdur.vim_id,
                        )

            except Exception as e:
                self.log.exception(e)
                return vdu_metrics

            # VCPU
            vdu_metrics.vcpu.total = vdur.vm_flavor.vcpu_count
            vdu_metrics.vcpu.utilization = metrics.vcpu.utilization

            # Memory (in bytes)
            vdu_metrics.memory.used = metrics.memory.used
            vdu_metrics.memory.total = 1e6 * vdur.vm_flavor.memory_mb
            vdu_metrics.memory.utilization = 100 * vdu_metrics.memory.used / vdu_metrics.memory.total

            # Storage
            vdu_metrics.storage.used = metrics.storage.used
            vdu_metrics.storage.total = 1e9 * vdur.vm_flavor.storage_gb
            vdu_metrics.storage.utilization = 100 * vdu_metrics.storage.used / vdu_metrics.storage.total

            # Network (incoming)
            vdu_metrics.network.incoming.packets = metrics.network.incoming.packets
            vdu_metrics.network.incoming.packet_rate = metrics.network.incoming.packet_rate
            vdu_metrics.network.incoming.bytes = metrics.network.incoming.bytes
            vdu_metrics.network.incoming.byte_rate = metrics.network.incoming.byte_rate

            # Network (outgoing)
            vdu_metrics.network.outgoing.packets = metrics.network.outgoing.packets
            vdu_metrics.network.outgoing.packet_rate = metrics.network.outgoing.packet_rate
            vdu_metrics.network.outgoing.bytes = metrics.network.outgoing.bytes
            vdu_metrics.network.outgoing.byte_rate = metrics.network.outgoing.byte_rate

            # External ports
            vdu_metrics.external_ports.total = len(vdur.external_interface)

            # Internal ports
            vdu_metrics.internal_ports.total = len(vdur.internal_interface)

            # TODO publish the metrics at the VDU-level

            return vdu_metrics

        except Exception as e:
            self.log.exception(e)
            raise

    @asyncio.coroutine
    def request_vnf_metrics(self, vnfr_id):
        try:
            # self.log.debug('request_vnf_metrics: {}'.format(vnfr_id))

            # For each VDU contained within the VNF, create a task to
            # retrieve the NFVI metrics associated with that VDU.
            tasks = list()
            for vdu in self.records.vdurs(vnfr_id):
                task = self.loop.create_task(self.request_vdu_metrics(vdu))
                tasks.append(task)

            vnf_metrics = RwVnfrYang.YangData_Vnfr_VnfrCatalog_Vnfr_NfviMetrics()

            # If there is no pending data, early out
            if not tasks:
                return vnf_metrics

            # Wait for the tasks to complete. Aggregate the results and
            # return them.
            yield from asyncio.wait(tasks, loop=self.loop)

            # TODO aggregated the metrics
            for task in tasks:
                vdu_metrics = task.result()

                # VCPU
                vnf_metrics.vcpu.total += vdu_metrics.vcpu.total
                vnf_metrics.vcpu.utilization += vdu_metrics.vcpu.total * vdu_metrics.vcpu.utilization

                # Memory (in bytes)
                vnf_metrics.memory.used += vdu_metrics.memory.used
                vnf_metrics.memory.total += vdu_metrics.memory.total
                vnf_metrics.memory.utilization += vdu_metrics.memory.used

                # Storage
                vnf_metrics.storage.used += vdu_metrics.storage.used
                vnf_metrics.storage.total += vdu_metrics.storage.total
                vnf_metrics.storage.utilization += vdu_metrics.storage.used

                # Network (incoming)
                vnf_metrics.network.incoming.packets += vdu_metrics.network.incoming.packets
                vnf_metrics.network.incoming.packet_rate += vdu_metrics.network.incoming.packet_rate
                vnf_metrics.network.incoming.bytes += vdu_metrics.network.incoming.bytes
                vnf_metrics.network.incoming.byte_rate += vdu_metrics.network.incoming.byte_rate

                # Network (outgoing)
                vnf_metrics.network.outgoing.packets += vdu_metrics.network.outgoing.packets
                vnf_metrics.network.outgoing.packet_rate += vdu_metrics.network.outgoing.packet_rate
                vnf_metrics.network.outgoing.bytes += vdu_metrics.network.outgoing.bytes
                vnf_metrics.network.outgoing.byte_rate += vdu_metrics.network.outgoing.byte_rate

                # External ports
                vnf_metrics.external_ports.total += vdu_metrics.external_ports.total

                # Internal ports
                vnf_metrics.internal_ports.total += vdu_metrics.internal_ports.total


            # TODO find out the correct way to determine the number of
            # active and inactive VMs in a VNF
            vnf_metrics.vm.active_vm = len(tasks)
            vnf_metrics.vm.inactive_vm = 0

            # VCPU (note that VCPU utilization if a weighted average)
            if vnf_metrics.vcpu.total > 0:
                vnf_metrics.vcpu.utilization /= vnf_metrics.vcpu.total

            # Memory (in bytes)
            if vnf_metrics.memory.total > 0:
                vnf_metrics.memory.utilization *= 100.0 / vnf_metrics.memory.total

            # Storage
            if vnf_metrics.storage.total > 0:
                vnf_metrics.storage.utilization *= 100.0 / vnf_metrics.storage.total

            # TODO publish the VNF-level metrics

            return vnf_metrics

        except Exception as e:
            self.log.exception(e)
            raise

    @asyncio.coroutine
    def request_ns_metrics(self, ns_instance_config_ref):
        try:
            # self.log.debug('request_ns_metrics: {}'.format(ns_instance_config_ref))

            # Create a task for each VNFR to retrieve the NFVI metrics
            # associated with that VNFR.
            vnfrs = self.records.vnfr_ids(ns_instance_config_ref)
            tasks = list()
            for vnfr in vnfrs:
                task = self.loop.create_task(self.request_vnf_metrics(vnfr))
                tasks.append(task)

            ns_metrics = RwNsrYang.YangData_Nsr_NsInstanceOpdata_Nsr_NfviMetrics()

            # If there are any VNFR tasks, wait for them to finish
            # before beginning the next iteration.
            if tasks:
                yield from asyncio.wait(tasks, loop=self.loop)

            # Aggregate the VNFR metrics
            for task in tasks:
                vnf_metrics = task.result()

                ns_metrics.vm.active_vm += vnf_metrics.vm.active_vm
                ns_metrics.vm.inactive_vm += vnf_metrics.vm.inactive_vm

                # VCPU
                ns_metrics.vcpu.total += vnf_metrics.vcpu.total
                ns_metrics.vcpu.utilization += vnf_metrics.vcpu.total * vnf_metrics.vcpu.utilization

                # Memory (in bytes)
                ns_metrics.memory.used += vnf_metrics.memory.used
                ns_metrics.memory.total += vnf_metrics.memory.total
                ns_metrics.memory.utilization += vnf_metrics.memory.used

                # Storage
                ns_metrics.storage.used += vnf_metrics.storage.used
                ns_metrics.storage.total += vnf_metrics.storage.total
                ns_metrics.storage.utilization += vnf_metrics.storage.used

                # Network (incoming)
                ns_metrics.network.incoming.packets += vnf_metrics.network.incoming.packets
                ns_metrics.network.incoming.packet_rate += vnf_metrics.network.incoming.packet_rate
                ns_metrics.network.incoming.bytes += vnf_metrics.network.incoming.bytes
                ns_metrics.network.incoming.byte_rate += vnf_metrics.network.incoming.byte_rate

                # Network (outgoing)
                ns_metrics.network.outgoing.packets += vnf_metrics.network.outgoing.packets
                ns_metrics.network.outgoing.packet_rate += vnf_metrics.network.outgoing.packet_rate
                ns_metrics.network.outgoing.bytes += vnf_metrics.network.outgoing.bytes
                ns_metrics.network.outgoing.byte_rate += vnf_metrics.network.outgoing.byte_rate

                # External ports
                ns_metrics.external_ports.total += vnf_metrics.external_ports.total

                # Internal ports
                ns_metrics.internal_ports.total += vnf_metrics.internal_ports.total

            # VCPU (note that VCPU utilization if a weighted average)
            if ns_metrics.vcpu.total > 0:
                ns_metrics.vcpu.utilization /= ns_metrics.vcpu.total

            # Memory (in bytes)
            if ns_metrics.memory.total > 0:
                ns_metrics.memory.utilization *= 100.0 / ns_metrics.memory.total

            # Storage
            if ns_metrics.storage.total > 0:
                ns_metrics.storage.utilization *= 100.0 / ns_metrics.storage.total

            return ns_metrics

        except Exception as e:
            self.log.exception(e)
            raise

    @asyncio.coroutine
    def publish_nfvi_metrics(self, ns_instance_config_ref):
        nfvi_xpath = "D,/nsr:ns-instance-opdata/nsr:nsr[nsr:ns-instance-config-ref='{}']/rw-nsr:nfvi-metrics"
        nfvi_xpath = nfvi_xpath.format(ns_instance_config_ref)

        registration_handle = yield from self.dts.register(
                xpath=nfvi_xpath,
                handler=rift.tasklets.DTS.RegistrationHandler(),
                flags=(RwDts.Flag.PUBLISHER | RwDts.Flag.NO_PREP_READ),
                )

        self.log.debug('preparing to publish NFVI metrics for {}'.format(ns_instance_config_ref))

        try:
            # Create the initial metrics element
            ns_metrics = RwNsrYang.YangData_Nsr_NsInstanceOpdata_Nsr_NfviMetrics()
            registration_handle.create_element(nfvi_xpath, ns_metrics)

            prev = time.time()
            while True:
                # Use a simple throttle to regulate the frequency that the
                # VDUs are sampled at.
                curr = time.time()

                if curr - prev < self.polling_period:
                    pause = self.polling_period - (curr - prev)
                    yield from asyncio.sleep(pause, loop=self.loop)

                prev = time.time()

                # Retrieve the NS NFVI metrics
                ns_metrics = yield from self.request_ns_metrics(ns_instance_config_ref)

                # Check that that NSR still exists
                if not self.records.has_nsr(ns_instance_config_ref):
                    break

                # Publish the NSR metrics
                registration_handle.update_element(nfvi_xpath, ns_metrics)

        except Exception as e:
            self.log.exception(e)
            raise

        finally:
            # Make sure that the NFVI metrics are removed from the operational
            # data
            yield from registration_handle.delete_element(nfvi_xpath)
            self.log.debug('deleted: {}'.format(nfvi_xpath))

            # Now that we are done with the registration handle, tell DTS to
            # deregister it
            registration_handle.deregister()

            self.log.debug('finished publishing NFVI metrics for {}'.format(ns_instance_config_ref))


class RecordManager(object):
    """
    There are two mappings that this class is reponsible for maintaining. The
    first is a mapping from the set of NSR IDs to the VNFR IDs contained within
    the network service,

        nsr-id
        |-- vnfr-id-1
        |-- vnfr-id-2
        |-- ...
        \-- vnfr-id-n

    The second, maps the set of VNFR IDs to the VDUR structures contains within
    those network functions,

        vnfr-id
        |-- vdur-1
        |-- vdur-2
        |-- ...
        \-- vdur-m


    Note that the VDURs can be identified by the vim-id contained in the VDUR
    structure.

    It is important to understand that the current model of the system does not
    have a direct connection from an NSR to a VNFR or VDUR. This means that the
    NSR structure does not contain any VNFR/VDUR information, and it would be
    necessary to query DTS in order to retrieve VNFR/VDUR information. This
    class manages the two mappings to keep track of the NSRs and VNFRs so that
    it is unnecessary to query DTS in order to determine which VNFRs/VDURs are
    contained within a given NSR. On the other hand, a VNFR does in fact
    contain VDUR information.

    Finally, note that it is necessary to retain the mapping from the VNFR to
    the VDUR because NFVI metric aggregation needs to publish aggregate
    information at both the NS ans VNF levels.

    """

    def __init__(self):
        self._nsr_to_vnfrs = dict()
        self._vnfr_to_vdurs = dict()

        # A mapping from the VDURs VIM ID to the VDUR structure
        self._vdurs = dict()

    def add_nsr(self, nsr):
        """Add an NSR to the manager

        Arguments:
            nsr - an NSR structure

        """
        if nsr.constituent_vnfr_ref:
            if nsr.ns_instance_config_ref not in self._nsr_to_vnfrs:
                self._nsr_to_vnfrs[nsr.ns_instance_config_ref] = set()

            mapping = self._nsr_to_vnfrs[nsr.ns_instance_config_ref]
            mapping.update(nsr.constituent_vnfr_ref)

    def add_vnfr(self, vnfr):
        """Add a VNFR to the manager

        Arguments:
            vnfr - a VNFR structure

        """
        # Create a list of VDURs filtering out the VDURs that have not been
        # assigned a vim-id
        vdurs = [vdur for vdur in vnfr.vdur if vdur.vim_id is not None]

        # There are no valid VDURs, early out now
        if not vdurs:
            return

        # Create a set for the VNFR if necessary
        if vnfr.id not in self._vnfr_to_vdurs:
            self._vnfr_to_vdurs[vnfr.id] = set()

        # Update the vnfr-id mapping
        mapping = self._vnfr_to_vdurs[vnfr.id]
        mapping.update(vdur.vim_id for vdur in vdurs)

        # Update the vdur mapping
        self._vdurs.update((vdur.vim_id, vdur) for vdur in vdurs)

    def has_nsr(self, nsr_id):
        """Returns True if the specified NSR ID is in the record manager

        Arguments:
            nsr_id - the ID of the NSR to check

        Returns:
            a boolean indicating whether the record manager contains the NSR

        """
        return nsr_id in self._nsr_to_vnfrs

    def has_vnfr(self, vnfr_id):
        """Returns True if the specified VNFR ID is in the record manager

        Arguments:
            vnfr_id - the ID of the VNFR to check

        Returns:
            a boolean indicating whether the record manager contains the VNFR

        """
        return vnfr_id in self._vnfr_to_vdurs

    def remove_vnfr(self, vnfr_id):
        """Remove the specified VNFR

        The VNFR will be removed along with any of the associated VDURs.

        Arguments:
            vnfr_id - the ID of the VNFR to remove

        """
        if vnfr_id not in self._vnfr_to_vdurs:
            return

        # Construct a set of VDURs to be deleted from the dict of vdurs
        vdur_ids = self._vnfr_to_vdurs[vnfr_id]
        vdur_ids &= set(self._vdurs.keys())

        # Remove the VDUR structures
        for vdur_id in vdur_ids:
            del self._vdurs[vdur_id]

        # Remove the mapping from the VNFR to the VDURs
        del self._vnfr_to_vdurs[vnfr_id]

    def remove_nsr(self, nsr_id):
        """Removes the specified NSR

        Note that none of the VNFRs associated with the NSR are removed; This
        is related to the separation between the NSR and VNFR in the yang model
        (see above). The removal of VNFRs is assumed to be a separate action.

        Arguments:
            nsr_id - the ID of the NSR to remove

        """
        del self._nsr_to_vnfrs[nsr_id]

    def vdurs(self, vnfr_id):
        """Return a list of the VDURs associated with a VNFR

        Arguments:
            vnfr_id - the ID of the VNFR

        Returns:
            a list of VDURs

        """
        vdurs = self._vnfr_to_vdurs.get(vnfr_id, set())
        return [self._vdurs[id] for id in vdurs]

    def vdur_ids(self, vnfr_id):
        """Return a list of the VDUR IDs associated with a VNFR

        Arguments:
            vnfr_id - the ID of the VNFR

        Returns:
            a list of VDUR IDs

        """
        return list(self._vnfr_to_vdurs.get(vnfr_id, list()))

    def vnfr_ids(self, nsr_id):
        """Return a list of the VNFR IDs associated with a NSR

        Arguments:
            nsr_id - the ID of the NSR

        Returns:
            a list of VNFR IDs

        """
        return list(self._nsr_to_vnfrs.get(nsr_id, list()))
