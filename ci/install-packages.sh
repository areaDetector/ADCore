#!/bin/bash
# Make sure we exit on any error
set -e

# Set USE_EPICS_DEV to YES to use the Debian epics-dev package
USE_EPICS_DEV=NO

# Set these flags to control whether to use a package, and if so to install or build
WITH_BOOST=YES
BOOST_EXTERNAL=YES
WITH_HDF5=YES
HDF5_EXTERNAL=NO
XML2_EXTERNAL=NO
WITH_NETCDF=YES
NETCDF_EXTERNAL=NO
WITH_NEXUS=YES
NEXUS_EXTERNAL=NO
WITH_TIFF=YES
TIFF_EXTERNAL=NO
WITH_JPEG=YES
JPEG_EXTERNAL=NO
WITH_SZIP=YES
SZIP_EXTERNAL=NO
WITH_ZLIB=YES
ZLIB_EXTERNAL=NO
WITH_BLOSC=YES
BLOSC_EXTERNAL=NO
WITH_BITSHUFFLE=YES
BITSHUFFLE_EXTERNAL=NO

mkdir external

if [[ $USE_EPICS_DEV == "YES" ]]; then
  #Enabling the NSLS-II EPICS debian package repositories
  curl http://epics.nsls2.bnl.gov/debian/repo-key.pub | sudo apt-key add -
  echo "deb http://epics.nsls2.bnl.gov/debian/ wheezy main contrib" | sudo tee -a /etc/apt/sources.list
  echo "deb-src http://epics.nsls2.bnl.gov/debian/ wheezy main contrib" | sudo tee -a /etc/apt/sources.list
  sudo apt-get install epics-dev
  EPICS_BASE=/usr/lib/epics
else
  cd external
  wget -nv https://epics.anl.gov/download/base/base-3.15.5.tar.gz
  tar -zxf base-3.15.5.tar.gz
  ln -s base-3.15.5 epics_base
  make -sj -C epics_base/
  EPICS_BASE=`pwd`/epics_base/
  cd ..
fi

# Set these flags appropriately
echo "EPICS_BASE = "     $EPICS_BASE         >  configure/RELEASE.local
echo "ASYN=`pwd`/external/asyn-R4-32"        >> configure/RELEASE.local
echo "ADSUPPORT=`pwd`/external/ADSupport"    >> configure/RELEASE.local
echo "WITH_BOOST     ="  $WITH_BOOST         >> configure/CONFIG_SITE.linux-x86_64.Common
echo "BOOST_EXTERNAL ="  $BOOST_EXTERNAL     >> configure/CONFIG_SITE.linux-x86_64.Common
echo "WITH_HDF5 = "      $WITH_HDF5          >> configure/CONFIG_SITE.linux-x86_64.Common
echo "HDF5_EXTERNAL = "  $HDF5_EXTERNAL      >> configure/CONFIG_SITE.linux-x86_64.Common
echo "XML2_EXTERNAL = "  $XML2_EXTERNAL      >> configure/CONFIG_SITE.linux-x86_64.Common
echo "WITH_NETCDF ="     $WITH_NETCDF        >> configure/CONFIG_SITE.linux-x86_64.Common
echo "NETCDF_EXTERNAL =" $NETCDF_EXTERNAL    >> configure/CONFIG_SITE.linux-x86_64.Common
echo "WITH_NEXUS = "     $WITH_NEXUS         >> configure/CONFIG_SITE.linux-x86_64.Common
echo "NEXUS_EXTERNAL = " $NEXUS_EXTERNAL     >> configure/CONFIG_SITE.linux-x86_64.Common
echo "WITH_TIFF = "      $WITH_TIFF          >> configure/CONFIG_SITE.linux-x86_64.Common
echo "TIFF_EXTERNAL = "  $TIFF_EXTERNAL      >> configure/CONFIG_SITE.linux-x86_64.Common
echo "WITH_JPEG = "      $WITH_JPEG          >> configure/CONFIG_SITE.linux-x86_64.Common
echo "JPEG_EXTERNAL = "  $JPEG_EXTERNAL      >> configure/CONFIG_SITE.linux-x86_64.Common
echo "WITH_SZIP = "      $WITH_SZIP          >> configure/CONFIG_SITE.linux-x86_64.Common
echo "SZIP_EXTERNAL = "  $SZIP_EXTERNAL      >> configure/CONFIG_SITE.linux-x86_64.Common
echo "WITH_ZLIB = "      $WITH_ZLIB          >> configure/CONFIG_SITE.linux-x86_64.Common
echo "ZLIB_EXTERNAL = "  $ZLIB_EXTERNAL      >> configure/CONFIG_SITE.linux-x86_64.Common
echo "WITH_BLOSC= "      $WITH_BLOSC         >> configure/CONFIG_SITE.linux-x86_64.Common
echo "BLOSC_EXTERNAL = " $BLOSC_EXTERNAL     >> configure/CONFIG_SITE.linux-x86_64.Common
echo "HOST_OPT=NO"                           >> configure/CONFIG_SITE.linux-x86_64.Common
echo "USR_CXXFLAGS_Linux=--coverage"         >> configure/CONFIG_SITE.linux-x86_64.Common
echo "USR_LDFLAGS_Linux=--coverage"          >> configure/CONFIG_SITE.linux-x86_64.Common

echo "======= configure/RELEASE.local ========================================="
cat configure/RELEASE.local

echo "======= configure/CONFIG_SITE.linux-x86_64.Common ======================="
cat configure/CONFIG_SITE.linux-x86_64.Common


# Installing the latest 3rd party packages
sudo apt-get update -qq

# The following are only installed if WITH_XXX=YES and XXX_EXTERNAL=YES
if [[ $WITH_BOOST == "YES" && $BOOST_EXTERNAL == "YES" ]]; then
  sudo apt-get install libboost-test-dev
fi

if [[ $WITH_HDF5 == "YES" && $HDF5_EXTERNAL == "YES" ]]; then
  sudo apt-get install libhdf5-serial-dev
fi

if [[ $WITH_TIFF == "YES" && $TIFF_EXTERNAL == "YES" ]]; then
  sudo apt-get install libtiff-dev
fi

if [[ $XML2_EXTERNAL == "YES" ]]; then
  sudo apt-get install libxml2-dev
fi

# TO DO: Install ZLIB, SZIP, JPEG, NEXUS, NETCDF if we want to use package versions

# Installing latest version of code coverage tool lcov (because the ubuntu package is very old)
wget https://sourceforge.net/projects/ltp/files/Coverage%20Analysis/LCOV-1.11/lcov-1.11.tar.gz
tar -zxf lcov-1.11.tar.gz
sudo make -C lcov-1.11 install
gem install coveralls-lcov

cd external

# Install asyn
wget -nv https://github.com/epics-modules/asyn/archive/R4-32.tar.gz
tar -zxf R4-32.tar.gz
echo "EPICS_BASE="$EPICS_BASE > asyn-R4-32/configure/RELEASE
#echo "EPICS_LIBCOM_ONLY=YES" >> asyn-R4-32/configure/CONFIG_SITE
make -sj -C asyn-R4-32/

# Install ADSupport
git clone https://github.com/areaDetector/ADSupport.git
echo "EPICS_BASE="$EPICS_BASE > ADSupport/configure/RELEASE.linux-x86_64.Common
# Copy the same config site file generated above for ADSupport
cp ../configure/CONFIG_SITE.linux-x86_64.Common ADSupport/configure
make -sj -C ADSupport

cd ..

