#!/bin/sh

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

# This script needs to run on jenkins(charm) !
# copy it from the source tree to:
#
# or call out of /usr/rift/scripts/packaging ?
#


#####################################################################################################
## Arguments
#####################################################################################################

# $1 is the dir to find rpms
# $2 is the build_number
# $3 is the repo to update, ex. nightly or testing

DIR=$1
BUILD=$2
REPO=$3

# copy or move? optional argument
#CPMV=$3
CPMV="copy"

# RW version. optional argument
#VER=$4
VER=4.0

#####################################################################################################
## Arguments
#####################################################################################################


REPO_BASE=/net/boson/home1/autobot/www/mirrors/releases/riftware/${VER}/20/x86_64
#LOG=/tmp/createrepo-${REPO}.log
LOG=/var/log/createrepo/createrepo.log
ME=`basename "$0"`
LOCK=/tmp/createrepo-${REPO}.lock.d

GLOB=${DIR}/*-${BUILD}*.rpm

function log()
{
	echo "(`date`) [update-riftware-repo $$] $1 " >> $LOG 2>&1
}

function update_buildsys()
{
       	log "[update_buildsys] Updating Build System";

       	setrepo="${RIFT_ROOT:-/usr/rift}/scripts/packaging/setrepo"

        if [ -f ${setrepo} ]; then
               	log "[update_buildsys] Calling ${setrepo} ${BUILD} ${REPO} "
                ${setrepo} ${BUILD} ${REPO}
        fi

}

# use a mkdir file lock as mkdir is supposed to be atomic over NFS
# http://unix.stackexchange.com/questions/70/what-unix-commands-can-be-used-as-a-semaphore-lock
create_lock_or_wait () {

	log "[create_lock_or_wait] lock=${LOCK}"

	while true; do
		#log "lockDir: `ls -all ${LOCK}` "

        	if mkdir "${LOCK}"; then
			log "[create_lock_or_wait] got the lock!"
        		break;
	        fi

		log "[create_lock_or_wait] LOCKED! ...sleeping..."
	        sleep 60
	done

}

remove_lock () {
	rmdir "${LOCK}"
	log "[remove_lock] lock=${LOCK} removed lock! "
	#log "lockDir: SHOULD_BE_GONE! `ls -all ${LOCK}` "
}


#function procs_running()
#{
#	#NUM=`ps aux | grep -i $ME | grep -iv grep | grep -iv ssh | wc -l`
#	NUM=`pgrep -f $ME -c`
#	log " "
#	log "[procs_running]: num=$NUM "
#	log "[procs_running]: PS= `ps aux | grep -i $ME | grep -iv grep | grep -iv ssh ` "
#	log " "
#	log "[procs_running]: PGREP=  `pgrep -a -f $ME ` "
#	log " "
#	echo "$NUM"
#	#return $NUM
#	#return `ps aux | grep -i $ME | grep -iv grep | grep -iv ssh | wc -l`
#}


#####################################################################################################
## START of main
#####################################################################################################

log " "
log "START @ `date` "
log "ME=$ME "
log "ARGS=$1 $2 $3 "
log "GLOB=${GLOB} "
log "REPO=${REPO} "
log " "

if [ "${DIR}" == "" ]; then
	echo "You must pass in the directory as arg1 to find the RPMS. Quitting."
	exit 1
fi

if [ "${BUILD}" == "" ]; then
	echo "You must pass in the build_number as arg2. Quitting."
	exit 1
fi

if [ "${REPO}" == "" ]; then
	echo "You must pass in the repo as arg3. Quitting."
	exit 1
fi

# really this should only be run by jenkins so the permissions are correct in the repo
if [ `whoami` != 'jenkins' ]; then
	echo "You must be jenkins to do this. Quitting."
	exit 1
fi

if ! which createrepo >/dev/null; then
	echo "createrepo not found! Quitting."
	exit 1;
fi




#NUM_RUNNING=`ps aux | grep -i $ME | grep -iv grep | grep -iv ssh | wc -l` # must ignore ssh too 
#NUM_RUNNING=$(procs_running)
NUM_RUNNING=`pgrep -f $ME -c`


#echo "[update-riftware-repo] glob=${GLOB} repo=${REPO} num_running=${NUM_RUNNING} " >> $LOG 2>&1
#echo "[update-riftware-repo] glob=${GLOB} repo=${REPO}" >> $LOG 2>&1
#echo "[update-riftware-repo] repo=${REPO} " >> $LOG 2>&1
#log "glob=${GLOB} repo=${REPO} num_running=${NUM_RUNNING}"

# each copy of this script runs inside ssh so that means 2 procs returned by pgrep
#while [ "$NUM_RUNNING" -gt 2 ]; do
	#$NUM_RUNNING=`ps aux | grep -i $ME | grep -iv grep | grep -iv ssh | wc -l`
	#NUM_RUNNING=`procs_running`
	#NUM_RUNNING=$(procs_running)
#	NUM_RUNNING=`pgrep -f $ME -c`

	#echo "[update-riftware-repo] glob=${GLOB} repo=${REPO} num_running=${NUM_RUNNING} ...sleeping..." >> $LOG 2>&1
#	log "glob=${GLOB} repo=${REPO} num_running=${NUM_RUNNING} ...sleeping..."
#	sleep 60
#done

# lock
create_lock_or_wait


# cp/mv glob from staging to real repo spot
if [ "${CPMV}" == "copy" ]; then
	log "CMD: cp -vf ${GLOB} ${REPO_BASE}/${REPO}/ "
	cp -vf ${GLOB} ${REPO_BASE}/${REPO}/ >> $LOG 2>&1
	# put real cp here!!!
fi

log "CDing to ${REPO_BASE}/${REPO}"
#cd /net/boson/home1/autobot/www/mirrors/releases/riftware/4.0/20/x86_64/${REPO} || exit 1;
cd ${REPO_BASE}/${REPO} || { echo "Failed to cd to ${REPO_BASE}/${REPO}. Exitting."; remove_lock; exit 1; }

# run setrepo to update the build system
update_buildsys

#
# http://linux.die.net/man/8/createrepo
#
# --update
# If metadata already exists in the outputdir and an rpm is unchanged (based on file size and mtime) since the metadata was generated, reuse the existing metadata rather than recalculating it.
# In the case of a large repository with only a few new or modified rpms this can significantly reduce I/O and processing time.

# make the simple repo first
#echo "[update-riftware-repo] running createrepo" >> $LOG 2>&1
log "running createrepo basic"
createrepo --update --profile . >> $LOG 2>&1

log "sleeping a bit"
sleep 10

# then the deltas for testing
if [ "${REPO}" == "testing" ]; then
	#echo "[update-riftware-repo] running createrepo with deltas" >> $LOG 2>&1
	log "running createrepo with deltas"
	createrepo -v --max-delta-rpm-size 5000000000 --oldpackagedirs . --deltas --num-deltas=5 --cachedir=/tmp/repo-${REPO}.cache --profile . >> $LOG 2>&1
fi

#unlock
remove_lock

log "END @ `date`"
log " "

