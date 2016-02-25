#!/bin/bash

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#


fore_ground=0
with_gdb=0
on_exit=0
reuse_confd_ws=0

while :; do
  key="$1"
  case $key in
     -bg|--fore-ground)
      fore_ground=1
      shift
     ;;
     -gdb|--with-gdb)
      with_gdb=1
      shift
     ;;
     --no-clean-exit)
      on_exit=1
      shift
     ;;
     --reuse-confd-ws)
      reuse_confd_ws=1
      shift
     ;;
     *)
      break
     ;;
  esac
  shift
done

cleanup_and_exit() {
  rm -rf ${RIFT_INSTALL}/tmp/agent_ut
  exit 0
}

system_cmd="./mgmt_tbed.py"
if [ ${with_gdb} -eq 1 ]; then
  system_cmd+=" --with-gdb"
fi

if [ ${on_exit} -eq 1 ]; then
  system_cmd+="  --no-clean-exit"
fi

if [ ${reuse_confd_ws} -eq 1 ]; then
  system_cmd+=" --reuse-confd-ws"
fi

# Start the system test
if [ ${fore_ground} -eq 1 ]; then
   ${system_cmd}
else
  cat /dev/null | ${system_cmd}& 
fi

system_pid=$!
trap "echo \"Killing system (pid: ${system_pid})\"; kill ${system_pid} 2>/dev/null; cleanup_and_exit" SIGINT SIGTERM SIGABRT SIGHUP

wait ${system_pid}

# Remove the temporary ut directory
if [ ${on_exit} -eq 0 ]
then
  echo "Cleaning up the temporary directory"
  cleanup_and_exit
fi
