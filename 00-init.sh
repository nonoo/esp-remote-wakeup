#!/bin/bash
set -e

apt update ; apt install -y python3 python-is-python3 python3-venv libusb-1.0-0 cmake

scriptdir=${0%`basename "$0"`}
cd $scriptdir
scriptdir=`pwd`

. .common.inc.sh

git submodule update --init --recursive

cd esp-idf
git submodule update --init --recursive
cd ..

rm -rf esp-idf-tools
mkdir -p esp-idf-tools
esp-idf/install.sh
rm -rf esp-idf-tools/dist

esp-idf/tools/idf_tools.py install-python-env

ENV=$scriptdir/esp-idf-tools/python_env/idf5.1_py3.11_env/bin/python
$ENV -m pip install "setuptools<81" wheel

echo "init done"
