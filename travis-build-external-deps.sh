#!/bin/bash

export EPICS_BASE=/usr/lib/epics
export EPICS_HOST_ARCH=linux-x86_64

mkdir external
cd external
wget -nv http://www.aps.anl.gov/bcda/synApps/tar/sscan_R2-10.tar.gz
tar -zxf sscan_R2-10.tar.gz
echo "EPICS_BASE=/usr/lib/epics" > sscan-2-10/configure/RELEASE
make -C sscan-2-10/

wget -nv http://www.aps.anl.gov/bcda/synApps/tar/calc_R3-4-2.tar.gz
tar -zxf calc_R3-4-2.tar.gz
echo "SSCAN=`pwd`/sscan-2-10" > calc-3-4-2/configure/RELEASE
echo "EPICS_BASE=/usr/lib/epics" >> calc-3-4-2/configure/RELEASE
make -C calc-3-4-2/

wget -nv http://www.aps.anl.gov/epics/download/modules/asyn4-26.tar.gz
tar -zxf asyn4-26.tar.gz
echo "EPICS_BASE=/usr/lib/epics" > asyn4-26/configure/RELEASE
#echo "EPICS_LIBCOM_ONLY=YES" >> asyn4-26/configure/CONFIG_SITE
make -C asyn4-26/


wget -nv http://www.aps.anl.gov/bcda/synApps/tar/busy_R1-6-1.tar.gz
tar -zxf busy_R1-6-1.tar.gz
echo "EPICS_BASE=/usr/lib/epics" > busy-1-6-1/configure/RELEASE
echo "ASYN=`pwd`/asyn4-26" >> busy-1-6-1/configure/RELEASE
make -C busy-1-6-1/

wget -nv http://www.aps.anl.gov/bcda/synApps/tar/autosave_R5-5.tar.gz
tar -zxf autosave_R5-5.tar.gz
echo "EPICS_BASE=/usr/lib/epics" > autosave-5-5/configure/RELEASE
make -C autosave-5-5/

cd ..

