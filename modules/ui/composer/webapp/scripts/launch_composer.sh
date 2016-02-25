#!/bin/bash

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#


# change to the directory of this script
THIS_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd $THIS_DIR
cd ..
echo "Running composer UI"
cd dist
ps -ef | awk '/[s]cripts\/server_composer_ui.py/{print $2}' | kill -9
python ../scripts/server_composer_ui.py
while true; do
  sleep 5
done
