#!/bin/sh

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#


SCREEN_NAME='rwlp'
SCREENRC='/home/rift/.install/var/rift/services/riftware-launchpad/rwlp.screenrc'

echo " ";
echo "Starting RiftWare Launchpad in a detached screen, to connect to #rift-shell: screen -r ${SCREEN_NAME}";
echo " ";
sleep 1;
/usr/bin/screen -d -m -S ${SCREEN_NAME} -c ${SCREENRC}
