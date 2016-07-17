/* NDFileHDF5VersionCheck.h
 *
 * The purpose of this file is to implement the version check
 * macro that is missing in some older versions of the HDF5
 * library.  The individual version numbers are still defined
 * but the actual macro for checking was introduced in the
 * later versions of the library.
 *
 * Alan Greer
 * February 18, 2016
 */
#ifndef NDFILEHDF5VERSIONCHECK_H_
#define NDFILEHDF5VERSIONCHECK_H_

#include <hdf5.h>

#ifndef H5_VERSION_GE
#define H5_VERSION_GE(Maj,Min,Rel) \
       (((H5_VERS_MAJOR==Maj) && (H5_VERS_MINOR==Min) && (H5_VERS_RELEASE>=Rel)) || \
        ((H5_VERS_MAJOR==Maj) && (H5_VERS_MINOR>Min)) || \
        (H5_VERS_MAJOR>Maj))
#endif

#endif // NDFILEHDF5VERSIONCHECK_H_

