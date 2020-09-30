#!/bin/bash
# Make sure we exit on any error
set -e

mkdir external
cd external
python3 -m venv pvi-venv
source pvi-venv/bin/activate

git clone https://github.com/dls-controls/pvi.git
cd pvi
git checkout dev
git submodule update --init -- src/pvi/submodules/asyn
git submodule update --init -- src/pvi/submodules/busy
git submodule update --init -- src/pvi/submodules/copyInfo
git show
pip install .[cli]

cd ../..

echo "PVI=`pwd`/external/pvi" >> configure/RELEASE.local
echo "BIN_PVI=`pwd`/external/pvi-venv/bin/pvi" >> configure/RELEASE.local
