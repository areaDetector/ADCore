#!/bin/bash

mkdir external
cd external

wget -nv http://www.aps.anl.gov/epics/download/modules/asyn4-26.tar.gz
tar -zxf asyn4-26.tar.gz
echo "EPICS_BASE=/usr/lib/epics" > asyn4-26/configure/RELEASE
#echo "EPICS_LIBCOM_ONLY=YES" >> asyn4-26/configure/CONFIG_SITE
make -C asyn4-26/

cd ..

