import socket
import subprocess
import time

import pytest

from gi.repository import (
        NsrYang,
        NsdYang,
        RwConmanYang,
        RwMcYang,
        RwNsrYang,
        VlrYang,
        RwVlrYang,
        VnfdYang,
        VnfrYang,
        RwVnfrYang
        )
import rift.auto.session
import rift.mano.examples.ping_pong_nsd as ping_pong


@pytest.fixture(scope='module')
def mc_proxy(request, mgmt_session):
    return mgmt_session.proxy(RwMcYang)


@pytest.fixture(scope='module')
def launchpad_session(request, mc_proxy, mgmt_domain_name, session_type):
    launchpad_host = mc_proxy.get("/mgmt-domain/domain[name='%s']/launchpad/ip_address" % mgmt_domain_name)

    if session_type == 'netconf':
        launchpad_session = rift.auto.session.NetconfSession(host=launchpad_host)
    elif session_type == 'restconf':
        launchpad_session = rift.auto.session.RestconfSession(host=launchpad_host)

    launchpad_session.connect()
    rift.vcs.vcs.wait_until_system_started(launchpad_session)
    return launchpad_session


@pytest.fixture(scope='module')
def ping_pong_records(request):
    return ping_pong.generate_ping_pong_descriptors(pingcount=1)


@pytest.fixture(scope='module')
def proxy(request, launchpad_session):
    return launchpad_session.proxy


@pytest.mark.depends('pingpong')
@pytest.mark.usefixtures('splat_launchpad')
@pytest.mark.incremental
class TestPingPongRecords(object):
    def is_valid_ip(self, address):
        """Verifies if it is a valid IP and if its accessible

        Args:
            address (str): IP address

        Returns:
            boolean
        """
        try:
            socket.inet_aton(address)
        except socket.error:
            return False
        else:
            return True

    def yield_vnfd_vnfr_pairs(self, proxy):
        """
        Yields tuples of vnfd & vnfr entries.

        Args:
            proxy (callable): Launchpad proxy

        Yields:
            Tuple: VNFD and its corresponding VNFR entry
        """
        def get_vnfd(vnfd_id):
            xpath = "/vnfd-catalog/vnfd[id='{}']".format(vnfd_id)
            return proxy(VnfdYang).get(xpath)

        vnfr = "/vnfr-catalog/vnfr"
        vnfrs = proxy(RwVnfrYang).get(vnfr, list_obj=True)
        for vnfr in vnfrs.vnfr:
            vnfd = get_vnfd(vnfr.vnfd_ref)
            yield vnfd, vnfr

    def yield_nsd_nsr_pairs(self, proxy):
        """Yields tuples of NSD & NSR pairs

        Args:
            proxy (callable): Launchpad proxy

        Yields:
            Tuple: NSD and its corresponding NSR record
        """
        nsr = "/ns-instance-opdata/nsr"
        nsrs = proxy(RwNsrYang).get(nsr, list_obj=True)
        for nsr in nsrs.nsr:
            nsd_path = "/ns-instance-config/nsr[id='{}']".format(
                    nsr.ns_instance_config_ref)
            nsd = proxy(RwNsrYang).get_config(nsd_path)

            yield nsd, nsr

    def test_vdu_record_params(self, proxy):
        """
        Asserts:
        1. If a valid floating IP has been assigned to the VM
        3. Check if the VM flavor has been copied over the VDUR
        """
        for vnfd, vnfr in self.yield_vnfd_vnfr_pairs(proxy):
            assert vnfd.mgmt_interface.port == vnfr.mgmt_interface.port

            for vdud, vdur in zip(vnfd.vdu, vnfr.vdur):
                assert vdud.vm_flavor == vdur.vm_flavor
                assert self.is_valid_ip(vdur.management_ip) is True
                assert vdud.external_interface[0].vnfd_connection_point_ref == \
                    vdur.external_interface[0].vnfd_connection_point_ref

    def test_external_vl(self, proxy):
        """
        Asserts:
        1. Valid IP for external connection point
        2. A valid external network fabric
        3. Connection point names are copied over
        """
        for vnfd, vnfr in self.yield_vnfd_vnfr_pairs(proxy):
            cp_des, cp_rec = vnfd.connection_point, vnfr.connection_point
            assert cp_des[0].name == cp_rec[0].name
            assert self.is_valid_ip(cp_rec[0].ip_address) is True

            xpath = "/vlr-catalog/vlr[id='{}']/network-id".format(cp_rec[0].vlr_ref)
            network_id = proxy(VlrYang).get(xpath)
            assert len(network_id) > 0

    def test_monitoring_params(self, proxy):
        """
        Asserts:
        1. The value counter ticks?
        2. If the meta fields are copied over
        """
        def mon_param_record(vnfr_id, mon_param_id):
             return '/vnfr-catalog/vnfr[id="{}"]/monitoring-param[id="{}"]'.format(
                    vnfr_id, mon_param_id)

        for vnfd, vnfr in self.yield_vnfd_vnfr_pairs(proxy):
            for mon_des in (vnfd.monitoring_param):
                mon_rec = mon_param_record(vnfr.id, mon_des.id)
                mon_rec = proxy(VnfrYang).get(mon_rec)

                # Meta data check
                fields = mon_des.as_dict().keys()
                for field in fields:
                    assert getattr(mon_des, field) == getattr(mon_rec, field)
                # Tick check
                #assert mon_rec.value_integer > 0

    def test_nsr_record(self, proxy):
        """
        Currently we only test for the components of NSR tests. Ignoring the
        operational-events records

        Asserts:
        1. The constituent components.
        2. Admin status of the corresponding NSD record.
        """
        for nsd, nsr in self.yield_nsd_nsr_pairs(proxy):
            # 1 n/w and 2 connection points
            assert len(nsr.vlr) == 1
            assert len(nsr.vlr[0].vnfr_connection_point_ref) == 2

            assert len(nsr.constituent_vnfr_ref) == 2
            assert nsd.admin_status == 'ENABLED'

    def test_create_update_vnfd(self, proxy, ping_pong_records):
        """
        Verify VNFD related operations

        Asserts:
            If a VNFD record is created
        """
        ping_vnfd, pong_vnfd, _ = ping_pong_records
        vnfdproxy = proxy(VnfdYang)

        for vnfd_record in [ping_vnfd, pong_vnfd]:
            xpath = "/vnfd-catalog/vnfd"
            vnfdproxy.create_config(xpath, vnfd_record.vnfd)

            xpath = "/vnfd-catalog/vnfd[id='{}']".format(vnfd_record.id)
            vnfd = vnfdproxy.get(xpath)
            assert vnfd.id == vnfd_record.id

            vnfdproxy.replace_config(xpath, vnfd_record.vnfd)

    def test_create_update_nsd(self, proxy, ping_pong_records):
        """
        Verify NSD related operations

        Asserts:
            If NSD record was created
        """
        _, _, ping_pong_nsd = ping_pong_records
        nsdproxy = proxy(NsdYang)

        xpath = "/nsd-catalog/nsd"
        nsdproxy.create_config(xpath, ping_pong_nsd.descriptor)

        xpath = "/nsd-catalog/nsd[id='{}']".format(ping_pong_nsd.id)
        nsd = nsdproxy.get(xpath)
        assert nsd.id == ping_pong_nsd.id

        nsdproxy.replace_config(xpath, ping_pong_nsd.descriptor)

    def test_cm_nsr(self, proxy):
        """
        Asserts:
            1. The ID of the NSR in cm-state
            2. Name of the cm-nsr
            3. The vnfr component's count
            4. State of the cm-nsr
        """
        for nsd, _ in self.yield_nsd_nsr_pairs(proxy):
            con_nsr_xpath = "/cm-state/cm-nsr[id='{}']".format(nsd.id)
            con_data = proxy(RwConmanYang).get(con_nsr_xpath)

            assert con_data is not None, \
                    "No Config data obtained for the nsd {}: {}".format(
                    nsd.name, nsd.id)
            assert con_data.name == "ping_pong_nsd"
            assert len(con_data.cm_vnfr) == 2

            state_path = con_nsr_xpath + "/state"
            proxy(RwConmanYang).wait_for(state_path, 'received', timeout=120)

    def test_cm_vnfr(self, proxy):
        """
        Asserts:
            1. The ID of Vnfr in cm-state
            2. Name of the vnfr
            3. State of the VNFR
            4. Checks for a reachable IP in mgmt_interface
            5. Basic checks for connection point and cfg_location.
        """
        def is_reachable(ip):
            rc = subprocess.call(["ping", "-c1", ip])
            if rc == 0:
                return True
            return False

        nsd, _ = list(self.yield_nsd_nsr_pairs(proxy))[0]
        con_nsr_xpath = "/cm-state/cm-nsr[id='{}']".format(nsd.id)

        for _, vnfr in self.yield_vnfd_vnfr_pairs(proxy):
            con_vnfr_path = con_nsr_xpath + "/cm-vnfr[id='{}']".format(vnfr.id)
            con_data = proxy(RwConmanYang).get(con_vnfr_path)

            assert con_data is not None
            assert vnfr.name == con_data.name

            state_path = con_vnfr_path + "/state"
            proxy(RwConmanYang).wait_for(state_path, 'ready', timeout=60)

            con_data = proxy(RwConmanYang).get(con_vnfr_path)
            assert is_reachable(con_data.mgmt_interface.ip_address) is True

            assert len(con_data.connection_point) == 1
            connection_point = con_data.connection_point[0]
            assert connection_point.name == vnfr.connection_point[0].name
            assert connection_point.ip_address == vnfr.connection_point[0].ip_address

            assert con_data.cfg_location is not None
