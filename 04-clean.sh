#!/bin/bash
set -e

scriptdir=${0%`basename "$0"`}
cd $scriptdir
scriptdir=`pwd`

. .common.inc.sh

get_idf
do_clean
