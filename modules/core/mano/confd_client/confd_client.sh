#!/bin/bash

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

echo "Starting confd"
cd $RIFT_ROOT/.install
pwd
./usr/bin/rw_confd&


echo "Starting the confd_client"
cd $RIFT_ROOT/.build/modules/core/mc/src/core_mc-build/confd_client/
./confd_client&

echo "sleeping for 20 secs for confd to complete initialization"
sleep 20

cd $RIFT_ROOT/modules/core/mc/confd_client
time ./test.sh



# echo "Testing confd config write performance"
# echo "Sending 20 create fedaration requests..."

# time for i in `seq 1 20`; do curl -d '{"federation": {"name": "foobar'$i'"}}' -H 'Content-Type: application/vnd.yang.data+json' http://localhost:8008/api/running -uadmin:admin -v; done
 
# echo "Testing confd config read performance"
# echo "Sending 200 read fedaration requests..."
# time for i in `seq 1 50`; do curl -s -H 'Content-Type: application/vnd.yang.data+json' http://localhost:8008/api/running/federation -uadmin:admin -v -X GET; done

# echo "Testing confd operational data get performance"
# echo "Sending 20 create fedaration requests..."

# time for i in `seq 1 200`; do  curl -s -H "Content-Type: application/vnd.yang.data+json" http://localhost:8008/api/operational/opdata -uadmin:admin -v -X GET; done

killall confd
trap 'kill $(jobs -pr)' SIGINT SIGTERM EXIT
