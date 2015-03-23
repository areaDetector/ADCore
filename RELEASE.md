ADCore Releases
===============

The latest untagged master branch can be obtained at
https://github.com/areaDetector/ADCore.

Tagged source code and pre-built binary releases prior to R2-0 are included
in the areaDetector releases available via links at
http://cars.uchicago.edu/software/epics/areaDetector.html.

Tagged source code releases from R2-0 onward can be obtained at 
https://github.com/areaDetector/ADCore/releases.

Tagged prebuilt binaries from R2-0 onward can be obtained at
http://cars.uchicago.edu/software/pub/ADCore.

The versions of EPICS base, asyn, and other synApps modules used for each release can be obtained from 
the EXAMPLE_RELEASE_PATHS.local, EXAMPLE_RELEASE_LIBS.local, and EXAMPLE_RELEASE_PRODS.local
files respectively, in the configure/ directory of the appropriate release of the 
[top-level areaDetector](https://github.com/areaDetector/areaDetector) repository.


Release Notes
=============

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
