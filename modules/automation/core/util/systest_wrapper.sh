#!/bin/bash
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Austin Cormier
# Creation Date: 2015/04/28
#
# Script for invoking and managing separate system, configuration and test processes.
#
#    systest_wrapper.sh --system_cmd "<system_cmd> --test_cmd "<test_cmd>" [--config_cmd "<config_cmd>"]
#                       [--dts_cmd "<dts_cmd>"] [--sink_cmd "<sink_cmd>"] [--wait] [--sysinfo]
#
# The system is command is run in the background while the config command
# (if provided) and test commands are run sequentially in the foreground.
# Since the config/test commands are run immediately after the system is started,
# they must be capable of determining when the system is ready to be configured/tested.
#
# Arguments:
#   system_cmd - The command which starts the system (which runs in the background).
#   config_cmd - The command which configures the running system in preparation to be tested.
#   test_cmd   - The command which tests the now started an configured system.
#   dts_cmd    - The command which enables dts tracing for a client, such as uAgent.
#   sink_cmd   - The command which is responsible for setting up the logging sink.
#   wait       - Block after test until receiving interrupt or kill signal
#   sysinfo    - The option to "show sysinfo" when the test fails
#
# Exit Code:
#   0 if the system started successfully and the provided config/test commands.
#   The exit code of the config_cmd if unsuccessful, otherwise the test_cmd exit code.

# If wait is specified, block until receiving an INT or TERM signal
# before continuing on to shut down the system

system_pid=0
curr_pid=0
curr_rc=0
config_cmd=""
test_cmd=""
system_cmd=""
up_cmd=""
dts_cmd=""
sink_cmd=""
teardown_wait=false
show_sysinfo_cmd=":"
systest_pipe=""
tmp_dir=""
pid2kill=0
max_kill_time=60
mypid=$$

function handle_teardown_wait(){
    if [ ${teardown_wait} = true ]; then
        wait_cond=false
        done_wait() {
            wait_cond=true
        }
        trap done_wait SIGTERM SIGINT
        echo "Scheduled system tests complete. Waiting for interrupt to resume teardown"
        while [ ${wait_cond} != true ]; do
            read
        done < ${systest_pipe}
    fi
}

function wait_for_killed_process_termination()
{
    time=0
    while [ -d /proc/${pid2kill} ] ; do
       if [ $time -ge $max_kill_time ] ; then
           echo "WARNING: Unable to kill pid: ${pid2kill}"
           # self-terminate if $pid2kill is still alive
           kill -9 $mypid 2>/dev/null
       fi
       sleep 1
       time=$((time+1))
    done
}

function handle_exit()
{
    # Give the system a chance to shutdown, and send it a SIGKILL to ensure
    # it gets killed if it decides to hang around.
    handle_teardown_wait

    trap "" EXIT SIGINT SIGTERM SIGHUP
    trap - EXIT SIGINT SIGTERM SIGHUP

    # cleanup temporaries
    rm "${systest_pipe}" 2>/dev/null
    rmdir ${tmp_dir} 2>/dev/null

    if [ "$1" -eq 0 ]; then
       pid2kill=$system_pid
    else
       curr_rc=1
       echo "Exiting with system_rc: $curr_rc"
       pid2kill=$curr_pid
    fi

    # run in the background the command waiting for termination of pid:$pid2kill
    wait_for_killed_process_termination&

    kill -9 $pid2kill 2>/dev/null
    wait $pid2kill

    exit ${curr_rc}
}

# Uncomment if you do not wish core files to be created
# ulimit -c 0

while [[ $# > 1 ]]
do
  key="$1"

  case $key in
    -c|--config_cmd)
      config_cmd="$2"
      shift
      ;;
    -t|--test_cmd)
      test_cmd="$2"
      shift
      ;;
    -u|--up_cmd)
     up_cmd="$2"
      shift
      ;;
    -s|--system_cmd)
      system_cmd="$2"
      shift
      ;;
    -d|--dts_cmd)
      dts_cmd="$2"
      shift
      ;;
    -s|--sink_cmd)
      sink_cmd="$2"
      shift
      ;;
    --wait)
      teardown_wait=true
      ;;
    --sysinfo)
      show_sysinfo_cmd="$RIFT_INSTALL/demos/show_sysinfo.py"
      ;;
    *)
      echo "ERROR: Got an unknown option: $key"
      exit 1
      ;;
  esac
  shift
done

echo "my pid: $$"

if [ "${system_cmd}" == "" ]; then
    echo "ERROR: system_cmd was not provided."
    exit 1
fi

if [ "${up_cmd}" == "" ]; then
    echo "ERROR: up_cmd was not provided."
    exit 1
fi

if [ "${test_cmd}" == "" ]; then
    echo "ERROR: test_cmd was not provided."
    exit 1
fi


echo "Launching system using command: ${system_cmd}"
echo "-------------------------------------------------------"

trap "echo \"Received SIGHUP\"; handle_exit 1" SIGHUP
trap "echo \"Received EXIT\"; handle_exit 0" EXIT

# We want to launch the system within a separate process group in the background
# so the reaper doesn't affect us.  Pipe /dev/null into the system to give it a
# "usable" stdin.  Otherwise we've seen ssh fail.
{ set -o monitor; cat /dev/null | ${system_cmd}& }
system_pid=$!

# Set to receive SIGINT signal upon abnormal termination of the system command
( while [ -d /proc/${system_pid} ] ; do sleep 1 ; done ; kill -SIGHUP $$ 2>/dev/null )&

{ set -o monitor; cat /dev/null | ${up_cmd}& }
curr_pid=$!
wait $curr_pid
curr_rc=$?
echo "DEBUG: Got up command rc: $curr_rc"
if [[ ${curr_rc} -ne 0 ]]; then
    echo "Exiting with up_cmd_rc: $curr_rc"
    exit ${curr_rc}
fi

# If a command for sink configuration is provided, run it and proceed with the
# system test regardless of the return value.
if [ "${sink_cmd}" != "" ]; then
    { set -o monitor; cat /dev/null | ${sink_cmd}& }
    curr_pid=$!
    wait $curr_pid
    curr_rc=$?
    echo "DEBUG: Got sink rc: $curr_rc"
fi

# If a dts command was provided, run it in the foreground and capture the return value.
# The system test will continue regardless of the dts_cmd's return value.
if [ "${dts_cmd}" != "" ]; then
    { set -o monitor; cat /dev/null | ${dts_cmd}& }
    curr_pid=$!
    wait $curr_pid
    curr_rc=$?
    echo "DEBUG: Got dts rc: $curr_rc"
fi

# If a separate config command was provided, run it in the foreground
# and capture the return value.
if [ "${config_cmd}" != "" ]; then
    { set -o monitor; cat /dev/null | ${config_cmd}& }
    curr_pid=$!
    wait $curr_pid
    curr_rc=$?
    echo "DEBUG: Got config rc: $curr_rc"
    if [[ ${curr_rc} -ne 0 ]]; then
        echo "Exiting with config_rc: $curr_rc"
        exit ${curr_rc}
    fi
fi

# Save log output to temporaries so that we can output the test results after
# the system has been shutdown
# cleanup temporaries on EXIT
tmp_dir=$(mktemp -d)
systest_pipe="${tmp_dir}/systest_pipe"
mkfifo ${systest_pipe}

# Test the system in the foreground
# Save the return code so we can exit with the correct exit code.

if [ "${test_cmd}" != "" ]; then
    { set -o monitor; cat /dev/null | ${test_cmd}& }
    curr_pid=$!
    wait $curr_pid
    curr_rc=$?
    echo "DEBUG: Got test rc: $curr_rc"
fi

echo "Exiting with test_rc: $curr_rc"
handle_exit 0
