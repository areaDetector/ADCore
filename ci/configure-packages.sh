#!/bin/bash
# Make sure we exit on any error
set -e

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
