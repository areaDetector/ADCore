ADCore Releases
===============

The latest untagged master branch can be obtained at
https://github.com/areaDetector/ADCore.

Tagged source code and pre-built binary releases prior to R2-0 are included
in the areaDetector releases available via links at
http://cars.uchicago.edu/software/epics/areaDetector.html.

Tagged source code releases from R2-0 onward can be obtained at 
https://github.com/areaDetector/ADCore/releases.

The versions of EPICS base, asyn, and other synApps modules used for each release can be obtained from 
the EXAMPLE_RELEASE_PATHS.local, EXAMPLE_RELEASE_LIBS.local, and EXAMPLE_RELEASE_PRODS.local
files respectively, in the configure/ directory of the appropriate release of the 
[top-level areaDetector](https://github.com/areaDetector/areaDetector) repository.


Release Notes
=============

R2-5 (October 28, 2016)
========================
### ADSupport
* Added a new repository ADSupport to areaDetector.  This module contains the source code for all 3rd party
  libraries used by ADCore.  The libraries that were previously built in ADCore have been moved
  to this new repository.  These are ADCore/ADApp/netCDFSrc, nexusSrc, and tiffSupport/tiffSrc, and
  tiffSupport/jpegSrc.  ADSupport also replaces the ADBinaries repository.  ADBinaries contained prebuilt
  libraries for Windows for xml2, GraphicsMagick, and HDF5.  It was becoming too difficult to maintain
  these prebuilt libraries to work with different versions of Visual Studio, and also with 32/64 bit, 
  static dynamic, debug/release, and to work with MinGW.  These libraries are now built from source code
  using the EPICS build system in ADSupport.
* The libraries in ADSupport can be built for Windows (Visual Studio or MinGW, 32/64 bit, static/dynamic), 
  Linux (currently Intel architectures only, 32/64 bit), Darwin, and vxWorks 
  (currently big-endian 32-bit architectures only).  
* Previously the only file saving plugin that was supported on vxWorks was netCDF.  Now all file saving 
  plugins are supported on vxWorks 6.x (TIFF, JPEG, netCDF, HDF5, Nexus).  HDF5 and Nexus are not supported
  on vxWorks 5.x because the compiler is too old.
* All 3rd party libraries are now optional.  For each library XXX there are now 4 Makefile variables that control
  support for that library.  XXX can be JPEG, TIFF, NEXUS, NETCDF, HDF5, XML2, SZIP, and ZLIB.
  - WITH_XXX   If this is YES then drivers or plugins that use this library will be built.  If NO then
    drivers and plugins that use this library will not be built.
  - XXX_EXTERNAL  If this is YES then the library is not built in ADSupport, but is rather assumed to be found
    external to the EPICS build system.  If this is NO then the XXX library will be built in ADSupport.
  - XXX_DIR  If this is defined and XXX_EXTERNAL=YES then the build system will search this directory for the 
    XXX library.
  - XXX_INCLUDE If this is defined then the build system will search this directory for the include files for
    the XXX library.
* ADSupport does not currently include support for GraphicsMagick.  This means that GraphicsMagick is not
  currently supported on Windows.  It can be used on Linux and Darwin if it is installed external to areaDetector
  and GRAPHICSMAGICK_EXTERNAL=YES.
* All EPICS modules except base and asyn are now optional.  Previously 
  commonDriverSupport.dbd included "calcSupport.dbd", "sscanSupport.dbd", etc.
  These dbd and libraries are now only included if they are defined in a RELEASE
  file.


### NDPluginPva
* New plugin for exporting NDArrays as EPICS V4 NTNDArrays.  It has an embedded EPICSv4 server to serve the NTNDArrays
  to V4 clients.
  When used with pvaDriver it provides a mechanism for distributing plugin processing across multiple  processes 
  and multiple machines. Thanks to Bruno Martins for this.

### pvaDriver
* New driver for importing an EPICS V4 NTNDArray into areaDetector.  It works by creating a monitor on the 
  specified PV and doing plugin callbacks each time the array changes.  
  When used with NDPluginPva it provides a mechanism for distributing plugin processing across multiple processes 
  and multiple machines.  Thanks to Bruno Martins for this.

### NDFileHDF5
* Added support for Single Writer Multiple Reader (SWMR).  This allows HDF5 files to be read while they are still be
  written.  This feature was added in HDF5 1.10.0-patch1, so this release or higher is required to use the 
  SWMR support in this plugin.
  The file plugin allows selecting whether SWMR support is enabled, and it is disabled by default.
  Files written with SWMR support enabled can only be read by programs built with HDF 1.10 or higher, so SWMR should not
  be enabled if older clients are to read the files.  SWMR is only supported on local, GPFS, and Lustre file systems. 
  It is not supported on NFS or SMB file systems, for example.  Thanks to Alan Greer for this.
  * NOTE: we discovered shortly before releasing ADSupport R1-0 and ADCore R2-5 that the
    Single Writer Multiple Reader (SWMR) support in HDF5 1.10.0-patch1 was broken.
    It can return errors if any of the datasets are of type H5_C_S1 (fixed length strings).
    We were able to reproduce the errors with a simple C program, and sent that to the HDF Group.
    They quickly produced a new unreleased version of HDF5 called 1.10-swmr-fixes that fixed the problem.

    The HDF5 Group plans to release 1.10.1, hopefully before the end of 2016.  That should be
    the first official release that will correctly support SWMR.

    As of the R1-0 release ADSupport contains 2 branches. 
    - master contains the HDF5 1.10.0-patch1 release from the HDF5 Group with only the minor changes
      required to build with the EPICS build system, and to work on vxWorks and mingw.
      These changes are documented in README.epics.  This version should not be used with SWMR
      support enabled because of the known problems described above.
    - swmr-fixes contains the 1.10-swmr-fixes code that the HDF Group provided.
      We had to make some changes to this code to get it to work on Windows.
      It is not an official release, but does appear to correctly support SWMR.
      Users who would like to begin to use SWMR before HDF5 1.10.1 is released can use
      this branch, but must be aware that it is not officially supported. 

* NDAttributes of type NDAttrString are now saved as 1-D array of strings (HDF5 type H5T_C_S1) rather
  than a 2-D array of 8-bit integers (HDF5 type H5T_NATIVE_CHAR), which is the datatype used prior to R2-5.  
  H5T_NATIVE_CHAR is really intended to be an integer data type, and so most HDF5 utilities (h5dump, HDFView, etc.) display
  these attributes by default as an array of integer numbers rather than as a string.  

### NDPosPlugin
* New plugin attach positional information to NDArrays in the form of NDAttributes. This plugin accepts an XML
  description of the position data and then attaches each position to NDArrays as they are passed through the plugin.
  Each position can contain a set of index values and each index is attached as a separate NDAttribute.  
  The plugin operates using a FIFO and new positions can be added to the queue while the plugin is actively 
  attaching position data.  When used in conjunction with the HDF5
  writer it is possible to store NDArrays in an arbitrary pattern within a multi-dimensional dataset.

### NDPluginDriver
* Added the ability to change the QueueSize of a plugin at run-time. This can be very useful,
  particularly for file plugins where an acquisition of N frames is overflowing the queue,
  but increasing the queue can fix the problem. This will be even more useful in ADCore R3-0
  where we plan to eliminate Capture mode in NDPluginFile. Being able to increase the queue does
  everything that Capture mode did, but has the additional advantage that in Capture mode the
  NDArray memory is not allocated from the NDArrayPool, so there is no check on allocating too
  many arrays or too much memory. Using the queue means that arrays are allocated from the pool,
  so the limits on total number of arrays and total memory defined in the constructor will be obeyed.
  This is very important in preventing system freezes if the user accidentally tries allocate all the
  system memory, which can effectively crash the computer.
* Added a new start() method that must be called after the plugin object is created.  This requires
  a small change to all plugins.  The plugins in ADCore/ADApp/pluginSrc can be used as examples.  This
  change was required to prevent a race condition when the plugin only existed for a very short
  time, which happens in the unit tests in ADCore/ADApp/pluginsTests.

### NDPluginTimeSeries
* New plugin for time-series data.  The plugin accepts input arrays of dimensions
  [NumSignals] or [NumSignals, NewTimePoints].  The plugin creates NumSignals 1-D
  arrays of dimension [NumTimPoints], each of which is the time-series for one signal.
  On each callback the new time points are appended to the existing time series arrays.
  The plugin can operate in one of two modes.  In Fixed Length mode the time-series arrays
  are cleared when acquisition starts, and new time points are appended until 
  NumTimePoints points have been received, at which point acquisition stops and further
  callbacks are ignorred.  In Circular Buffer mode when NumTimePoints samples are received
  then acquisition continues with the new time points replacing the oldest ones in the
  circular buffer.  In this mode the exported NDArrays and waveforms always contain the latest 
  NumTimePoints samples, with the first element of the array containing the oldest time
  point and the last element containing the most recent time point.    
* This plugin is used by R7-0 and later of the 
  [quadEM module](https://github.com/epics-modules/quadEM).
  It should also be useful for devices like ADCs, transient digitizers, and other devices
  that produce time-series data on one or more input signals.  
* There is a new ADCSimDetector test application in 
  [areaDetector/ADExample](https://github.com/areaDetector/ADExample) 
  that tests and demonstrates this plugin.  This test application simulates a buffered 
  ADC with 8 input waveform signals.

### NDPluginFFT
* New plugin to compute 1-D or 2-D Fast Fourier transforms.  It exports 1-D or 2-D
  NDArrays containing the absolute value of the FFT.  It creates 1-D waveform
  records of the input, and the real, imaginary, and absolute values of the first row of the FFT.
  It also creates 1-D waveform records of the time and frequency axes, which are useful if the 1-D
  input represents a time-series. The waveform records are convenient for plotting in OPI screens. 
* The FFT algorithm requires that the input array dimensions be a power of 2, but the plugin
  will pad the array to the next larger power of 2 if the input array does not meet this
  requirement.  
* The simDetector test application in 
  [areaDetector/ADExample](https://github.com/areaDetector/ADExample) 
  has a new simulation mode that generates images based on the sums and/or products of 4 sine waves.
  This application is useful for testing and demonstrating the NDPluginFFT plugin.

### NDPluginStats and NDPluginROIStat
* Added waveform record containing NDArray timestamps to time series data arrays. Thanks to
  Stuart Wilkins for this.

### NDPluginCircularBuff
* Initialize the TriggerCalc string to "0" in the constructor to avoid error messages during iocInit
  if the string has not been set to a valid value that is stored with autosave.

### asynNDArrayDriver
* Fixed bug in FilePath handling on Windows. If the file path ended in "/" then it would incorrectly
  report that the directory did not exist when it did.
* Added asynGenericPointerMask to interrupt mask in constructor.  Should always have been there.
* Added asynDrvUserMask to interface mask in constructor.  Should always have been there.

### NDArrayBase.template, NDPluginDriver.cpp
* Set ArrayCallbacks.VAL to 1 so array callbacks are enabled by default.

### NDPluginBase.template
* Changed QueueSize from longin to longout, because the plugin queue size can now be changed at runtime.
  Added longin QueueSize_RBV.
* Changed EnableCallbacks.VAL to $(ENABLED=0), allowing enabling callbacks when loading database,
  but default remains Disable.
* Set the default value of the NDARRAY_ADDR macro to 0 so it does not need to be defined in most cases.
  
### ADApp/op/adl
* Fixed many medm adl files so text fields have correct string/decimal, width and aligmnent attributes to
  improve autoconversion to other display managers.

### nexusSrc/Makefile
* Fixed so it will work when hdf5 and sz libraries are system libraries.
  Used same logic as commonDriverMakefile

### iocBoot
* Deleted commonPlugins.cmd and commonPlugin_settings.req.  These were accidentally restored before the R2-4
  release after renaming them to EXAMPLE_commonPlugins.cmd and EXAMPLE_commonPlugin_settings.req.

### ImageJ EPICS_ADViewer
* Changed to work with 1-D arrays, i.e. nx>0, ny=0, nz=0.  Previously it did not work if ny=0.  This
  is a useful enhancement because the ImageJ Dynamic Profiler can then be used to plot the 1-D array.


R2-4 (September 21, 2015)
========================
### Removed simDetector and iocs directory. 
Previously the simDetector was part of ADCore, and there was an iocs directory that built the simDetector 
application both as part of an IOC and independent of an IOC. This had 2 disadvantages:

1. It prevented building the simDetector IOC with optional plugins that reside in separate
   repositories, such as ffmpegServer and ADPluginEdge.  This is because ADCore needs to
   be built before the optional plugins, but by then the simDetector IOC is already built
   and cannot use the optional plugins.
2. It made ADCore depend on the synApps modules required to build an IOC, not just the
   EPICS base and asyn that are required to build the base classes and plugins.
   It was desirable to minimize such dependencies in ADCore.
  
For these reasons the simDetector driver and IOC code have been moved to a new repository
called ADExample.  This repository is just like any other detector repository.
This solves problem 1 above because the optional plugins can now be built after ADCore
but before ADExample. 

### NDAttribute
* Fixed problem that the sourceType property was never set.

### NDRoiStat[.adl, .edl, ui, .opi]
* Fixed problem with ROI numbers when calling related displays.

### ADApp/pluginTests/
* New directory with unit tests.

### XML schema
* Moved the XML schema files from the iocBoot directory to a new XML_schema directory.

### iocBoot
* Moved commonPlugin_settings.req from ADApp/Db to iocBoot.  
  Renamed commonPlugins.cmd to EXAMPLE_commonPlugins.cmd and commonPlugin_settings.req to
  EXAMPLE_commonPlugin_settings.req.  These files must be copied to commonPlugins.cmd and
  commonPlugin_settings.req respectively.  This was done because these files are typically
  edited locally, and so should not be in git. 
  iocBoot now only contains EXAMPLE_commonPlugins.cmd and EXAMPLE_commonPlugin_settings.req.  
  EXAMPLE_commonPlugins.cmd adds ADCore/iocBoot to the autosave search path.
  
### ADApp
* commonLibraryMakefile has been changed to define xxx_DIR and set LIB_LIBS+ = xxx if xxx_LIB is defined.  
  If xxx_LIB is not defined then xxx_DIR is not defined and it sets LIB_SYS_LIBS += xxx.  
  xxx includes HDF5, SZIP, and OPENCV. 
  commonDriverMakefile has been changed similarly for PROD_LIBS and PROD_SYS_LIBS.
  This allows optional libraries to either searched in the system location or a user-defined location 
  without some conflicts that could previously occur.


R2-3 (July 23, 2015)
========================
### devIocStats and alive modules
* The example iocs in ADCore and other drivers can now optionally be built with the 
  devIocStats module, which provides very useful resource utilization information for the IOC.
  devIocStats runs on all supported architectures.
  The OPI screen can be loaded from Plugins/Other/devIocStats.
  It is enabled by default in areaDetector/configure/EXAMPLE_RELEASE_PRODS.local. 
* The synApps alive module can also be built into detector IOCs.  It provdes status information
  about the IOC to a central server.  It is disabled by default in 
  areaDetector/configure/EXAMPLE_RELEASE_PRODS.local.  areaDetector/INSTALL_GUIDE.md has been
  updated to describe what needs to be done to enable or disable these optional modules.

### NDPluginFile
* Fixed a serious performance problem.  The calls to openFile() and closeFile() in the derived file 
  writing classes were being done with the asynPortDriver mutex locked.
  This meant that driver callbacks were blocked from putting entries on the queue during this time, 
  slowing down the drivers and potentially causing them to drop frames.  
  Changed openFileBase() and closeFileBase() to unlock the mutex during these operations.  
  Testing with the simDetector and ADDexela shows that the new version no longer slows
  down the driver or drops frames at the driver level when saving TIFF files or netCDF files 
  in Single mode.  This required locking the mutex in the derived file writing classes when
  they access the parameter library.

### NDPluginROIStat
* Added time-series support for each of the statistics in each ROI.  This is the same as the
  time-series support in the NDPluginStats and NDPluginAttribute plugins.

### NDFileHDF5
* Bug fixes: 
  * When writing files in Single mode if NumCapture was 0 then the chunking was computed 
    incorrectly and the files could be much larger than they should be.
  * There was a problem with the HDF5 istorek parameter not being set correctly.

### commonDriverMakefile
* Include SNCSEQ libraries if SNCSEQ is defined.  This must be defined if the CALC module
  was built with SNCSEQ support.
* Optionally include DEVIOCSTATS and ALIVE libraries and dbd files if these are defined.


R2-2 (March 23, 2015)
========================
### Compatibility
* This release requires at least R4-26 of asyn because it uses the info(asyn:READOUT,"1") tag
  in databases to have output records update on driver callbacks.
* This release requires R2-2 of areaDetector/ADBinaries
* This release requires R2-2 of areaDetector/areaDetector because of changes to EXAMPLE_CONFIG_SITE.local.
* Detector IOC startup scripts will need a few minor changes to work with this release of ADCore.
  iocBoot/iocSimDetector/st.cmd should be used as an example.
  - The environment variable EPICS_DB_INCLUDE_PATH must be defined and must include $(ADCORE)/db
  - The environment variable CBUFFS must be defined to specify the number of frames buffered in the
    NDPluginCircularBuff plugin, which is loaded by commonPlugins.cmd.
  - When loading NDStdArrays.template NDARRAY_PORT must be specified.  NDPluginBase should no longer be loaded, 
    this is now done automatically via an include in NDStdArrays.template.
  - Example lines:
  ```
   epicsEnvSet("CBUFFS", "500")
   epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")
   dbLoadRecords("NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,
                 TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Int8,FTVL=UCHAR,NELEMENTS=3145728")
  ```
  
### NDPluginROIStat
* New plugin that supports multiple regions-of-interest with simple statistics on each.
  It is more efficient and convenient than the existing NDPluginROI and NDPluginStats when many 
  regions of interest with simple statistics are needed.  Written by Matthew Pearson.
  
### NDPluginCircularBuff
* New plugin that implements a circular buffer.  NDArrays are stored in the buffer until a trigger
  is received.  When a trigger is received it outputs a configurable number of pre-trigger and post-trigger
  NDArrays.  The trigger is based on NDArray attributes using a user-defined calculation.  Written by Edmund Warrick.
  
### NDPluginAttribute
* New plugin that exports attributes of NDArrays as EPICS PVs.  Both scalar (ai records) and time-series
  arrays (waveform records) are exported.  Written by Matthew Pearson.
  
### NDPluginStats
* Added capability to reset the statistics to 0 (or other user-defined values) with a new
  $(P)$(R)Reset sseq records in NDStats.template.  The reset behavior can be changed by 
  configuring these records.

### NDPluginROI
* Remember the requested ROI size and offset.  If the requested values cannot be satisfied due
  to constraints such as binning or the input array size, then use the requested values when the
  constraints no longer apply.

### NDPluginOverlay
* Bug fix: the vertical line in the cross overlay was not drawn correctly if WidthX was greater than 1.

### NDPluginFile
* Created the NDFileCreateDir parameter. This allows file writers to create a controlled number
  of directories in the path of the output file.
* Added the NDFileTempSuffix string parameter. When this string is not empty it will be
  used as a temporary suffix on the output filename while the file is being written. When writing
  is complete, the file is closed and then renamed to have the suffix removed. The rename operation 
  is atomic from a filesystem view and can be used by monitoring applications like rsync or inotify
  to kick off processing applications. 
  
### NDFileHDF5
* Created a separate NDFileHDF5.h file so the class can be exported to other applications.
* Updated the HDF5 library for Windows in ADBinaries from 1.8.7 to 1.8.14.  The names of the libraries has changed,
  so Makefiles in ADCore needed to be changed.  There is a new flag, HDF5_STATIC_BUILD which can be set to
  NO or YES depending on which version of the HDF5 library should be used.  This symbol is now defined in
  areaDetector/configure/EXAMPLE_CONFIG_SITE.local to be YES for static builds and NO for dynamic builds.
  In principle one can use the dynamic version of the HDF5 library when doing a static build, and vice-versa.
  In practice this has not been tested.
* Bug fixes: 
  * The NDArrayPort could not be changed after iocInit.
  * Hardlinks did not work for NDAttribute datasets.
  * Failing to specify a group with "ndattr_default=true" would crash the IOC.
  * NDAttribute datasets of datatype NDAttrString would actually be HDF5 attributes,
    not datasets.  They would only store a single string value, not an array of string values which they 
    should if the file contains multiple NDArrays.
  * NDAttribute datasets were pre-allocated to the size of NumCapture, rather than 
    growing with the number of NDArrays in the file.  This meant that there was no way to specify that the HDF5 file 
    in Stream mode should continue to grow forever until Capture was set to 0.  Specifying NumCapture=0 did not work, 
    it crashed the IOC.  Setting NumCapture to a very large number resulted in wasted file space, 
    because all datasets except the detector dataset were pre-allocated to this large size.
  * HDF5 attributes defined in the XML layout file for NDAttribute datasets and constant datasets
    did not get written to the HDF5 file.
  * The important NDArray properties uniqueID, timeStamp, and epicsTS did not get written to the HDF5 file.
  * NDAttribute datasets had two "automatic" HDF5 attributes, "description" and "name".  These do not provide
    a complete description of the source of the NDAttribute data, and the HDF5 attribute names are prone
    to name conflicts with user-defined attributes.  "description" and "source" have been renamed to 
    NDAttrDescription and NDAttrSource. Two additional automatic attributes have been added, 
    NDAttrSourceType and NDAttrName, which now completely define the source of the NDAttribute data.

### Version information
* Added a new include file, ADCoreVersion.h that defines the macros ADCORE_VERION, ADCORE_REVISION, and
  ADCORE_MODIFICATION.  The release number is ADCORE_VERSION.ADCORE_REVISION.ADCORE_MODIFICATION.
  This can be used by drivers and plugins to allow them to be used with different releases of ADCore.

### Plugins general
* Added epicsShareClass to class definitions so classes are exported with Windows DLLs.
* Install all plugin header files so the classes can be used from other applications.

### netCDFSupport
* Fixes to work on vxWorks 6.9 from Tim Mooney.

### NDAttribute class
* Changed the getValue() method to do type conversion if the requested datatype does not match the
  datatype of the attribute.

### Template files
* Added a new template file NDArrayBase.template which contains records for all of the 
  asynNDArrayDriver parameters except those in NDFile.template.  Moved records from ADBase.template
  and NDPluginBase.template into this new file.  Made all template files "include" the files from the
  parent class, rather than calling dbLoadRecords for each template file.  This simplifies commonPlugins.cmd.
  A similar include mechanism was applied to the *_settings.req files, which simplifies commonPlugin_settings.req.
* Added a new record, $(P)$(R)ADCoreVersion_RBV, that is loaded for all drivers and plugins. 
  This record contains the ADCore version number. This can be used by Channel Access clients to alter their
  behavior depending on the version of ADCore that was used to build this driver or plugin.
  The record contains the string ADCORE_VERSION.ADCORE_REVISION.ADCORE_MODIFICATION, i.e. 2.2.0 for this release.

* Added the info tag "autosaveFields" to allow automatic creation of autosave files.
* ADBase.template
  - Added optional macro parameter RATE_SMOOTH to smooth the calculated array rate.
    The default value is 0.0, which does no smoothing.  Range 0.0-1.0, larger values 
    result in more smoothing.
  
### ImageJ Viewer
* Bug fixes from Lewis Muir.

### simDetector driver
* Created separate simDetector.h file so class can be exported to other applications.
      
### iocs/simDetectorNoIOC
* New application that demonstrates how to instantiate a simDetector driver
  and a number of plugins in a standalone C++ application, without running an EPICS IOC.
  If asyn and ADCore are built with the EPICS_LIBCOM_ONLY flag then this application only
  needs the libCom library from EPICS base and the asyn library.  It does not need any other
  libraries from EPICS base or synApps.

### Makefiles
* Added new build variable $(XML2_INCLUDE), which replaces hardcoded /usr/include/libxml2 in
  several Makefiles.  $(XML2_INCLUDE) is normally defined in 
  $(AREA_DETECTOR)/configure/CONFIG_SITE.local, typically to be /usr/include/libxml2.
* Changes to support the EPICS_LIBCOM_ONLY flag to build applications that depend only
  on libCom and asyn.
    
    
R2-1 (October 17, 2014)
=======================
### NDPluginFile
* Added new optional feature "LazyOpen" which, when enabled and in "Stream" mode, will defer 
  file creation until the first frame arrives in the plugin. This removes the need to initialise
  the plugin with a dummy frame before starting capture.  

### NDFileHDF5
* Added support for defining the layout of the HDF5 file groups, dataset and attributes in an XML
  definition file. This was a collaboration between DLS and APS: Ulrik Pedersen, Alan Greer, 
  Nicholas Schwarz, and Arthur Glowacki. See project pages: 
  [AreaDetector HDF5 XML Layout](http://confluence.diamond.ac.uk/x/epF-AQ)
  [HDF5 Writer Plugin](https://confluence.aps.anl.gov/x/d4GG)

### NDFileTiff
* All NDArray attributes are now written as TIFF ASCII file tags, up to a maximum of 490 tags.
  Thanks to Matt Pearson for this.

### NDPluginOverlay
* Added support for text overlays. Thanks to Keith Brister for this.
* Added support for line width in rectangle and cross overlays.  Thanks to Matt Pearson for this.
* Fixed problem with DrawMode=XOR. This stopped working back in 2010 when color support was added.

### NDPluginTransform
* Complete rewrite to greatly improve simplicity and efficiency.  It now supports 8 transformations
  including the null transformation.  Performance improved by a factor of 13 to 85 depending
  on the transformation.  Thanks to Chris Roehrig for this.

### Miscellaneous
* Added a new table to the 
  [top-level documentation] (http://cars.uchicago.edu/software/epics/areaDetector.html).
  This contains for each module, links to:
  - Github repository
  - Documentation
  - Release Notes
  - Directory containing pre-built binary files
* Added support for cygwin32 architecture.  This did not work in R2-0.


R2-0 (April 4, 2014)
====================
* Moved the repository to [Github](https://github.com/areaDetector/ADCore).
* Re-organized the directory structure to separate the driver library from the example 
  simDetector IOC application.
* Moved the pre-built libraries for Windows to the new ADBinaries repository.
* Removed pre-built libraries for Linux.  Support libraries such as HDF5 and GraphicsMagick 
  must now be present on the build system computer.
* Added support for dynamic builds on win32-x86 and windows-x64. 

### NDArray and asynNDArrayDriver
* Split NDArray.h and NDArray.cpp into separate files for each class: 
  NDArray, NDAttribute, NDAttributeList, and NDArrayPool.
* Changed all report() methods to have a FILE *fp argument so output can go to a file. 
* Added a new field, epicsTS, to the NDArray class. The existing timeStamp field is a double,
  which is convenient because it can be easily displayed and interpreted.  However, it cannot
  preserve all of the information in an epicsTimeStamp, which this new field does.
  This is the definition of the new epicsTS field.
```
epicsTimeStamp epicsTS;  /**< The epicsTimeStamp; this is set with
                           * pasynManager->updateTimeStamp(), 
                           * and can come from a user-defined timestamp source. */
```
* Added 2 new asynInt32 parameters to the asynNDArrayDriver class.
    - NDEpicsTSSec contains NDArray.epicsTS.secPastEpoch
    - NDEpicsTSNsec contains NDArray.epicsTS.nsec

* Added 2 new records to NDPluginBase.template.
    - $(P)$(R)EpicsTSSec_RBV contains the current NDArray.epicsTS.secPastEpoch
    - $(P)$(R)EpicsTSNsec_RBV contains the current NDArray.epicsTS.nsec

* The changes in R2-0 for enhanced timestamp support are described in 
[areaDetectorTimeStampSupport](http://cars.uchicago.edu/software/epics/areaDetectorTimeStampSupport.html).

### NDAttribute
* Added new attribute type, NDAttrSourceFunct. 
  This type of attribute gets its value from a user-defined C++ function. 
  It can thus be used to get any type of metadata. Previously only EPICS PVs 
  and driver/plugin parameters were available as metadata. 
* Added a new example file, ADSrc/myAttributeFunctions.cpp, that demonstrates 
  how to write user-defined attribute functions. 
* Made all contents of attribute XML files be case-sensitive. It was announced in R1-7 that 
  datatype and dbrtype strings would need to be upper-case. This is now enforced. 
  In addition attribute names are now case-sensitive. 
  Any mix of case is allowed, but the NDAttributeList::find() method is now case sensitive. 
  This was done because it was found that the epicsStrCaseCmp function was significantly 
  reducing performance with long attribute lists. 
* Removed the possibility to change anything except the datatype and value of an attribute once 
  it is created. The datatype can only be changed from NDAttrUndefined to one of the actual values.
* Added new setDataType() method, removed dataType from setValue() method.
* Added getName(), getDescription(), getSource(), getSourceInfo(), getDataType() methods.
* Fixed logic problem with PVAttribute.  Previously it could update the actual attribute value
  whenever a channel access callback arrived.  It now caches the callback value in private data and
  only copied it to the value field when updateValue() is called.
* Changed constructor to have 6 required paramters, added sourceType and pSource.

### NDPluginDriver 
* This is the base class from which all plugins derive. Added the following calls
  to the NDPluginDriver::processCallbacks() method:
    - setTimeStamp(&pArray->epicsTS);
    - setIntegerParam(NDEpicsTSSec, pArray->epicsTS.secPastEpoch);
    - setIntegerParam(NDEpicsTSNsec, pArray->epicsTS.nsec);

  It calls setTimeStamp with the epicsTS field from the NDArray that was passed to the
  plugin. setTimeStamp sets the internal timestamp in pasynManager. This is the timestamp
  that will be used for all callbacks to device support and read() operations in this
  plugin asynPortDriver. Thus, the record timestamps will come from the NDArray passed
  to the plugin if the record TSE field is -2. It also sets the new asynNDArrayDriver
  NDEpicsTSSec and NDEpicsTSNsec parameters to the fields from the NDArray.epicsTS.
  These records can then be used to monitor the EPICS timestamp in the NDArray even
  if TSE is not -2.

### NDPluginOverlay. 
* Fixed bug in the cross overlay that caused lines not to display if the cross was 
  clipped to the image boundaries. The problem was attempting to store a signed value in a size_t variable. 

### NDPluginROI. 
* Make 3-D [X, Y, 1] arrays be converted to 2-D even if they are not RGB3. 

###NDPluginStats. 
* Fixed bug if a dimension was 1; this bug was introduced when changing dimensions to size_t. 

### NDFileNetCDF. 
* Changes to work on vxWorks 6.8 and above.
* Writes 2 new variables to every netCDF file for each NDArray. 
  - epicsTSSec contains NDArray.epicsTS.secPastEpoch. 
  - epicsTSNsec contains NDArray.epicsTS.nsec. 
    
  Note that these variables are arrays of length numArrays,
  where numArrays is the number of NDArrays (images) in the file. It was not possible
  to write the timestamp as a single 64-bit value because the classic netCDF file
  format does not support 64-bit integers.

### NDFileTIFF. 
* Added 3 new TIFF tags to each TIFF file:</p>
  - Tag=65001, field name=NDUniqueId, field_type=TIFF_LONG, value=NDArray.uniqueId.
  - Tag=65002, field name=EPICSTSSec, field_type=TIFF_LONG, value=NDArray.epicsTS.secPastEpoch.
  - Tag=65003, field name=EPICSTSNsec, field_type=TIFF_LONG, value=NDArray.epicsTS.nsec.

  It was not possible to write the timestamp as a single 64-bit value because TIFF
  does not support 64-bit integer tags. It does have a type called TIFF_RATIONAL which
  is a pair of 32-bit integers. However, when reading such a tag what is returned
  is the quotient of the two numbers, which is not what is desired.

### NDPluginAttribute. 
* New plugin that allows trending and publishing an NDArray attribute over channel access.


R1-9-1 and earlier
==================
Release notes are part of the
[areaDetector Release Notes](http://cars.uchicago.edu/software/epics/areaDetectorReleaseNotes.html).
