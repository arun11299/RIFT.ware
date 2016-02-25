#!/bin/bash

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

server_pid=0

# change to the directory of this script
THIS_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

function handle_received_signal(){
    forever stopall && kill -9 $server_pid 2>/dev/null
    wait $server_pid
    echo "Terminated web server PID: $server_pid"
    exit
}

cd $THIS_DIR
echo "Running webserver"
cd ../public
ps -ef | awk '/[s]scripts\/server_rw.ui_ui.py/{print $2}' | kill -9 2>/dev/null

../scripts/server_rw.ui_ui.py&
server_pid=$!

echo "Running API server"
cd ../../api
forever stopall
forever start -a -l forever.log -o out.log -e err.log server.js

# Ensure that the forever script is stopped when this script exits
trap "echo \"Received EXIT\"; handle_received_signal" EXIT
trap "echo \"Received SIGINT\"; handle_received_signal" SIGINT
trap "echo \"Received SIGKILL\"; handle_received_signal" SIGKILL
trap "echo \"Received SIGABRT\"; handle_received_signal" SIGABRT
trap "echo \"Received SIGQUIT\"; handle_received_signal" SIGQUIT
trap "echo \"Received SIGSTOP\"; handle_received_signal" SIGSTOP
trap "echo \"Received SIGTERM\"; handle_received_signal" SIGTERM
trap "echo \"Received SIGTRAP\"; handle_received_signal" SIGTRAP

# Keep this script in the foreground so the system doesn't think that the
# server crashed.
while true; do
  sleep 5
done
