ADCore Releases
===============

Introduction
------------

The latest untagged master branch can be obtained at
https://github.com/areaDetector/ADCore.

Tagged source code and pre-built binary releases prior to R2-0 are included
in the areaDetector releases available via links at
https://areadetector.github.io/areaDetector/legacy_versions.html.

Tagged source code releases from R2-0 onward can be obtained at
https://github.com/areaDetector/ADCore/releases.

The versions of EPICS base, asyn, and other synApps modules used for each release can be obtained from
the EXAMPLE_RELEASE_PATHS.local, EXAMPLE_RELEASE_LIBS.local, and EXAMPLE_RELEASE_PRODS.local
files respectively, in the configure/ directory of the appropriate release of the
[top-level areaDetector](https://github.com/areaDetector/areaDetector) repository.

## __R3-15 (July XXX 1, 2025)__

### Fixes for Github Actions
  * Fixed problem with TIRPC in asyn.
  * Updated from windows-2019 to windows-2022.

### NDPluginCodec
  * Fixed memory leaks in the JPEG compressor and decompressor. Thanks to Evan Daykin for this.

### ADTop.adl
  * Added ADHamammatsuDCAM and BlackflyS PGE 23S6C.
  * Reformatted into 2 columns.

## __R3-14 (December 1, 2024)__

### asynNDArrayDriver and NDPluginBase.template
  * Added support for pre-allocating NDArrays.
    This is useful for preventing dropped frames on high-speed cameras because of the finite
    time required for the operating system to allocate memory.
    If a camera is acquiring at high speed and NDArrays are allocated on demand to fill
    plugin buffers during acquisition then the driver can drop frames during an initial acquisition.
    By pre-allocating frames into the NDArrayPool free list this problem can be avoided.
    There are 2 new records in NDArrayBase.template.
    
    - NumPreAllocBuffers  The number of buffer to pre-allocate.
    - PreAllocBuffers  Processing this record does the pre-allocation.
    
    The pre-allocation operation can be time-consuming if a large amount of memory (e.g. 128 GB)
    is being allocated.
    The PoolUsedBuffers, PoolAllocBuffers and PoolUsedMem records are updated during
    this process so the progress is visible on the detector OPI screen in the Buffers sub-screen.
    PreAllocBuffers is a busy record so clients can determine when the process is complete
    by using ca_put_callback.
    This feature requires the following:

      - The driver must have collected at least one frame using the current dimensions
        and data type.
      - The driver must store the most recent NDArray in this->pArrays[0].
        Most drivers already do this, but some will need to be updated to implement this.
    
    If either of these conditions are not met then there will be an error message saying
    "ERROR, must collect an array to get dimensions first".
  * Changed how the NDArrayPool statistics records (PoolAllocBuffers, etc.) are scanned.
    They were previously only periodically scanned, with PoolUsedMem scanning periodically 
    and the other records processing via FLNK fields.
    This has changed so that a new record, PoolPollStats is periodically scanned,
    and the other records are I/O Intr scanned.
    This allows these records to be updated during the pre-allocation operation described above.
    PoolPollStats causes callbacks for the I/O Intr scanned records in asynNDArrayDriver.

### NDPluginFile
  * Improved the feedback on progress when saving a file in Capture mode.
    Previously the only indication that the file saving was in progress was the
    status of the WriteFile_RBV PV.  There was no indication of how many arrays
    remained to be written to the file, or how many arrays/second were being written.
    This was changed to add the following progress indicators:

      - ExecutionTime_RBV is updated with the time to write each array.
      - ArrayCounter_RBV is incremented as each array is written.
      - ArrayRate_RBV is updated, so the number of frames/s being written is visible.
      - NumCaptured_RBV counts down from NumCaptured to 0, so the number
        of remaining frames is visible.

### NDFileTIFF
  * Fixed a bug introduced in R3-8.  
    Integer attributes smaller than Int64 might not be correctly written to the file.

### NDPluginROIStat
  * Added the ability to clear the time series data without starting a new acquisition.

### myAttributeFunctions.cpp
  * Added code to create NDAttributes of all numeric data types.
  * These were used to test the fix to NDFileTIFF, and can be generally useful.

### Documentation
  * Removed the documentation/ directory, which was obsolete.

## __R3-13 (February 9, 2024)__

### NDArrayPool
  * Provide a mechanism to override the default memory allocator for NDArrays.
    Thanks to Emilio PJ on Github for this.

### asynNDArrayDriver
  * Added UpdateTimeStamps method.
  * Added NDFileFreeCapture parameter to manually free the capture buffer.
  * Allow the filename in readNDAttributes file to be an XML string rather than a file name.
    This allows Channel Access clients to write the XML directly. Thanks to Marcell Nagy for this.

### NDAttribute
  * Added support for attribute type CONST, where the value is the "source" value specified in the XML file.
    Thanks to Keenan Lang for this.

### NDPluginFile
  * Change capture buffer to use std::vector which is simpler and cleaner.
  * Free capture buffer when capture or streaming start to prevent memory leak.
  * Added FreeCapture record to manually free the capture buffer.
  * Include the interface and interrupt masks passed to the constructor in the masks passed to NDPluginDriver.

### NDFileNexus
  * Set the plugin type.

### NDPluginBadPixel
  * Fix missing argument to constructor from configuration command.

### NDPluginProcess
  * Improved the logic for high and low clipping so that both the threshold
    and the replacement value can be independently specified.
    Previously these were always the same value.
    - LowClip has been renamed to LowClipThresh.
    - HighClip has been renamed to HighClipThresh.
    - LowClipValue and HighClipValue have been added.
    - **This change is not backwards compatible, the value of Low/HighClipValue must now be specified.**

### commonDriverMakefile, commonLibraryMakefile
  * Fix error in upper/lower case of NeXus_DIR.
  * Fix error with nanohttp_stream library.

### Continuous integration
  * Added Github Actions builds.  Thanks to Ralph Lange and Michael Davidsaver for this.

### validateXML.sh
  * Fix location of iocimDetector.

### EXAMPLE_commonPlugins.cmd
  * Load NDPluginPva by default.
  * Fix path to support module .template and .req files to be (MODULE)/db rather than (MODULE)/moduleApp/Db. 

## __R3-12-1 (January 22, 2022)__

### ADCoreVersion.h
  * Changed to 3.12.1.  R3-12 neglected to change this, so it was still 3.11.

### test_NDFileHDF5AttributeDataset.cpp
  * Fixed this test for the changes in NDFileHDF5AttributeDataset.cpp made in R3-12.

### Documentation file NDFileHDF5.rst
 * Updated for the changes in NDFileHDF5AttributeDataset.cpp made in R3-12.

## __R3-12 (January 20, 2022)__

### ADAutoSaveMenu.req
  * New file to be used with the configMenu utility in autosave.
    It saves all of the camera and plugin settings defined in iocBoot/iocXXX/auto_settings.req.
    This allows manually saving and loading different configurations for different setups with the same IOC.
    It also allows saving and then loading the settings when a camera is reset or restarted,
    without restarting the IOC.

### ADPlugins OPI sub-screen.
  * Added a related display "Plugins/Other #2/AS configMenu" which allows manually saving and loading
    configurations that use ADAutoSaveMenu.req described above.

### ntndArrayConverter
  * Added conversion from Posix time to EPICS time when copying NTNDArray to NDArray.

### EXAMPLE_commonPlugins.cmd
  * Load configMenu.db.
  * Fixes to set_request_file_path to use the /db directory rather than the App/Db directory.

### commonDriverMakefile
  * Added optional reccaster support module.

### CCDMultiTrack.cpp
  * Corrected index error.

### NDFileHDF5AttributeDataset.cpp
  * Removed unnecessary creation of 1x1 dimensions.

## __R3-11 (May 26, 2021)__

### NDPluginBadPixel
  * New plugin to fix bad pixels in an NDArray.  The bad pixels are defined
    in a JSON text file.  Each bad pixel can be replaced with a fixed value,
    replaced with the value of a nearby single pixel, or replaced by the
    median value of nearby pixels.
    The documentation is [here](https://areadetector.github.io/areaDetector/ADCore/NDPluginBadPixel.html).

### NDFileHDF
  * Fixed a problem with writing constant string datasets and HDF5 string attributes.
    The call to `H5Tset_size()` is suppposed to include the trailing nil character
    in the `size` argument, but it did not.  This led to HDF5 files that did not have the
    correct contents, though we never had reports of problems reading them.
    It also led to an HDF5 library error message if a string of length 0 was passed.
    Added 1 to the `size` argument.

### commonLibraryMakefile, commonDriverMakefile
  * Build with new library nanohttp_stream from ADSupport if WITH_GRAPHICSMAGICK=YES.

## __R3-10 (September 20, 2020)__

### Many files
  * Changed the code and the Makefiles to avoid using shareLib.h from EPICS base.
    Added new header files ADCoreAPI.h and NDPluginAPI.h that are used to control whether
    functions, classes, and variables are defined internally to the libraries or externally. 
    This is the mechanism now used in EPICS base 7.
    It makes it much easier to avoid mistakes in the order of include files that cause external 
    functions to be accidentally exported in the DLL or shareable library. 
    This should work on all versions of base, and have no impact on user code.
  * Fixed many minor problems that caused compiler warnings.

### ADDriver
  * Added new bool member variable deviceIsReachable and new method connect().
    If drivers detect that the underlying hardware is unreachable they should
    set deviceIsReachable=false and call this->disconnect().  That will cause
    asynManager to flag the driver as disconnected, and prevent queuing any
    read or write requests.  It will also cause the associated asyn record .CNCT
    field to be "Disconnected" which displays prominently in red letters on all
    driver OPI screens.

### NDFileHDF
  * Fixed a problem loading XML layout files with invalid XML syntax.
    Previously it would crash the IOC with no error message.
    Now it prints an error message, sets the error status PVs in the OPI screen,
    and does not crash.
  * Fixed a problem that constant datasets were not closed before closing the file.
    This produced warning messages, and probably minor memory leaks.
  * Changed free() calls for XML variables to xmlGetGlobalState()->xmlFree().
    This fixes issues when mixing static and dynamically built libraries.
    Thanks to Ben Bradnick for this.
    
### NDFileMagick
  * Fixed a problem that GraphicsMagick was being initialized too late, resulting
    in an assertion failure with newer versions of GraphicsMagick than the one
    in ADSupport.
    Thanks to Michael Davidsaver for this.

### NDPluginCircularBuff
  * Added extra checks when pre-count is set
    - A negative value resulted in a segfault.
    - Changing it while acquiring doesn't make sense, as it is used
      to allocate the ring buffer when starting an acquisition

### NDPluginStats
  * Added new CursorVal record that contains the current value of the pixel at the cursor position.
    Thanks to Ray Gregory from ORNL for this.

### NDPluginPva
  * Removed unneeded NTNDArrayRecord::destroy() method which will cause an error in a future release of base.

### docs/ADCore/NDFileHDF5.rst, XML_schema/hdf5
  * Fixed the XML schema and the documentation for the use of the "when" attribute in HDF5 layout XML files.
    Previously it stated that "when" could only be used for HDF5 attributes, not datasets.
    This is incorrect, it can also be used for datasets.  It also incorectly stated that the allowed
    values were "OnFileOpen", "OnFileClose", and "OnFileWrite".  "OnFileWrite" is a valid
    value for HDF5 datasets, but it is not an allowed value for HDF5 attributes.

### NDStats.adl
  * Fixed typo in arguments for centroidX timeseries plot related display.

## __R3-9 (February 24, 2020)__

### CCDMultiTrack
  * New CCDMultiTrack class and database to support spectroscopy detectors
    that support multi-track readout (Peter Heesterman)

### NDPluginStats
  * Fixed an off-by-one bug in the histogram calculation.
    It was incorrectly reporting the number of elements above HistMax and the mid-point element of
    the histogram had 0 counts when it should not.

### NDFileHDF5
  * Fixed problem with XML2 library linking warnings on static Windows builds (Peter Heesterman)

### NDPluginStdArrays
  * Changes to support compressed NDArrays. This will really only work properly when the ArrayData waveform record
    FTVL is CHAR, since compressed data is just a stream of bytes.

### NDFileNetCDF
  * Changes to handle NDArrays and NDAttributes with data types epicsInt64 and epicsUInt64.  The netCDF classic file
    format does not handle 64-bit integer data.  The workaround is to cast the data to float64, so the netCDF library
    thinks it is writing float64 data, and marks the netCDF data in in the file as type `double`.
    However the netCDF global attribute "datatype" (enum NDDataType_t) will correctly indicate the actual datatype.
    File readers will need to cast the data to the actual datatype after reading the data with the netCDF library functions.
  * Incremented the global attribute NDNetCDFFileVersion from 3.0 to 3.1 because the order of the NDDataType_t enums
    changed in ADCore R3-8 to insert NDInt64 and NDUInt64 after NDUInt32.  This changed the enum values of NDFloat32
    and NDFloat64.  File readers using the value of these enums thus need to know which version of ADCore the file
    was written with.  This change should have been made in R3-8.

### SCANRATE macro
  * A new macro has been added to NDPosPlugin.template, NDPluginBase.template, NDFile.tamplate, NDArrayBase.template and
    ADBase.template. This can be used to control the SCAN value of status PVs which update on every frame such as
    ArrayCounter_RBV, TimeStamp_RBV, UniqueId_RBV among others. These were found to be intensive on the CPU when set to
    I/O Intr for a detector running at faster than 1 KHz. The performance could be improved by setting the SCAN value
    to update less frequently.
  * The default value of the macro has been set to I/O Intr so that it will not affect any applications that do not
    require the SCAN rate throttled.
  * The ImageJ EPICS_AD_Viewer plugin monitors ArrayCounter_RBV to decide when there is a new image to display. That
    means that it will not display faster than the SCANRATE you select.
  * By making the records periodically scanned they will be reading even when the detector is stopped, which is a bit
    more overhead than SCAN=I/O Intr.

### docs
  * Replaced all raw HTML tables in .rst files with Sphinx flat-tables.

### iocBoot/EXAMPLE_commonPlugins.cmd
  * Added command `callbackSetQueueSize(5000)` to avoid `callbackRequest: cbLow ring buffer full` errors.

### Database files ADBase.template, NDPluginBase.template, and ADPrefixes.template
  * Fixed syntax errors. They were missing quotes around the macro parameters $(PORT) and $(NELEMENTS).
    This caused parsing errors if the macros contained special characters like colon (:).

### OPI files
  * Added .bob files for the Phoebus Display Builder.  These are autoconverted from the .adl files.

## __R3-8 (October 20, 2019)__

Note: This release requires asyn R4-37 because it uses new asynInt64 support.

### 64-bit integer support
  * NDArray now supports NDInt64 and NDUInt64 data types
  * NDAttribute now supports NDAttrInt64 and NDAttrUInt64 data types
  * All standard plugins were modified to handle the new 64-bit NDArray and NDAttribute data types
  * The following file plugins can now write NDArray data types NDInt64 and NDUInt64, and NDAttribute data types
    NDAttrInt64 and NDAttrUInt64:
    * NDFileHDF
    * NDFileNexus
    * NDFileTIFF
  * NDFileNetCDF cannot write the 64-bit integer data types because the netCDF3 "Classic" data model does not support them.
  * 64-bit integer TIFF files cannot be read by ImageJ, but they can be read by IDL.
  * ntndArrayConverter converts NDInt64 and NDUInt64 to the equivalent pvData "long" and "ulong".

### asynNDArrayDriver, ADDriver
  * Moved the following parameters from ADDriver to asynNDArrayDriver.
    This allows them to be used with other drivers that derive from asynNDArrayDriver:
    * ADAcquire
    * ADAcquireBusy
    * ADWaitForPlugins
    * ADManufacturer
    * ADModel
    * ADSerialNumber
    * ADFirmwareVersion
    * ADSDKVersion

    These changes should be transparent to derived classes.

### Makefiles
  * Changes to avoid compiler warnings.

### myAttributeFunctions.cpp
  * Added new TIME64 parameter and functTime64 function.
    This creates an attribute of type NDAttrUInt64 that contains the current EPICS time with secPastEpoch in the
    high-order 32-bits and nsec in the low order 32-bits.  This allows testing 64-bit attribute handling.

### ADTop.[adl, .ui, .opi, .edl]
  * Added additional ADSpinnaker and ADVimba cameras

## __R3-7 (August 8, 2019)__

Note: This release requires asyn R4-36 because it uses new features of asynPortDriver.

### asynNDArrayDriver, ADDriver, NDPluginDriver
  * Changed to use the new method asynPortDriver::parseAsynUser()
  * Use the asyn addr value in the writeXXX and readXXX methods. Previously they were not using it.
### docs/ADCore
  * Began conversion of raw HTML tables to Sphinx list-table or flat-table in all .rst files.
### ntndArrayConverter
  * Fixed bugs that could cause a crash when attributes were added to the NDArrays.
### NDFileTIFF
  * Fixed stack overflow problem on vxWorks

## __R3-6 (May 29, 2019)__

### NDFileHDF5
  * Fixed issues with chunking.
    * User-defined chunking only supported 2-D arrays, plus the dimension for multiple arrays.
      This is insufficient, we need to be able to support N-dimensional arrays in a general manner.
    * Direct chunk write only worked with 1-D or 2-D arrays. This prevented it from working with RGB images, for example.
    * New records have been added for NDArray dimensions 2-9 (ChunkSize2, ChunkSize3, ... ChunkSize9).
    * The labels on the OPI screen for NumColsChunks and NumRowsChunks changes "Dim0 chunk size" and "Dim1 chunk size",
      added "Dim2 chunk size" to this screen as well.
    * Added a new NDFileHDF5_ChunkingFull screen that shows all 12 chunking related records (10 dimensions, NumFramesChunks, ChunkSizeAuto).
    * Previously the documentation said that if nColChunks and nRowChunks were configured by the user to the special value 0,
      they will default to the dimensions of the incoming frames. However, this was not really true, setting to 0 only worked once.
      If they were set to 0 and a 512x512 array was saved then the chunking was set to 512x512.
      If the user then disabled binning and saved a 1024x1024 array the chunking remained at 512x512, which was not desirable.
    * Added new ChunkSizeAuto bo record to allow always setting the chunking to automatic or manual.
      If ChunkSizeAuto=Yes then the chunking in each dimension is automatically set to that dimension of the NDArray, so
      the chunking is one complete NDArray. The automatic control does not affect NumFramesChunks.
  * Fixed a problem in NDFiileHDF5::calcNumFrames(), it was setting NumCapture to 1 if numExtraDims=0, which is incorrect.
    This caused the autosaved value to be replaced by 1 each time the IOC restarted.
  * Added support for JPEG compression.  There is a new JPEGQuality record (1-100) to control the compression quality.
    This requires ADSupport R1-8 which adds HDF5 JPEG codec filter and plugin.
    JPEG compression can be done in the NDFileHDF5 plugin itself, or in NDPluginCodec writing pre-compressed NDArrays with
    HDF5 direct chunk write.

### NDFileTIFF
  * Increased the last used defined tag (TIFFTAG_LAST_ATTRIBUTE) from 65500 to 65535,
    which is the maximum allowed by TIFF specification.
  * The last usable tag is now TIFFTAG_LAST_ATTRIBUTE, while previously it was TIFFTAG_LAST_ATTRIBUTE-1.
  * Fixed bug in memory allocation for user-defined tags that could lead to access violation.

### NDPluginStats
  * Removed calls to processCallbacks() when CursorX, CursorY, or CentroidThreshold are changed.
    This is no longer thread safe.
    To see updates when plugin is not receiving NDArrays change the PV and then press ProcessPlugin.

### NDPluginColorConvert
  * Added support for converting Bayer color to RGB.  Thanks to Arvinder Palaha for this.

### commonDriverMakefile
  * Added ADPluginBar and ADCompVision plugins if defined.  ADCompVision requires WITH_BOOST=YES.

### EXAMPLE_commonPlugins.cmd, EXAMPLE_commonPlugin_settings.req
  * Added commented out lines for ADCompVision and ADPluginBar.

## __R3-5 (April 12, 2019)__

### Documentation
  * Converted documentation from raw HTML documentation to .rst files using Sphinx.
  * Added a new docs/ directory which replaces the old documentation directory.
  * The old .html files were converted to .rst using [pandoc](https://pandoc.org).
    Most documentation includes tables that describe the plugin parameters and
    record names.  These tables do not convert well to .rst so they have been left as native
    html in the files.
  * The new documentation is hosted at [areaDetector.github.io](https://areaDetector.github.io).
  * Many thanks to Stuart Wilkins for this major effort.

### ADDriver, asynNDArrayDriver
* Fixed a race condition in R3-4 with NumQueuedArrays and AcquireBusy.
  This caused AcquireBusy to sometimes go to Done before the plugins were done processing.
  A typical symptom was for successive points in a scan to have the same values from the statistics plugin,
  because the detector did not wait for the plugin to post a new value before it said it was done.
  The logic for this was moved from the database (ADBase.template) to the driver (ADDriver.cpp).

### NDArray
* Changed the codec member from a std::string to a structure that contains not just the name of the codec
  but also the compression level, shuffle parameter, etc.

### NDPluginCodec
* Added support for the lz4 and bitshuffle/lz4 codecs.
  These are the compressors that the Eiger uses, so compressed NDArrays from ADEiger can
  now be decompressed by NDPluginCodec.
  The ImageJ pvAccess plugin in ADViewers now also supports decompressing lz4 and bitshuffle/lz4.
  These codecs are independent of Blosc, which also supports lz4 and bitshuffle but with some differences.
* Fixed a problem with NDPluginCodec.template for the Blosc codec.
  The Bit and Byte shuffle values in the BloscShuffle records were swapped so when Bit shuffle was selected
  it was actually doing Byte shuffle and vice versa.

### NDPluginAttribute
* Changed the time series support to use NDPluginTimeSeries.  This is very similar to the change that
  was made in R3-3 for NDPluginStats.
  This significantly reduced the code size, while adding the capability of running in Circular Buffer mode,
  not just a fixed number of time points.
  Thanks to Hinko Kocevar for this.
* EXAMPLE_commonPlugins.cmd has changed to load an NDPluginTimeSeries plugin and database for the NDPluginAttribute plugin,
  so the local commonPlugins.cmd file must be updated.

### commonDriverMakefile
* Added support for bitshuffle.  This was added in ADSupport R1-7.
  To use it set WITH_BITSHUFFLE=YES in areaDetector/configure/CONFIG_SITE.local.

### NDFileHDF5
* Added support for Direct Chunk Write.  This allows the plugin to directly write compressed NDArrays from
  NDPluginCodec or from the detector driver (e.g. ADEiger).
  This can significantly improve performance by bypassing much if the code in the HDF5 library.
* Added support for bitshuffle/lz4 compression, which is independent of Blosc.
* Added support for lz4 compression, which is independent of bitshuffle and Blosc.
* Added FlushNow record to force flushing the datasets to disk in SWMR mode.
* Changed the enum strings for compression (Blosc, LZ4, BSLZ4) to be compatible with NDPluginCodec.
  This may break backwards compatibilty with clients that were settings these using the enum string.
* Fixed many memory leaks.  The most serious ones were causing ~100kB leak per HDF5 file, which was significant when
  saving many small files.
* Changes to allow it to run correctly with the unit tests on Windows and Linux.

### NDFileHDF5.template
* Previously the size of the XMLFileName waveform record was set to 1048576.
  This only needs to be large if using it to transmit the actual XML content, which is not typical.
  Changed the size to be controlled by the macro XMLSIZE with a default of 2048.

### asynNDArrayDriver, NDPluginDriver, NDArrayBase.template, NDPluginBaseFull.adl
* Added support for new parameters NDCodec and NDCompressedSize and new records Codec_RBV and CompressedSize_RBV.
  These contain the values of NDArray.codec and NDArray.compressedSize.

### NDPluginBase.template
* Removed DTYP from the MaxArrayRate_RBV calc record which does not support DTYP.

### pluginTests
* Changes to allow unit tests to run on Windows if boost is available.


## __R3-4 (December 3, 2018)__

### ADSrc/asynNDArrayDriver.h, asynNDArrayDriver.cpp
* Fixed a serious problem caused by failure to lock the correct mutex when plugins called
  incrementQueuedArrayCount() and decrementQueuedArrayCount().
  This caused the Acquire and Acquire_RBV PVs to occasionally get stuck in the 1 (Acquire) state when acquistion
  was complete.  It might have also caused other problems that were not reported.
  This problem was introduced in R3-3.
* Fixed a race condition in the asynNDArrayDriver destructor.
  This was causing occasional failures in the Travis unit tests.

### NDPluginCodec
* New plugin written by Bruno Martins to support compressing and decompressing NDArrays.
* Compressors currently supported are JPEG (lossy) and Blosc (lossless).
* NDArray has a new .codec field that is the string name for the compression in use.
  It is empty() for no compression.
  It also has a new .compressedSize field that stores the compressed size in bytes.
  This may be less than .dataSize, which is the actual allocated size of .pData.
* The converters between NDArrays and NTNDArrays support this new codec field.
* Currently the main use case will be to transport compressed NTNDArrays using pvAccess.
* The ImageJ pvAccess viewer now supports decompression of all of the compressors supported by this plugin.
  This can greatly reduce network bandwidth usage when the IOC and viewer are on different machines.
* We also plan to enhance the HDF5 file plugin to support writing NDArrays that are already compressed,
  using the Direct Chunk Write feature. This should should improve performance.

### NDPluginDriver, NDPluginPva, NDPluginStdArrays
* Added new base class parameter and record MaxByteRate.
  This allows control of the maximum data output rate in bytes/s.
  If the output rate would exceed this then the output array is dropped and DroppedOutputArrays is incremented.
  This can be useful, for example, to limit the network bandwidth from a plugin.
  * For most plugins this logic is implemented in NDPluginDriver::endProcessCallbacks() when the plugin
    is finishing its operation and is doing callbacks to any downstream plugins.
    However, the NDPluginPva and NDPluginStdArrays plugins are treated differently because the
    output we generally want to throttle is not the NDArray passed to downstream plugins,
    but rather the size of the output for the pvaServer (NDPluginPva) or the size of
    the arrays passed back to device support for waveform records (NDPluginStdArrays).
  * For these plugins the throttling logic is thus also implemented inside the plugin.
    If these plugins are throttled then they really do no useful work, and so ArrayCounter
    is not incremented. This makes the ArrayRate reflect the rate at which the plugin
    is actually doing useful work.
    For NDPluginStdArrays this is also important because clients (e.g. ImageJ) may monitor
    the ArrayCounter_RBV field to decide when to read the array and update the display.
* Added new MaxArrayRate and MaxArrayRate_RBV records.
  These are implemented in the database with calc records.
  They write and read from MinCallbackTime but provide units of arrays/sec rather than sec/array.
* Optimization improvement when output arrays are sorted.
  Previously it always put the array in the sort queue, even if the order of this array was OK.
  That introduced an unneeded latency because the sort task only runs periodically.
  It caused ImageJ update rates to be slow, because the PVA output then comes in bursts,
  and some arrays are dropped either in the pvAccess server or client (not sure which).
  Now if the array is in the correct order it is output immediately.

### NDPluginCircularBuff
* Added new FlushOnSoftTrg record that controls whether the pre-buffer is flushed OnNewArray (previous behavior, default),
  or Immediately when a software trigger is received.  Thanks to Slava Isaev for this.

### NDFileTIFF
* Allow saving NDArrays with a single dimension.

### NDPluginStats
* Set NDArray uniqueId, timeStamp, and epicsTS fields for output time series NDArrays.

### OPI files
* ADTop.adl
  * Added ADVimba and GenICam
* NDStatsTimeSeriesBasicAll.adl, NDStatsTimeSeriesCentroidAll.adl, NDStatsTimeSeriesPlot.adl
  * Changed X axis from point number to time.
* NDPluginBase.adl
  * Fixed text widget type to string

### EXAMPLE_commonPlugins.cmd
* Added optional lines for ffmpegServer (commented out).


## __R3-3-2 (July 9, 2018)__

### ADApp/commonDriverMakefile
* Changed so that qsrv dbd and lib files are only included if WITH_QSRV=YES.
  Previously they were included if WITH_PVA=YES.  However base 3.14.12 supports
  WITH_PVA but does not support qsrv.  This allows WITH_PVA=YES to be used on 3.14.12
  as long as WITH_QSRV=NO.


## __R3-3-1 (July 1, 2018)__

### ADApp/commonDriverMakefile
* Added qsrv dbd and lib files so that areaDetector IOCs can serve normal EPICS PVs using pvAccess.
  Thanks to Pete Jemian for this.

### ADApp/ADSrc
* Changes in include statements in several files to eliminate warning when building dynamically with
  Visual Studio.

### ADApp/ADSrc/Makefile, ADApp/pluginSrc/Makefile, ADApp/pluginTests/Makefile
* Changed USR_INCLUDES definitions for all user-defined include directories,
  (for example XML_INCLUDE) from this:
  <pre>
  USR_INCLUDES += -I$(XML2_INCLUDE)
  </pre>
  to this:
  <pre>
  USR_INCLUDES += $(addprefix -I, $(XML2_INCLUDE))
  </pre>
  This allows XML2_INCLUDE to contain multiple directory paths.

  Note that these user-defined include directories must __not__ contain the -I in their definitions.
  Prior to areaDetector R3-3-1 the areaDetector/configure/EXAMPLE_CONFIG_SITE.local* files incorrectly had
  the -I flags in them, and these would not work correctly with the Makefiles in this release
  (or prior releases) of ADCore or other repositories.


## __R3-3 (June 27, 2018)__

### NDArrayPool design changes
* Previously each plugin used its own NDArrayPool. This design had the problem that it was not really possible
  to enforce the maxMemory limits for the driver and plugin chain.  It is the sum of the memory use by the driver
  and all plugins that matters, not the use by each individual driver and plugin.
* The NDPluginDriver base class was changed to set its pNDArrayPool pointer to the address passed to it in the
  NDArray.pNDArrayPool for the NDArray in the callback.  Ultimately all NDArrays are derived from the driver,
  either directly, or via the NDArrayPool.copy() or NDArrayPool.convert() methods.  This means that plugins
  now allocate NDArrays from the driver's NDArrayPool, not their own.  Any NDArrays allocated before the first
  callback still use the plugin's private NDArrayPool, but only a few plugins do this, and these only allocate
  a single NDArray so they don't use much memory.
* This means that the maxMemory argument to the driver constuctor now controls
  the total amount of memory that can be allocated for the driver and all downstream plugins.
* The maxBuffers argument to all driver and plugin constructors is now ignored.
  There is now no limit on the number of NDArrays, only on the total amount of memory.
* The maxBuffers argument to the ADDriver and NDPluginDriver base class constructors are still present so existing drivers
  and plugins will work with no changes. This argument is simply ignored.
  A second constructor will be added to each base class in the future and the old one will be deprecated.
* The maxMemory argument to the NDPluginDriver constructor is only used for NDArrays allocated before the
  first callback, so it can safely be set to 0 (unlimited).
* The freelist in NDArrayPool was changed from being an EPICS ellList to an std::multiset.  The freelist is
  now sorted by the size of the NDArray.  This allows quickly finding an NDArray of the correct size,
  and knowing if no such NDArray exists.
* Previously there was no way to free the memory in the freelist, giving the memory back to the operating system
  after a large number of NDArrays had been allocated, without restarting the IOC.  The NDArrayPool class now
  has an emptyFreeList() method that deletes all of the NDArrays in the freelist.  asynNDArrayDriver has a
  new NDPoolEmptyFreeList parameter, and NDArrayBase.template has a new bo record called $(P)$(R)EmptyFreeList
  that will empty the freelist when processed.  Note that on Linux the freed memory may not actually be returned
  to the operating system.  On Centos7 (and presumably many other versions of Linux) setting the value of the
  environment variable MALLOC_TRIM_THRESHOLD_ to a small value will allow the memory to actually be returned
  to the operating system.
* Improved the efficiency of memory allocation.  Previously the first NDArray that is large enough was returned.
  Now if the size of the smallest available NDArray exceeds the requested size by a factor of 1.5 then the
  memory in that NDArray is freed and reallocated to be the requested size.  Thanks to Michael Huth for the first
  implementation of this.
* These changes are generally backwards compatible. However, startup scripts that set a non-zero value for
  maxMemory in the driver may need to increase this value because all NDArrays are now allocated from this NDArrayPool.

### Queued array counting and waiting for plugins to complete
* Previously if one wanted to wait for plugins to complete before the driver indicated that acquisition was complete
  then one needed to set CallbacksBlock=Yes for each plugin in the chain.
  Waiting for plugins is needed in cases like the following, for example:
  - One is doing a step scan and one of the counters for the step-scan is a PV from the statistics plugin. It is necessary to
    wait for the statistics plugin to complete to be sure the PV value is for current NDArray and not the previous one.
  - One is doing a scan and writing the NDArrays to a file with one of the file plugins. It is necessary to wait
    for the file plugin to complete before changing the file name for the next point.
* There are 2 problems with setting CallbacksBlock=Yes.
  - It slows down the driver because the plugin is executing in the driver thread and not in its own thread.
  - It is complicated to change all of the required plugin settings from CallbacksBlock=No to CallbacksBlock=Yes.
* The NDPluginDriver base class now increments a NumQueuedArrays counter in the driver that owns each NDArray as it is queued,
  and decrements the counter after the processing is done.
* All drivers have 3 new records:
  - NumQueuedArrays: This record indicates the total number of NDArrays that are currently processing or are queued
    for processing by this driver.
  - WaitForPlugins: This record determines whether AcquireBusy waits for NumQueuedArrays to go to 0 before changing to 0 when acquisition completes.
  - AcquireBusy This is a busy record that is set to 1 when Acquire changes to 1. It changes back to 0 when acquisition completes,
    i.e. when Acquire_RBV=0. If WaitForPlugins is Yes then it also waits for NumQueuedArrays to go to 0 before changing to 0.
* The ADCollect sub-screen now contains these 3 PVs.
* The ADBase screen contains the ADCollect screen, so it shows these PVs.
* Driver screens typically do not use the ADCollect sub-screen, so they need to be individually edited to contain these PVs.
  They are not yet all complete.
* With this new design it should rarely be necessary to change plugins to use CallbacksBlock=Yes.

### NDArray, NDArrayPool
* Changes to allow the NDArray class to be inherited by derived classes.  Thanks to Sinisa Veseli for this.
* Added the epicsTS (EPICS time stamp) field to the report() output.
  Previously the timeStamp field was in the report, but not the epicsTS field was not.
* NDArrayPool::report() now prints a summary of the freeList entries if details>5 and shows the details
  of each array in the freeList if details>10.  This information can be printed with "asynReport 6 driverName" for example.

### NDPluginPva
* Added call to NDPluginDriver::endProcessCallbacks at the end of processCallbacks().
  This will do NDArray callbacks if enabled and will copy the last NDArray to pArrays[0] for asynReport.

### ntndArrayConverter.cpp
* Added conversion of the NDArray.timeStamp and NDArray.epicsTS fields from EPICS epoch (Jan. 1 1990) to
  Posix epoch (Jan. 1, 1970). Needed because NDArrays use EPICS epoch but pvAccess uses Posix epoch and the
  timestamps shown by pvGet were incorrect for the NTNDArrays.

### NDPluginFile
* Fixes to readFileBase() so that the ReadFile PV actually works.  This has now been implemented for NDFileTIFF.
* Return an error if Capture mode is enabled in NDFileModeSingle.

### NDFileTIFF
* Added support for readFile() so it is now possible to read a TIFF file into an NDArray using this plugin and
  do callbacks to downstream plugins.
  - All datatypes (NDDataType_t) are supported.
  - It supports Mono, RGB1, and RGB3 color modes.  It also correctly reads files written with RGB2 color mode.
  - It restores the NDArray fields uniqueID, timeStamp, and epicsTS if they are present.
  - It restores all of the NDArray NDAttributes that were written to the TIFF file.
    Because of the way the NDAttributes are stored in the TIFF file the restored attributes are all of type NDAttrString,
    rather than the numeric data types the attributes may have originally used.
  - One motivation for adding this capability is for the NDPluginProcess plugin to be able to read TIFF files
    for the background and flat field images, rather than needing to collect them each time it is used.

### NDPluginProcess
* Load a dedicated TIFF plugin for the NDPluginProcess plugin in commonPlugins.cmd.  This TIFF plugin is used for reading
  background or flatfield TIFF files.
* Add an sseq record to load the background image from a TIFF file. This executes all the following steps:
  1. Saves the current NDArrayPort fo the Process plugin to a temporary location
  2. Sets the NDArrayPort to the TIFF plugin.
  3. Enables ArrayCallbacks for the TIFF plugin in case they were disabled.
  4. Process the ReadFile record in the TIFF plugin.  This reads the TIFF file and does callback to the Process plugin.
  5. Loads the NDArray from the callback into the background image.
  6. Restores the previous NDArrayPort from the temporary location.
* Add an sseq record to load the flatfile from a TIFF file.  This executes the same steps as for the background
  above, except that in step 5 it loads the NDArray into the flatfile image.

### NDPluginStats
* Changed the time series to use NDPluginTimeSeries, rather than having the time series logic in NDPluginStats.
  This reduced the code by 240 lines, while adding the capability of running in Circular Buffer mode,
  not just a fixed number of time points.
* NOTE: The names of the time series arrays for each statistic have not changed.  However, the name of the PVs to control
  the time series acquisition have changed, for example from $(P)$(R)TSControl, to $(P)$(R)TS:TSAcquire.  This may
  require changes to clients that were controlling time series acquisitions.
* EXAMPLE_commonPlugins.cmd has changed to load an NDPluginTimeSeries plugin and database for each NDPluginStats plugin,
  so the local commonPlugins.cmd file must be updated.

### ADApp/Db/
* Added default ADDR=0 and TIMEOUT=1 to many template files.  This means these values do not need to be specified
  when loading these databases if these defaults are acceptable, which is often the case.

### ADApp/op/adl
* Fixes to a number of .adl files to set text widget size and alignment, etc. to improve conversion to .opi and .ui files.

### ADApp/op/edl/autoconvert
* Major improvement in quality of edm screens (colors, fonts, etc.) thanks to Bruce Hill.

### ADApp/pluginTests
* Added a new unit test, test_NDArrayPool to test NDArrayPool::alloc().
* All unit tests were changed to create an asynNDArrayDriver and use the NDArrayPool from that, rather than directly
  creating an NDArrayPool.


## __R3-2 (January 28, 2018)__

### NDPluginStats
* Previously the X axis for the histogram plot was just the histogram bin number.
  Added code to compute an array of intensity values and a new HistHistogramX_RBV waveform record which
  contains the intensity values for the X axis of the histogram plot.
  This uses a new NDPlotXY.adl medm screen which accepts both X and Y waveform records to plot.

### NDPluginFile
* Changed the way that capture mode is implemented.
  Previously it created NumCapture NDArrays using the "new" operator.
  As NDArrays arrived it copied the data and attributes into these arrays.
  This had several problems:
  - The NDArrays were not allocated from the NDArrayPool, so memory limits were not enforced.
  - Because they were not allocated from the NDArrayPool if they were passed to other functions
    or plugins there would be problems with attempts to call NDArray::reserve() or release().
  - The copy operation is inefficient and not a good idea.

  The change was to simply allocate an array of NumCapture pointers.  As NDArrays arrive
  their reference count is incremented with reserve() and the pointer is copied to the array.
  After the files are written the cleanup routine now simply decrements the reference counter
  with release(), rather than having to delete the NDArray.
  The new scheme is much cleaner.  It will require setting the memory and array limits
  for the NDArrayPool to be large enough to buffer NumCapture frames, whereas previously
  this would not have been required.  It may thus require some changes to startup scripts.

### NDFileHDF5
* Added support for blosc compression library.  The compressors include blosclz, lz4, lz4hc, snappy, zlib, and zstd.
  There is also support for ByteSuffle and BitShuffle.
  ADSupport now contains the blosc library, so it is available for most architectures.
  The build flags WITH_BLOSC, BLOSC_EXTERNAL, and BLOSC_LIB have been added, similar to other optional libraries.
  Thanks to Xiaoqiang Wang for this addition.
* Changed all output records in NDFileHDF.template to have PINI=YES.  This is how other plugins all work.

### NDPluginOverlay
* Improved the behavior when changing the size of an overlay. Previously the Position was always preserved when
  the Size was changed. This was not the desired behavior when the user had set the Center rather than Position.
  Now the code remembers whether Position or Center was last set, and preserves the appropriate value when
  the Size is changed.
* Overlays were previously constrained to fit inside the image on X=0 and Y=0 edges.  However, the user may want part of
  the overlay to be outside the image area. The location of the overlay can now be set anywhere, including negative positions.
  Each pixel in the overlay is now only added if it is within the image area.
* Fixed problems with incorrect drawing and crashing if part of an overlay was outside the image area.
* Removed rounding when setting the center from the position or the position from the center.
  This was causing the location to change when setting the same center or position.
* Changed the cross overlay so that it is drawn symmetrically with the same number of pixels on each side of the center.
  This means the actual size is 2*Size/2 + 1, which will be Size+1 if Size is even.

### asynNDArrayDriver
* Added a second checkPath() method which takes an std::string argument for the path to check.
  It adds the  appropriate terminator if not present, but does not set the NDFilePath parameter.
  This new method is now used by the existing checkPath() method, but can also be used by other code.

### Operator displays (medm, edm, caQtDM, CSS-BOY)
* Fixed medm files in several ways:
  - Text graphics widget sizes are set to the actual size of the text.  medm will display text outside the widget if it
    is not large enough, but other display managers will not.
  - Text update widgets were set to the correct datatype.  medm will display an enum widget as a string even if
    the datatype is set to "decimal" rather than "string", but other display managers will not.
  - Text widgets in titles that use macros (e.g. $(P)$(R)) were set to be as large as possible so they can display
    long PV names with display managers that won't display text outside of the widget.
* Added ADApp/op/Makefile.  This Makefile runs the conversion tools to convert the medm adl files to edl for edm,
  ui for caQtDM, and opi for CSS-BOY.  A RULES_OPI file was added to synApps/support/configure to support this.
  If that RULES_OPI file is not found the Makefile does nothing. If the RULES_OPI file is found then a CONFIG_SITE
  file in synApps/configure or in EPICS base must define these symbols:
  - ADL2EDL is the path to adl2edl for edm
  - ADL2UI is the path to adl2ui for caQtDM
  - CSS is the path to css.  It must be a recent version that supports the command
<pre>
   css -nosplash -application org.csstudio.opibuilder.adl2boy.application
</pre>
* The edl/autoconvert, ui/autoconvert, and opi/autoconvert directories contain new conversions of all of the medm files.
* The edl, ui, and opi directories are intended to contain manually tweaked versions of the files.  Many of the
  files in these directories have been removed, either because they were actually old autoconverted files, or because
  they are obsolete and the new autoconverted files are better.

### commonDriverMakefile
* The variable PROD_NAME has been replaced with DBD_NAME.  This makes it clear that this variable is used to
  specify the name of the application DBD file.  It allows different architectures to use different DBD file
  names.  For backwards compatibility if PROD_NAME is specified and DBD_NAME is not then DBD_NAME will be
  set to PROD_NAME.

### NDFileTIFF
* Improved asynTrace debugging.

### NDPluginAttrPlot
* Bug fix, start the plugin in the constructor.

### NDPluginROIStat
* Fixed array delete at end of processCallbacks().

### ntndArrayConvert.cpp
* Minor fix to work with EPICS 7.

### NDPluginDriver
* Force queueSize to be >=1 when creating queues in createCallbackThreads.  Was crashing when autosave value was 0.

### NDScatter.template
* Removed SCAN=I/O Intr for an output record which was a mistake and could cause crashes.

### pluginTests/Makefile
* Fixed errors with extra parentheses that were preventing include USR_INCLUDES directories from being added.

### XML_schema/NDAttributes.xsd
* Removed the "when" attribute, this is not supported in NDAttributes XML files, only in NDFileHDF5 layout XML files.

### EPICS V4 (pvAccess)
* Changed the Makefile variable from WITH_EPICS_V4 to WITH_PVA.  This is more consistent with the EPICS 7
  release, where the V4 name is no longer used.


## __R3-1 (July 3, 2017)__

### GraphicsMagick
* Changes to commonDriverMakefile and commonLibraryMakefile so they work GraphicsMagick both from
  its recent addition to ADSupport R1-3 (GRAPHICSMAGICK_EXTERNAL=NO) and as a
  system library (GRAPHICSMAGIC_EXTERNAL=YES).
* Added support for 32-bit images in NDFileMagick.
* Improved the documentation for NDFileMagick.

### pluginSrc/Makefile, pluginTests/Makefile
* Fixed some problems with XXX_INCLUDE definitions (XXX=HDF5, XML2, SZIP, GRAPHICSMAGICK, BOOST).

### NDPluginDriver
* Fixed limitation where the ArrayPort string was limited to 20 characters.  There is now no limit.
* Fixed problem with the value of the QueueFree record at startup and when the queue size was changed.
  The queue size logic was OK but the displayed value of QueueFree was wrong.

### ADTop.adl
* Add PhotonII detector.

### NDPluginAttrPlot
* Added documentation.

### NDPluginPva
* Added performance measurements to documentation.

### asynNDArrayDriver.h
* Include ADCoreVersion.h from this file so drivers don't need to explicitly include it.


## __R3-0 (May 5, 2017)__

### Requirements
* This release requires EPICS base 3.14.12.4 or higher because it uses the CFG rules which were fixed
  in that release.

### Incompatible changes
* This release is R3-0 rather than R2-7 because a few changes break backwards compatibility.
  * The constructors for asynNDArray driver and NDPluginDriver no longer take a numParams argument.
    This takes advantage of the fact that asynPortDriver no longer requires parameter counting as of R4-31.
    The constructor for ADDriver has not been similarly changed because this would require changing all drivers,
    and it was decided to wait until future changes require changing drivers before doing this.
  * The constructor for NDPluginDriver now takes an additional maxThreads argument.
  * NDPluginDriver::processCallbacks() has been renamed to NDPluginDriver::beginProcessCallbacks().
  * These changes will require minor modifications to any user-written plugins and any drivers that are derived directly
    from asynNDArrayDriver.
  * All of the plugins in ADCore, and other plugins in the areaDetector project
    (ADPluginEdge, ffmpegServer, FastCCP, ADPCO, ADnED) have had these changes made.
  * The constructors and iocsh configuration commands for NDPluginStdArrays and NDPluginPva have been changed
    to add the standard maxBuffers argument.  EXAMPLE_commonPlugins.cmd has had these changes made.
    Local startup scripts may need modifications.

### Multiple threads in single plugins (NDPluginDriver, NDPluginBase.template, NDPluginBase.adl, many plugins)
* Added support for multiple threads running the processCallbacks() function in a single plugin.  This can improve
  the performance of the plugin by a large factor.  Linear scaling with up to 5 threads (the largest
  value tested) was observed for most of the plugins that now support multiple threads.
  The maximum number of threads that can be used for the plugin is set in the constructor and thus in the
  IOC startup script.  The actual number of threads to use can be controlled via an EPICS PV at run time,
  up to the maximum value passed to the constructor.
  Note that plugins need to be modified to be thread-safe for multiple threads running in a single plugin object.
  The following table describes the support for multiple threads in current plugins.

| Plugin               | Supports multiple threads | Comments                                                      |
| ------               | ------------------------- | --------                                                      |
| NDPluginFile         | No                        | File plugins are nearly always limited by the file I/O, not CPU |
| NDPluginAttribute    | No                        | Plugin does not do any computation, no gain from multiple threads |
| NDPluginCircularBuff | No                        | Plugin does not do any computation, no gain from multiple threads |
| NDPluginColorConvert | Yes                       | Multiple threads supported and tested |
| NDPluginFFT          | Yes                       | Multiple threads supported and tested |
| NDPluginGather       | No                        | Plugin does not do any computation, no gain from multiple threads |
| NDPluginOverlay      | Yes                       | Multiple threads supported and tested |
| NDPluginProcess      | No                        | The recursive filter stores results in the object itself, hard to make thread safe |
| NDPluginPva          | No                        | Plugin is very fast, probably not much gain from multiple threads |
| NDPluginROI          | Yes                       | Multiple threads supported and tested |
| NDPluginROIStat      | Yes                       | Multiple threads supported and tested. Note: the time series arrays may be out of order if using multiple threads |
| NDPluginScatter      | No                        | Plugin does not do any computation, no gain from multiple threads |
| NDPluginStats        | Yes                       | Multiple threads supported and tested. Note: the time series arrays may be out of order if using multiple threads |
| NDPluginStdArrays    | Yes                       | Multiple threads supported and tested. Note: the callbacks to the waveform records may be out of order if using multiple threads|
| NDPluginTimeSeries   | No                        | Plugin does not do much computation, no gain from multiple threads |
| NDPluginTransform    | Yes                       | Multiple threads supported and tested. |
| NDPosPlugin          | No                        | Plugin does not do any computation, no gain from multiple threads |

* Added a new endProcessCallbacks method so derived classes do not need to each implement the logic to call
  downstream plugins.  This method supports optionally sorting the output callbacks by the NDArray::UniqueId
  value.  This is very useful when running multiple threads in the plugin, because these are likely to do
  their output callbacks in the wrong order.  The base class will sort the output NDArrays to be in the correct
  order when possible.  The sorting capability is also useful for the new NDPluginGather plugin, even though
  it only uses a single thread.
* Renamed NDPluginDriver::processCallbacks() to NDPluginDriver::beginProcessCallbacks(). This makes it clearer
  that this method is intended to be called at the beginning of processCallbacks() in the derived class.
  NDPluginDriver::processCallbacks() is now a pure virtual function, so it must be implemented in the derived
  class.
* Added new parameter NDPluginProcessPlugin and new bo record ProcessPlugin.  NDPluginDriver now stores
  the last NDArray it receives.  If the ProcessPlugin record is processed then the plugin will execute
  again with this last NDArray.  This allows modifying plugin behaviour and observing the results
  without requiring the underlying detector to collect another NDArray.  If the plugin is disabled then
  the NDArray is released and returned to the pool.
* Moved many of the less commonly used PVs from NDPluginBase.adl to new file NDPluginBaseFull.adl.  This reduces the screen
  size for most plugin screens, and hides the more obscure PVs from the casual user.  The More related display widget in
  NDPluginBase.adl can now load both the asynRecord.adl and the NDPluginBaseFull.adl screens.
* iocBoot/EXAMPLE_commonPlugins.cmd now accepts an optional MAX_THREADS environment variable.  This defines
  the maximum number of threads to use for plugins that can run in multiple threads.  The default is 5.

### NDPluginScatter
* New plugin NDPluginScatter is used to distribute (scatter) the processing of NDArrays to multiple downstream plugins.
  It allows multiple intances of a plugin to process NDArrays in parallel, utilizing multiple cores
  to increase throughput. It is commonly used together with NDPluginGather, which gathers the outputs from
  multiple plugins back into a single stream.
  This plugin works differently from other plugins that do callbacks to downstream plugins.
  Other plugins pass each NDArray that they generate of all downstream plugins that have registered for callbacks.
  NDPluginScatter does not do this, rather it passes each NDArray to only one downstream plugin.
  The algorithm for chosing which plugin to pass the next NDArray to can be described as a modified round-robin.
  The first NDArray is passed to the first registered callback client, the second NDArray to the second client, etc.
  After the last client the next NDArray goes to the first client, and so on. The modification to strict round-robin
  is that if client N input queue is full then an attempt is made to send the NDArray to client N+1,
  and if this would fail to client N+2, etc. If no clients are able to accept the NDArray because their queues are
  full then the last client that is tried (N-1) will drop the NDArray. Because the "last client" rotates according
  to the round-robin schedule the load of dropped arrays will be uniform if all clients are executing at the same
  speed and if their queues are the same size.

### NDPluginGather
* New plugin NDPluginGather is used to gather NDArrays from multiple upstream plugins and merge them into a single stream.
  When used together with NDPluginScatter it allows multiple intances of a plugin to process NDArrays
  in parallel, utilizing multiple cores to increase throughput.
  This plugin works differently from other plugins that receive callbacks from upstream plugins.
  Other plugins subscribe to NDArray callbacks from a single upstream plugin or driver.
  NDPluginGather allows subscribing to callbacks from any number of upstream plugins.
  It combines the NDArrays it receives into a single stream which it passes to all downstream plugins.
  The EXAMPLE_commonPlugins.cmd and medm files in ADCore allow up to 8 upstream plugins, but this number can
  easily be changed by editing the startup script and operator display file.

### NDPluginAttrPlot
 * New plugin that caches NDAttribute values for an acquisition and exposes values of the selected ones to the EPICS
   layer periodically.  Written by Blaz Kranjc from Cosylab.  No documentation yet, but it should be coming soon.

### asynNDArrayDriver, NDFileNexus
* Changed XML file parsing from using TinyXml to using libxml2.  TinyXml was originally used because libxml2 was not
  available for vxWorks and Windows.  libxml2 was already used for NDFileHDF5 and NDPosPlugin, originally using pre-built
  libraries for Windows in ADBinaries.  ADSupport now provides libxml2, so it is available for all platforms, and
  there is no need to continue building and using TinyXml.  This change means that libxml2 is now required, and so
  the build option WITH_XML2 is no longer used.  XML2_EXTERNAL is still used, depending on whether the version
  in ADSupport or an external version of the library should be used.  The TinyXml source code has been removed from
  ADCore.
* Added support for macro substitution in the XML files used to define NDAttributes.  There is a new NDAttributesMacros
  waveform record that contains the macro substitution strings, for example "CAMERA=13SIM1:cam1:,ID=ID34:".
* Added a new NDAttributesStatus mbbi record that contains the status of reading the attributes XML file.
  It is used to indicate whether the file cannot be found, if there is an XML syntax error, or if there is a
  macro substitutions error.

### PVAttribute
* Fixed a race condition that could result in PVAttributes not being connected to the channel.  This was most likely
  to occur for local PVs in the areaDetector IOC where the connection callback would happen immediately, before the
  code had been initialized to handle the callback. The race condition was introduced in R2-6.

### NDFileHDF5
* Fixed a problem with PVAttributes that were not connected to a PV.  Previously this generated errors from the HDF5
  library because an invalid datatype of -1 was used.  Now the data type for such disconnected attributes is set to
  H5T_NATIVE_FLOAT and the fill value is set to NAN.  No data is written from such attributes to the file, so the
  fill value is used.

### Plugin internals
* All plugins were modified to no longer count the number of parameters that they define, taking advantage of
  this feature that was added to asynPortDriver in asyn R4-31.
* All plugins were modified to call NDPluginDriver::beginProcessCallbacks() rather than
  NDPluginDriver::processCalbacks() at the beginning of processCallbacks() as described above.
* Most plugins were modified to call NDPluginDriver::endProcessCallbacks() near the end of processCallbacks().
  This takes care of doing the NDArray callbacks to downstream plugins, and sorting the output NDArrays if required.
  It also handles the logic of caching the last NDArray in this->pArrays[0].  This significantly simplifies the code
  in the derived plugin classes.
* Previously all plugins were releasing the asynPortDriver lock when calling doCallbacksGenericPointer().
  This was based on a very old observation of a deadlock problem if the the lock was not released.  Releasing
  the lock causes serious problems with plugins running multiple threads, and probably was never needed.  Most
  plugins no longer call doCallbacksGenericPointer() directly because it is now done in NDPluginDriver::endProcessCallbacks().
  The lock is no longer released when calling doCallbacksGenericPointer().  The simDetector driver has also been modified
  to no longer release the lock when calling plugins with doCallbacksGenericPointer(), and all other drivers should be
  modified as well.  It is not really a problem with drivers however, since the code doing those callbacks is normally
  only running in a single thread.

### commonLibraryMakefile, commonDriverMakefile
* These files are now installed in the top-level $(ADCORE)/cfg directory.  External software that uses these files
  (e.g. plugins and drivers not in ADCore) should be changed to use this location rather than $(ADCORE)/ADApp/ since
  the location of the files in the source tree could change in the future.

### NDOverlayN.template
* Removed PINI=YES from CenterX and CenterY records.  Only PositionX/Y should have PINI=YES, otherwise
  the behavior depends on the order of execution with SizeX/Y.

### Viewers
* The ADCore/Viewers directory containing the ImageJ and IDL viewers has been moved to its own
[ADViewers repository](https://github.com/areaDetector/ADViewers).
* It now contains a new ImageJ EPICS_NTNDA_Viewer plugin written by Tim Madden and Marty Kraimer.
  It is essentially identical to EPICS_AD_Viewer.java except that it displays NTNDArrays from the NDPluginPva plugin,
  i.e. using pvAccess to transport the images rather than NDPluginStdArrays which uses Channel Access.
* EPICS_AD_Viewer.java has been changed to work with the new ProcessPlugin feature in NDPluginDriver by monitoring
  ArrayCounter rather than UniqueId.


## __R2-6 (February 19, 2017)__

### NDPluginDriver, NDPluginBase.template, NDPluginBase.adl
* If blockCallbacks is non-zero in constructor then it no longer creates a processing thread.
  This saves resources if the plugin will only be used in blocking mode.  If the plugin is changed
  to non-blocking mode at runtime then the thread will be created then.
* Added new parameter NDPluginExecutionTime and new ai record ExecutionTime_RBV.  This gives the execution
  time in ms the last time the plugin ran.  It works both with BlockingCallbacks=Yes and No.  It is very
  convenient for measuring the performance of the plugin without having to run the detector at high
  frame rates.

### NDArrayBase.template
* Added new longout record NDimensions and new waveform record Dimensions to control the NDArray
  dimensions.  These were needed for NDDriverStdArrays, and may be useful for other drivers.
  Previously there were only input records (NDimensions_RBV and Dimensions_RBV)
  for these parameters.

### NDPluginSupport.dbd
* Build this file in Makefile, remove from source so it is easier to maintain correctly.

### NDPluginROI
* Added CollapseDims to optionally collapse (remove) output array dimensions whose value is 1.
  For example an output array that would normally have dimensions [1, 256, 256] would be
  [256, 256] if CollapseDims=Enable.

### pluginTests
* Added ROIPluginWrapper.cpp to test the CollapseDims behavior in NDPluginROI.

### NDPluginTransform
* Set the NDArraySize[X,Y,Z] parameters appropriately after the transformation.  This is also done
  by the ROI plugin, and is convenient for clients to see the sizes, since the transform can
  swap the X and Y dimensions.

### NDPluginOverlay
* Added new Ellipse shape to draw elliptical or circular overlays.
* Improved efficiency by only computing the coordinates of the overlay pixels when the overlay
  definition changes or the image format changes.  The pixel coordinates are saved in a list.
  This is particularly important for the new Ellipse shape because it uses trigonometric functions
  to compute the pixel coordinates. When neither the overlay definition or the image format changes
  it now just sets the pixel values for each pixel in the list.
* Added CenterX and CenterY parameter for each overlay.  One can now specify the overlay location
  either by PositionX and PositionY, which defines the position of the upper left corner of the
  overlay, or by CenterX and CenterY, which define the location of the center of the overlay.
  If CenterX/Y is changed then PositionX/Y will automatically update, and vice-versa.
* Changed the meaning of SizeX and SizeY for the Cross overlay shape.  Previously the total size
  of a Cross overlay was SizeX\*2 and SizeY\*2.  It is now SizeX and SizeY.  This makes it consistent
  with the Rectangle and Overlay shapes, i.e. drawing each of these shapes with the same PositionX
  and SizeX/Y will result in shapes that overlap in the expected manner.
* Slightly changed the meaning of SizeX/Y for the Cross and Rectangle shapes.  Previously the total
  size of the overlay was SizeX and SizeY.  Now it is SizeX+1 and SizeY+1, i.e. the overlay extends
  +-SizeX/2 and +-SizeY/2 pixels from the center pixel.  This preserves symmetry when WidthX/Y is 1,
  and the previous behavior is difficult to duplicate for the Ellipse shape.

### NDPluginStats
* Extensions to compute Centroid
  * Added calculations of 3rd and 4th order image moments, this provides skewness and kurtosis.
  * Added eccentricity and orientation calculations.
* Changed Histogram.
  * Previously the documentation stated that all values less than or equal to HistMin will
    be in the first bin of the histogram, and all values greater than or equal to histMax will
    be in last bin of the histogram.  This was never actually implemented; values outside the range
    HistMin:HistMax were not included in the histogram at all.
  * Rather than change the code to be consistent with the documentation two new records were added,
    HistBelow and HistAbove.  HistBelow contains the number of values less than HistMin,
    while HistAbove contains the number of values greater than HistMax.  This was done
    because adding a large number of values to the first and last bins of the histogram would change
    the entropy calculation, and also make histogram plots hard to scale nicely.

### NDPluginPos
* Added NDPos.adl medm file.
* Removed NDPosPlugin.dbd from NDPluginSupport.dbd because it can only be built if WITH_XML2 is set.

### NDPluginPva
* The pvaServer is no longer started in the plugin code, because that would result in running multiple servers
  if multiple NDPluginPva plugins were loaded.  Now the command "startPVAServer" must be added to the IOC startup
  script if any NDPluginPva plugins are being used.

### NDPluginFile
* If the NDArray contains an attribute named FilePluginClose and the attribute value is non-zero then
  the current file will be closed.

### NDFileTIFF
* If there is an NDAttribute of type NDAttrString named TIFFImageDescription then this attribute is written
  to the TIFFTAG_IMAGEDESCRIPTION tag in the TIFF file.  Note that it will also be written to a user
  tag in the TIFF file, as with all other NDAttributes.  This is OK because some data processing code
  may expect to find the information in one location or the other.
* Added documentation on how the plugin writes NDAttributes to the TIFF file.

### NDAttribute
* Removed the line `#define MAX_ATTRIBUTE_STRING_SIZE 256` from NDAttribute.h because it creates the
  false impression that there is a limit on the size of string attributes.  There is not.
  Some drivers and plugins may need to limit the size, but they should do this with local definitions.
  The following files were changed to use local definitions, with these symbolic names and values:

| File                           | Symbolic name             | Value |
| ------------------------------ | ------------------------- | ----- |
| NDFileHDF5AttributeDataset.cpp | MAX_ATTRIBUTE_STRING_SIZE | 256   |
| NDFileNetCDF.cpp               | MAX_ATTRIBUTE_STRING_SIZE | 256   |
| NDFileTIFF.cpp                 | STRING_BUFFER_SIZE        | 2048  |

### paramAttribute
* Changed to read string parameters using the asynPortDriver::getStringParam(int index, std::string&amp;)
  method that was added in asyn R4-31.  This removes the requirement that paramAttribute specify a maximum
  string parameters size, there is now no limit.

### PVAttribute
* Fixed problem where a PV that disconnected and reconnected would cause multiple CA subscriptions.
  Note that if the data type of the PV changes on reconnect that the data may not be correct because
  the data type of a PVAttribute is not allowed to change.  The application must be restarted in this
  case.

### asynNDArrayDriver, NDArrayBase.template, NDPluginBase.adl, ADSetup.adl, all plugin adl files
* Added 2 new parameters: NDADCoreVersion, NDDriverVersion and new stringin records ADCoreVersion_RBV and
  DriverVersion_RBV.  These show the version of ADCore and of the driver or plugin that the IOC was
  built with.  Because NDPluginBase.adl grew larger all of the other plugin adl files have changed
  their layouts.

### ADDriver, ADBase.template, ADSetup.adl
* Added 3 new parameters: ADSerialNumber, ADFirmwareVersion, and ADSDKVersion and new stringin records
  SerialNumber_RBV, FirmwareVersion_RBV, and SDKVersion_RBV. These show the serial number and firmware
  version of the detector, and the version of the vendor SDK library that the IOC was built with.
  Because ADSetup.adl grew larger all driver adl files need to change their layouts.  This has been done
  for ADBase.adl in ADCore.  New releases of driver modules will have the changed layouts.

### pvaDriver
* Moved the driver into its own repository areaDetector/pvaDriver.  The new repository contains
  both the driver library from ADCore and the example IOC that was previously in ADExample.

### NDArray.cpp
* Print the reference count in the report() method.

### iocBoot/EXAMPLE_commonPlugins.cmd
* Add commented out line to call startPVAServer if the EPICS V4 NDPva plugin is loaded.
  Previously the plugin itself called startPVAServer, but this can result in the function
  being called multiple times, which is not allowed.

### Viewers/ImageJ
* Improvements to EPICS_AD_Viewer.java
  * Automatically set the contrast when a new window is created. This eliminates the need to
    manually set the contrast when changing image size, data type, and color mode in many cases.
  * When the image window is automatically closed and reopened because the size, data type,
    or color mode changes the new window is now positioned in the same location as the window
    that was closed.
* New ImageJ plugin called GaussianProfiler.java.  It was written by Noumane Laanait when he
  was at the APS (currently at ORNL).  This is similar to the standard ImageJ Plot Profile tool in Live
  mode, but it also fits a Gaussian peak to the profile, and prints the fit parameters centroid, FWHM,
  amplitude, and background.  It is very useful for focusing x-ray beams, etc.
* New ImageJ plugin called EPICS_AD_Controller.java.  This plugin allows using the ImageJ ROI tools
  (rectangle and oval) to graphically define the following:
  * The readout region of the detector/camera
  * The position and size of an ROI (NDPluginROI)
  * The position and size of an overlay (NDPluginOverlay)

  The plugin chain can include an NDPluginTransform plugin which changes the image orientation and an
  NDPluginROI plugin that changes the binning, size, and X/Y axes directions.  The plugin corrects
  for these transformations when defining the target object.  Chris Roehrig from the APS wrote an
  earlier version of this plugin.

### Dependencies
* This release requires asyn R4-31 or later because it uses new features in asynPortDriver.


## __R2-5 (October 28, 2016)__

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


## __R2-4 (September 21, 2015)__

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


## __R2-3 (July 23, 2015)__

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


## __R2-2 (March 23, 2015)__

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
<pre>
    epicsEnvSet("CBUFFS", "500")
    epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")
    dbLoadRecords("NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,
                 TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Int8,FTVL=UCHAR,NELEMENTS=3145728")
</pre>

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
  A similar include mechanism was applied to the `*_settings.req` files, which simplifies commonPlugin_settings.req.
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


## __R2-1 (October 17, 2014)__

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
  [top-level documentation] (https://cars.uchicago.edu/software/epics/areaDetector.html).
  This contains for each module, links to:
  - Github repository
  - Documentation
  - Release Notes
  - Directory containing pre-built binary files
* Added support for cygwin32 architecture.  This did not work in R2-0.


## __R2-0 (April 4, 2014)__

### General
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
* Changed all report() methods to have a `FILE *fp` argument so output can go to a file.
* Added a new field, epicsTS, to the NDArray class. The existing timeStamp field is a double,
  which is convenient because it can be easily displayed and interpreted.  However, it cannot
  preserve all of the information in an epicsTimeStamp, which this new field does.
  This is the definition of the new epicsTS field.
<pre>
epicsTimeStamp epicsTS;  /**> The epicsTimeStamp; this is set with
                          * pasynManager->updateTimeStamp(),
                          * and can come from a user-defined timestamp source. */
</pre>
* Added 2 new asynInt32 parameters to the asynNDArrayDriver class.
    - NDEpicsTSSec contains NDArray.epicsTS.secPastEpoch
    - NDEpicsTSNsec contains NDArray.epicsTS.nsec

* Added 2 new records to NDPluginBase.template.
    - $(P)$(R)EpicsTSSec_RBV contains the current NDArray.epicsTS.secPastEpoch
    - $(P)$(R)EpicsTSNsec_RBV contains the current NDArray.epicsTS.nsec

* The changes in R2-0 for enhanced timestamp support are described in
[areaDetectorTimeStampSupport](https://cars.uchicago.edu/software/epics/areaDetectorTimeStampSupport.html).

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

### NDPluginStats.
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
* Added 3 new TIFF tags to each TIFF file:
  - Tag=65001, field name=NDUniqueId, field_type=TIFF_LONG, value=NDArray.uniqueId.
  - Tag=65002, field name=EPICSTSSec, field_type=TIFF_LONG, value=NDArray.epicsTS.secPastEpoch.
  - Tag=65003, field name=EPICSTSNsec, field_type=TIFF_LONG, value=NDArray.epicsTS.nsec.

  It was not possible to write the timestamp as a single 64-bit value because TIFF
  does not support 64-bit integer tags. It does have a type called TIFF_RATIONAL which
  is a pair of 32-bit integers. However, when reading such a tag what is returned
  is the quotient of the two numbers, which is not what is desired.

### NDPluginAttribute.
* New plugin that allows trending and publishing an NDArray attribute over channel access.


## __R1-9-1 and earlier__
Release notes are part of the
[areaDetector Release Notes](https://cars.uchicago.edu/software/epics/areaDetectorReleaseNotes.html).
