#!/bin/bash

scriptdir=${0%`basename "$0"`}
cd $scriptdir
scriptdir=`pwd`

. .common.inc.sh

parse_args $@
get_idf

while [ 1 ]; do
	if [ ! -c $port_monitor ]; then
		echo -ne "\n\n\nwaiting for $port_monitor to appear"
		while [ ! -c $port_monitor ]; do
			echo -n "."
			sleep 1
		done
	fi

	$idf --port $port_monitor monitor --no-reset
	if [ $? -eq 0 ]; then
		break
	fi
	rm -f /var/lock/LCK..`basename ${port_monitor}`

	# Fixing terminal line endings messed up by minicom.
	stty sane

	sleep 1
done
