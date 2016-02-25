# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import asyncio
import concurrent.futures
import time

from gi.repository import (
    NsrYang,
    RwNsrYang,
    RwDts as rwdts)

import rift.tasklets


class ConfigAgentJob(object):
    """A wrapper over the config agent job object, providing some
    convenience functions.

    YangData_Nsr_NsInstanceOpdata_Nsr_ConfigAgentJob contains
    ||
     ==> VNFRS
          ||
           ==> Primitives

    """
    # The normalizes the state terms from Juju to our yang models
    # Juju : Yang model
    STATUS_MAP = {"completed": "success",
                  "pending"  : "pending",
                  "running"  : "pending",
                  "failed"   : "failure"}

    def __init__(self, nsr_id, job, tasks=None):
        """
        Args:
            nsr_id (uuid): ID of NSR record
            job (YangData_Nsr_NsInstanceOpdata_Nsr_ConfigAgentJob): Gi object
            tasks: List of asyncio.tasks. If provided the job monitor will
                use it to monitor the tasks instead of the execution IDs
        """
        self._job = job
        self.nsr_id = nsr_id
        self.tasks = tasks

    @property
    def id(self):
        """Job id"""
        return self._job.job_id

    @property
    def name(self):
        """Job name"""
        return self._job.job_name

    @property
    def job_status(self):
        """Status of the job (success|pending|failure)"""
        return self._job.job_status

    @job_status.setter
    def job_status(self, value):
        """Setter for job status"""
        self._job.job_status = value

    @property
    def job(self):
        """Gi object"""
        return self._job

    @property
    def xpath(self):
        """Xpath of the job"""
        return ("D,/nsr:ns-instance-opdata" +
                "/nsr:nsr[nsr:ns-instance-config-ref='{}']" +
                "/nsr:config-agent-job[nsr:job-id='{}']"
                ).format(self.nsr_id, self.id)

    @staticmethod
    def convert_rpc_input_to_job(nsr_id, rpc_output, tasks):
        """A helper function to convert the YangOutput_Nsr_ExecNsConfigPrimitive
        to YangData_Nsr_NsInstanceOpdata_Nsr_ConfigAgentJob (NsrYang)

        Args:
            nsr_id (uuid): NSR ID
            rpc_output (YangOutput_Nsr_ExecNsConfigPrimitive): RPC output
            tasks (list): A list of asyncio.Tasks

        Returns:
            ConfigAgentJob
        """
        # Shortcuts to prevent the HUUGE names.
        CfgAgentJob = NsrYang.YangData_Nsr_NsInstanceOpdata_Nsr_ConfigAgentJob
        CfgAgentVnfr = NsrYang.YangData_Nsr_NsInstanceOpdata_Nsr_ConfigAgentJob_Vnfr
        CfgAgentPrimitive = NsrYang.YangData_Nsr_NsInstanceOpdata_Nsr_ConfigAgentJob_Vnfr_Primitive

        job = CfgAgentJob.from_dict({
                "job_id": rpc_output.job_id,
                "job_name" : rpc_output.name,
                "job_status": "pending",
            })

        for vnfr in rpc_output.vnf_out_list:
            vnfr_job = CfgAgentVnfr.from_dict({
                    "id": vnfr.vnfr_id_ref,
                    "vnf_job_status": "pending",
                    })

            for primitive in vnfr.vnf_out_primitive:
                vnf_primitive = CfgAgentPrimitive.from_dict({
                        "name": primitive.name,
                        "execution_status": ConfigAgentJob.STATUS_MAP[primitive.execution_status],
                        "execution_id": primitive.execution_id
                    })
                vnfr_job.primitive.append(vnf_primitive)

            job.vnfr.append(vnfr_job)

        return ConfigAgentJob(nsr_id, job, tasks)


class ConfigAgentJobMonitor(object):
    """Job monitor: Polls the Juju controller and get the status.
    Rules:
        If all Primitive are success, then vnf & nsr status will be "success"
        If any one Primitive reaches a failed state then both vnf and nsr will fail.
    """
    POLLING_PERIOD = 2

    def __init__(self, dts, log, job, executor, loop, config_plugin):
        """
        Args:
            dts : DTS handle
            log : log handle
            job (ConfigAgentJob): ConfigAgentJob instance
            executor (concurrent.futures): Executor for juju status api calls
            loop (eventloop): Current event loop instance
            config_plugin : Config plugin to be used.
        """
        self.job = job
        self.log = log
        self.loop = loop
        self.executor = executor
        self.polling_period = ConfigAgentJobMonitor.POLLING_PERIOD
        self.config_plugin = config_plugin
        self.dts = dts

    @asyncio.coroutine
    def _monitor_processes(self, registration_handle):
        result = 0
        for process in self.job.tasks:
            rc = yield from process
            self.log.debug("Process {} returned rc: {}".format(process, rc))
            result |= rc

        if result == 0:
            self.job.job_status = "success"
        else:
            self.job.job_status = "failure"

        registration_handle.update_element(self.job.xpath, self.job.job)


    @asyncio.coroutine
    def publish_action_status(self):
        """
        Starts publishing the status for jobs/primitives
        """
        registration_handle = yield from self.dts.register(
                xpath=self.job.xpath,
                handler=rift.tasklets.DTS.RegistrationHandler(),
                flags=(rwdts.Flag.PUBLISHER | rwdts.Flag.NO_PREP_READ),
                )

        self.log.debug('preparing to publish job status for {}'.format(self.job.xpath))

        try:
            registration_handle.create_element(self.job.xpath, self.job.job)

            # If the config is done via a user defined script
            if self.job.tasks is not None:
                yield from self._monitor_processes(registration_handle)
                return

            prev = time.time()
            # Run until pending moves to either failure/success
            while self.job.job_status == "pending":
                curr = time.time()

                if curr - prev < self.polling_period:
                    pause = self.polling_period - (curr - prev)
                    yield from asyncio.sleep(pause, loop=self.loop)

                prev = time.time()

                tasks = []
                for vnfr in self.job.job.vnfr:
                    task = self.loop.create_task(self.get_vnfr_status(vnfr))
                    tasks.append(task)

                # Exit, if no tasks are found
                if not tasks:
                    break

                yield from asyncio.wait(tasks, loop=self.loop)

                job_status = [task.result() for task in tasks]

                if "failure" in job_status:
                    self.job.job_status = "failure"
                elif "pending" in job_status:
                    self.job.job_status = "pending"
                else:
                    self.job.job_status = "success"

                # self.log.debug("Publishing job status: {} at {} for nsr id: {}".format(
                #     self.job.job_status,
                #     self.job.xpath,
                #     self.job.nsr_id))

                registration_handle.update_element(self.job.xpath, self.job.job)


        except Exception as e:
            self.log.exception(e)
            raise


    @asyncio.coroutine
    def get_vnfr_status(self, vnfr):
        """Schedules tasks for all containing primitives and updates it's own
        status.

        Args:
            vnfr : Vnfr job record containing primitives.

        Returns:
            (str): "success|failure|pending"
        """
        tasks = []
        job_status = []

        for primitive in vnfr.primitive:
            if primitive.execution_id == "":
                # TODO: For some config data, the id will be empty, check if
                # mapping is needed.
                job_status.append(primitive.execution_status)
                continue

            task = self.loop.create_task(self.get_primitive_status(primitive))
            tasks.append(task)

        if tasks:
            yield from asyncio.wait(tasks, loop=self.loop)

        job_status.extend([task.result() for task in tasks])
        if "failure" in job_status:
            vnfr.vnf_job_status = "failure"
            return "failure"

        elif "pending" in job_status:
            vnfr.vnf_job_status = "pending"
            return "pending"

        else:
            vnfr.vnf_job_status = "success"
            return "success"

    @asyncio.coroutine
    def get_primitive_status(self, primitive):
        """
        Queries the juju api and gets the status of the execution id.

        Args:
            primitive : Primitive containing the execution ID.
        """

        try:
            status = yield from self.loop.run_in_executor(
                    self.executor,
                    self.config_plugin.get_action_status,
                    primitive.execution_id
                    )
            # self.log.debug("Got {} for execution id: {}".format(
            #         status,
            #         primitive.execution_id))
        except Exception as e:
            self.log.exception(e)
            status = "failed"

        # Handle case status is None
        if status:
            primitive.execution_status = ConfigAgentJob.STATUS_MAP[status]
        else:
            primitive.execution_status = "failure"

        return primitive.execution_status


class CfgAgentJobDtsHandler(object):
    """Dts Handler for CfgAgent"""
    XPATH = "D,/nsr:ns-instance-opdata/nsr:nsr/nsr:config-agent-job"

    def __init__(self, dts, log, loop, nsm, cfgm):
        """
        Args:
            dts  : Dts Handle.
            log  : Log handle.
            loop : Event loop.
            nsm  : NsmManager.
            cfgm : ConfigManager.
        """
        self._dts = dts
        self._log = log
        self._loop = loop
        self._cfgm = cfgm
        self._nsm = nsm

        self._regh = None

    @property
    def regh(self):
        """ Return registration handle """
        return self._regh

    @property
    def nsm(self):
        """ Return the NSManager manager instance """
        return self._nsm

    @property
    def cfgm(self):
        """ Return the ConfigManager manager instance """
        return self._cfgm

    @staticmethod
    def cfg_job_xpath(nsr_id, job_id):
        return ("D,/nsr:ns-instance-opdata" +
                "/nsr:nsr[nsr:ns-instance-config-ref = '{}']" +
                "/nsr:config-agent-job[nsr:job-id='{}']").format(nsr_id, job_id)

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
                    nsr_id = path_entry.key00.ns_instance_config_ref

                    nsr_ids = []
                    if nsr_id is None or nsr_id == "":
                        nsrs = list(self.nsm.nsrs.values())
                        nsr_ids = [nsr.id for nsr in nsrs]
                    else:
                        nsr_ids = [nsr_id]

                    for nsr_id in nsr_ids:
                        job = self.cfgm.get_job(nsr_id)

                        # If no jobs are queued for the NSR
                        if job is None:
                            continue

                        xact_info.respond_xpath(
                            rwdts.XactRspCode.MORE,
                            CfgAgentJobDtsHandler.cfg_job_xpath(nsr_id, job.job_id),
                            job)

                except Exception as e:
                    self._log.exception("Caught exception:", str(e))
                xact_info.respond_xpath(rwdts.XactRspCode.ACK)

            else:
                xact_info.respond_xpath(rwdts.XactRspCode.NA)

        hdl = rift.tasklets.DTS.RegistrationHandler(on_prepare=on_prepare,)
        with self._dts.group_create() as group:
            self._regh = group.register(xpath=CfgAgentJobDtsHandler.XPATH,
                                        handler=hdl,
                                        flags=rwdts.Flag.PUBLISHER,
                                        )


class ConfigAgentJobManager(object):
    """A central class that manager all the Config Agent related data,
    Including updating the status

    TODO: Needs to support multiple config agents.
    """
    def __init__(self, dts, log, loop, nsm):
        """
        Args:
            dts  : Dts handle
            log  : Log handler
            loop : Event loop
            nsm  : NsmTasklet instance
        """
        self.jobs = {}
        self.dts = dts
        self.log = log
        self.loop = loop
        self.nsm = nsm
        self.handler = CfgAgentJobDtsHandler(dts, log, loop, nsm, self)
        self.executor = concurrent.futures.ThreadPoolExecutor(max_workers=1)

    def add_job(self, rpc_output, tasks=None):
        """Once an RPC is trigger add a now job

        Args:
            rpc_output (YangOutput_Nsr_ExecNsConfigPrimitive): Rpc output
            tasks(list) A list of asyncio.Tasks

        """
        nsr_id = rpc_output.nsr_id_ref
        self.jobs[nsr_id] = ConfigAgentJob.convert_rpc_input_to_job(nsr_id, rpc_output, tasks)

        self.log.debug("Creating a job monitor for Job id: {}".format(
                rpc_output.job_id))

        # For every Job we will schedule a new monitoring process.
        job_monitor = ConfigAgentJobMonitor(
            self.dts,
            self.log,
            self.jobs[nsr_id],
            self.executor,
            self.loop,
            self.nsm.config_agent_plugins[0]  # Hack
            )
        task = self.loop.create_task(job_monitor.publish_action_status())

    def get_job(self, nsr_id):
        """Get the job associated with the NSR Id, if present."""
        try:
            return self.jobs[nsr_id].job
        except KeyError:
            return None

    @asyncio.coroutine
    def register(self):
        yield from self.handler.register()
