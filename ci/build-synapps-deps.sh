#!/bin/bash
# Make sure we exit on any error
set -e

mkdir external
cd external

#wget -nv https://github.com/epics-modules/asyn/archive/R4-26.tar.gz
#tar -zxf R4-26.tar.gz
git clone https://github.com/epics-modules/asyn asyn-R4-31
echo "EPICS_BASE=/usr/lib/epics" > asyn-R4-31/configure/RELEASE
#echo "EPICS_LIBCOM_ONLY=YES" >> asyn-R4-31/configure/CONFIG_SITE
make -C asyn-R4-31/

git clone https://github.com/areaDetector/ADSupport.git
echo "EPICS_BASE=/usr/lib/epics" > ADSupport/configure/RELEASE.linux-x86_64.Common
#echo "HDF5=/usr"                             >> ADSupport/configure/CONFIG_SITE.linux-x86_64.Common
#echo "HDF5_LIB=/usr/lib"                     >> ADSupport/configure/CONFIG_SITE.linux-x86_64.Common
#echo "HDF5_INCLUDE=-I/usr/include"           >> ADSupport/configure/CONFIG_SITE.linux-x86_64.Common
make -C ADSupport

cd ..

