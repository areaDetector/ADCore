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

R2-1 (July XXX 2014)
--------------------
* NDPluginOverlay
    - Added support for text overlays. Thanks to Keith Brister for this.
    - Fixed problem with DrawMode=XOR. This stopped working back in 2010 when color support was added.
* NDPluginTransform
    - Complete rewrite to greatly improve simplicity and efficiency.  It now supports 8 transformations
      including the null transformation.  Performance improved 10-100 times.  Thanks to Chris Roehrig
      for this.
* NDPluginFile
    - Added support for an NDArray attribute "FilePluginWriteFile".  If this exists and is 0 then
      the NDArray will not be written to the file.
    - Added new optional feature "LazyOpen" which, when enabled and in "Stream" mode, will defer 
      file creation until the first frame arrives in the plugin. This removes the need to initialise
      the plugin with a dummy frame before starting capture.  
* NDFileTiff
    - All NDArray attributes are now written as TIFF ASCII file tags, up to a maximum of 490 tags.
      Thanks to Matt Pearson for this.
* Added support for cygwin32 architecture.  This did not work in R2-0.  NOT YET WORKING.
* NDFileHDF5
    - Added support for defining the layout of the HDF5 file groups, dataset and attributes in an XML
      definition file.

R2-0
----
* Moved the repository to [Github](https://github.com/areaDetector/ADCore).
* Re-organized the directory structure to separate the driver library from the example 
  simDetector IOC application.
* Moved the pre-built libraries for Windows to the new ADBinaries repository.
* Removed pre-built libraries for Linux.  Support libraries such as HDF5 and GraphicsMagick 
  must now be present on the build system computer.
* Added support for dynamic builds on win32-x86 and windows-x64. 

###NDArray and asynNDArrayDriver
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

###NDAttribute
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

###Plugins
* NDPluginDriver (the base class from which all plugins derive) added the following calls
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
* NDPluginOverlay. Fixed bug in the cross overlay that caused lines not to display if the cross was 
  clipped to the image boundaries. The problem was attempting to store a signed value in a size_t variable. 
* NDPluginROI. Make 3-D [X, Y, 1] arrays be converted to 2-D even if they are not RGB3. 
* NDPluginStats. Fixed bug if a dimension was 1; this bug was introduced when changing dimensions to size_t. 
* NDFileNetCDF. 
    - Changes to work on vxWorks 6.8 and above.
    - Writes 2 new variables to every netCDF file for each NDArray. 
        - epicsTSSec contains NDArray.epicsTS.secPastEpoch. 
        - epicsTSNsec contains NDArray.epicsTS.nsec. 
    
  Note that these variables are arrays of length numArrays,
  where numArrays is the number of NDArrays (images) in the file. It was not possible
  to write the timestamp as a single 64-bit value because the classic netCDF file
  format does not support 64-bit integers.
* NDFileTIFF. Added 3 new TIFF tags to each TIFF file:</p>
    - Tag=65001, field name=NDUniqueId, field_type=TIFF_LONG, value=NDArray.uniqueId.
    - Tag=65002, field name=EPICSTSSec, field_type=TIFF_LONG, value=NDArray.epicsTS.secPastEpoch.
    - Tag=65003, field name=EPICSTSNsec, field_type=TIFF_LONG, value=NDArray.epicsTS.nsec.

  It was not possible to write the timestamp as a single 64-bit value because TIFF
  does not support 64-bit integer tags. It does have a type called TIFF_RATIONAL which
  is a pair of 32-bit integers. However, when reading such a tag what is returned
  is the quotient of the two numbers, which is not what is desired.
* NDPluginAttribute. New plugin that allows trending and publishing an NDArray attribute over channel access.


R1-9-1 and earlier
------------------
Release notes are part of the
[areaDetector Release Notes](http://cars.uchicago.edu/software/epics/areaDetectorReleaseNotes.html).
