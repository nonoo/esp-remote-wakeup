#!/bin/bash
set -e

scriptdir=${0%`basename "$0"`}
cd $scriptdir
scriptdir=`pwd`

. .common.inc.sh

# This script makes sure esp-idf is updated to the hash set in .common.inc.sh

if [ ! -d esp-idf ]; then
	git clone https://github.com/espressif/esp-idf/
fi

cd esp-idf
git reset --hard
git clean -dffx
cd ..

if ! esp_idf_is_up_to_date; then
	cd esp-idf
	git fetch
	git checkout $esp_idf_hash
	cd ..
fi

cd esp-idf

git submodule update --init --recursive

cd ..

rm -rf esp-idf-tools
mkdir -p esp-idf-tools
esp-idf/install.sh
rm -rf esp-idf-tools/dist

esp-idf/idf_tools.py install-python-env

echo "init done"
