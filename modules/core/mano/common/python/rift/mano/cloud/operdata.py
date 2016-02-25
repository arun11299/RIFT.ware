import asyncio
import rift.tasklets

from gi.repository import(
        RwCloudYang,
        RwDts as rwdts,
        )

class CloudAccountNotFound(Exception):
    pass


class CloudAccountDtsOperdataHandler(object):
    def __init__(self, dts, log, loop):
        self._dts = dts
        self._log = log
        self._loop = loop

        self.cloud_accounts = {}

    def add_cloud_account(self, account):
        self.cloud_accounts[account.name] = account
        account.start_validate_credentials(self._loop)

    def delete_cloud_account(self, account_name):
        del self.cloud_accounts[account_name]

    def _register_show_status(self):
        def get_xpath(cloud_name=None):
            return "D,/rw-cloud:cloud/account{}/connection-status".format(
                    "[name='%s']" % cloud_name if cloud_name is not None else ''
                    )

        @asyncio.coroutine
        def on_prepare(xact_info, action, ks_path, msg):
            path_entry = RwCloudYang.CloudAccount.schema().keyspec_to_entry(ks_path)
            cloud_account_name = path_entry.key00.name
            self._log.debug("Got show cloud connection status request: %s", ks_path.create_string())

            if not cloud_account_name:
                self._log.warning("Cloud account name %s not found", cloud_account_name)
                xact_info.respond_xpath(rwdts.XactRspCode.NA)
                return

            try:
                account = self.cloud_accounts[cloud_account_name]
            except KeyError:
                self._log.warning("Cloud account %s does not exist", cloud_account_name)
                xact_info.respond_xpath(rwdts.XactRspCode.NA)
                return

            connection_status = account.connection_status
            self._log.debug("Responding to cloud connection status request: %s", connection_status)
            xact_info.respond_xpath(
                    rwdts.XactRspCode.MORE,
                    xpath=get_xpath(cloud_account_name),
                    msg=account.connection_status,
                    )

            xact_info.respond_xpath(rwdts.XactRspCode.ACK)

        yield from self._dts.register(
                xpath=get_xpath(),
                handler=rift.tasklets.DTS.RegistrationHandler(
                    on_prepare=on_prepare),
                flags=rwdts.Flag.PUBLISHER,
                )

    def _register_validate_rpc(self):
        def get_xpath():
            return "/rw-cloud:update-cloud-status"

        @asyncio.coroutine
        def on_prepare(xact_info, action, ks_path, msg):
            if not msg.has_field("cloud_account"):
                raise CloudAccountNotFound("Cloud account name not provided")

            cloud_account_name = msg.cloud_account
            try:
                account = self.cloud_accounts[cloud_account_name]
            except KeyError:
                raise CloudAccountNotFound("Cloud account name %s not found" % cloud_account_name)

            account.start_validate_credentials(self._loop)

            xact_info.respond_xpath(rwdts.XactRspCode.ACK)

        yield from self._dts.register(
                xpath=get_xpath(),
                handler=rift.tasklets.DTS.RegistrationHandler(
                    on_prepare=on_prepare
                    ),
                flags=rwdts.Flag.PUBLISHER,
                )

    @asyncio.coroutine
    def register(self):
        yield from self._register_show_status()
        yield from self._register_validate_rpc()
