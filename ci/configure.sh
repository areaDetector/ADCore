#!/bin/bash
# Configure ADCore in preparation for build
# Make sure we exit on any error
set -e

# Generate the configure/RELEASE.local and configure/CONFIG_SITE.linux-x86_64.Common
# with the details of where to find various external libraries.
echo "EPICS_BASE=/usr/lib/epics"             >> configure/RELEASE.local

echo "WITH_HDF5 = YES"                       >> configure/CONFIG_SITE.linux-x86_64.Common
echo "HDF5_EXTERNAL = YES"                   >> configure/CONFIG_SITE.linux-x86_64.Common

echo "WITH_XML2     = YES"                   >> configure/CONFIG_SITE.linux-x86_64.Common
echo "XML2_EXTERNAL = YES"                   >> configure/CONFIG_SITE.linux-x86_64.Common
echo "XML2_INCLUDE=-I/usr/include/libxml2"   >> configure/CONFIG_SITE.linux-x86_64.Common

echo "WITH_BOOST     = YES"                  >> configure/CONFIG_SITE.linux-x86_64.Common
echo "BOOST_EXTERNAL = YES"                  >> configure/CONFIG_SITE.linux-x86_64.Common

echo "HOST_OPT=NO"                           >> configure/CONFIG_SITE.linux-x86_64.Common 
echo "USR_CXXFLAGS_Linux=--coverage"         >> configure/CONFIG_SITE.linux-x86_64.Common 
echo "USR_LDFLAGS_Linux=--coverage"          >> configure/CONFIG_SITE.linux-x86_64.Common 

echo "ASYN=`pwd`/external/asyn-R4-26"        >> configure/RELEASE.local
echo "ADSUPPORT=`pwd`/external/ADSupport"    >> configure/RELEASE.local

echo "======= configure/RELEASE.local ========================================="
cat configure/RELEASE.local

echo "======= configure/CONFIG_SITE.linux-x86_64.Common ======================="
cat configure/CONFIG_SITE.linux-x86_64.Common

