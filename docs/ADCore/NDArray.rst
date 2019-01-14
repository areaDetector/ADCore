NDArray
=======

NDArray
-------

The NDArray (N-Dimensional array) is the class that is used for passing
detector data from drivers to plugins. An NDArray is a general purpose
class for handling array data. An NDArray object is self-describing,
meaning it contains enough information to describe the data itself. It
can optionally contain "attributes" (class NDAttribute) which contain
meta-data describing how the data was collected, etc.

An NDArray can have up to ND_ARRAY_MAX_DIMS dimensions, currently 10. A
fixed maximum number of dimensions is used to significantly simplify the
code compared to unlimited number of dimensions. Each dimension of the
array is described by an `NDDimension
structure <areaDetectorDoxygenHTML/struct_n_d_dimension.html>`__. The
`NDArray class
documentation <areaDetectorDoxygenHTML/class_n_d_array.html>`__\ describes
this class in detail.

NDArrayPool
-----------

The NDArrayPool class manages a free list (pool) of NDArray objects.
Drivers allocate NDArray objects from the pool, and pass these objects
to plugins. Plugins increase the reference count on the object when they
place the object on their queue, and decrease the reference count when
they are done processing the array. When the reference count reaches 0
again the NDArray object is placed back on the free list. This mechanism
minimizes the copying of array data in plugins. The `NDArrayPool class
documentation <areaDetectorDoxygenHTML/class_n_d_array_pool.html>`__\ describes
this class in detail.

NDAttribute
-----------

The NDAttribute is a class for linking metadata to an NDArray. An
NDattribute has a name, description, data type, value, source type and
source information. Attributes are identified by their names, which are
case-sensitive. There are methods to set and get the information for an
attribute.

It is useful to define some conventions for attribute names, so that
plugins or data analysis programs can look for a specific attribute. The
following are the attribute conventions used in current plugins:

.. raw:: html

  <table class="table table-bordered">
    <tbody>
      <tr>
        <td align="center" colspan="4">
          <b>Conventions for standard attribute names</b> </td>
      </tr>
      <tr>
        <th>
          Attribute name
        </th>
        <th>
          Description
        </th>
        <th>
          Data type
        </th>
      </tr>
      <tr>
        <td>
          ColorMode </td>
        <td>
          Color mode of the NDArray. Used by many plugins to determine how to process the
          array. </td>
        <td>
          NDAttrInt32 (NDColorMode_t) </td>
      </tr>
      <tr>
        <td>
          BayerPattern </td>
        <td>
          Bayer patter of an image collect by a color camera with a Bayer mask. Could be used
          to convert to a an RGB color image. This capability may be added to NDPluginColorConvert.
        </td>
        <td>
          NDAttrInt32 (NDBayerPattern_t) </td>
      </tr>
      <tr>
        <td>
          DriverFilename </td>
        <td>
          The name of the file originally collected by the driver. This is used by NDPluginFile
          to delete the original driver file if the DeleteDriverFile flag is set and the NDArray
          has been successfully saved in another file. </td>
        <td>
          NDAttrString </td>
      </tr>
      <tr>
        <td>
          FilePluginDestination </td>
        <td>
          This is used by NDPluginFile to determine whether to process this NDArray. If this
          attribute is present and is "all" or the name of this plugin then the NDArray is
          processed, otherwise it is ignored. </td>
        <td>
          NDAttrString </td>
      </tr>
      <tr>
        <td>
          FilePluginFileName </td>
        <td>
          This is used by NDPluginFile to set the file name when saving this NDArray. </td>
        <td>
          NDAttrString </td>
      </tr>
      <tr>
        <td>
          FilePluginFileNumber </td>
        <td>
          This is used by NDPluginFile to set the file number when saving this NDArray.
        </td>
        <td>
          NDAttrInt32 </td>
      </tr>
      <tr>
        <td>
          FilePluginFileClose </td>
        <td>
          This is used by NDPluginFile to close the file after processing this NDArray.
        </td>
        <td>
          NDAttrInt32 </td>
      </tr>
      <tr>
        <td>
          [HDF dataset name] </td>
        <td>
          This is used by NDFileHDF5 to determine which dataset in the file this NDArray should
          be written to. The attribute name is the name of the HDF5 dataset. </td>
        <td>
          NDAttrString </td>
      </tr>
      <tr>
        <td>
          [posName] </td>
        <td>
          This is used by NDFileHDF5 to determine which position in the dataset this NDArray
          should be written to. The attribute name is contained in a *PosName* record defined
          in NDFileHDF5.template. This is designed to allow, for example, "snake scan" data
          to be placed in the correct order in an HDF5 file. </td>
        <td>
          NDAttrInt32 </td>
      </tr>
    </tbody>
  </table>


Attribute names are case-sensitive. For attributes not in this table a
good convention would be to use the corresponding driver parameter
without the leading ND or AD, and with the first character of every
"word" of the name starting with upper case. For example, the standard
attribute name for ADManufacturer should be "Manufacturer",
ADNumExposures should be "NumExposures", etc.

The `NDAttribute class
documentation <areaDetectorDoxygenHTML/class_n_d_attribute.html>`__
describes this class in detail.

NDAttributeList
---------------

The NDAttributeList implements a linked list of NDAttribute objects.
NDArray objects contain an NDAttributeList which is how attributes are
associated with an NDArray. There are methods to add, delete and search
for NDAttribute objects in an NDAttributeList. Each attribute in the
list must have a unique name, which is case-sensitive.

When NDArrays are copied with the NDArrayPool methods the attribute list
is also copied.

IMPORTANT NOTE: When a new NDArray is allocated using
``NDArrayPool::alloc()`` the behavior of any existing attribute list on the
NDArray taken from the pool is determined by the value of the global
variable ``eraseNDAttributes``. By default the value of this variable is
0. This means that when a new NDArray is allocated from the pool its
attribute list is **not** cleared. This greatly improves efficiency in
the normal case where attributes for a given driver are defined once at
initialization and never deleted. (The attribute **values** may of
course be changing.) It eliminates allocating and deallocating attribute
memory each time an array is obtained from the pool. It is still
possible to add new attributes to the array, but any existing attributes
will continue to exist even if they are ostensibly cleared e.g.
``asynNDArrayDriver::readNDAttributesFile()`` is called again. If it is
desired to eliminate all existing attributes from NDArrays each time a
new one is allocated then the global variable ``eraseNDAttributes``
should be set to 1. This can be done at the iocsh prompt with the
command:

.. code:: c

   var eraseNDAttributes 1


The `NDAttributeList class
documentation <areaDetectorDoxygenHTML/class_n_d_attribute_list.html>`__
describes this class in detail.

PVAttribute
-----------

The PVAttribute class is derived from NDAttribute. It obtains its value
by monitor callbacks from an EPICS PV, and is thus used to associate
current the value of any EPICS PV with an NDArray. The `PVAttribute
class
documentation <areaDetectorDoxygenHTML/class_p_v_attribute.html>`__
describes this class in detail.

paramAttribute
--------------

The paramAttribute class is derived from NDAttribute. It obtains its
value from the current value of a driver or plugin parameter. The
paramAttribute class is typically used when it is important to have the
current value of the parameter and the value of a corresponding
PVAttribute might not be current because the EPICS PV has not yet
updated. The `paramAttribute class
documentation <areaDetectorDoxygenHTML/classparam_attribute.html>`__
describes this class in detail.

functAttribute
--------------

The functAttribute class is derived from NDAttribute. It obtains its
value from a user-written C++ function. The functAttribute class is thus
very general, and can be used to add almost any information to an
NDArray. ADCore contains example code, myAttributeFunctions.cpp that
demonstates how to write such functions. The `functAttribute class
documentation <areaDetectorDoxygenHTML/classfunct_attribute.html>`__
describes this class in detail.

asynNDArrayDriver
-----------------

asynNDArrayDriver inherits from asynPortDriver. It implements the
asynGenericPointer functions for NDArray objects. This is the class from
which both plugins and area detector drivers are indirectly derived. The
`asynNDArrayDriver class
documentation <areaDetectorDoxygenHTML/classasyn_n_d_array_driver.html>`__\ describes
this class in detail.

The file
`asynNDArrayDriver.h <areaDetectorDoxygenHTML/asyn_n_d_array_driver_8h.html>`__
defines a number of parameters that all NDArray drivers and plugins
should implement if possible. These parameters are defined by strings
(drvInfo strings in asyn) with an associated asyn interface, and access
(read-only or read-write). There is also an integer index to the
parameter which is assigned by asynPortDriver when the parameter is
created in the parameter library. The EPICS database
NDArrayBase.template provides access to these standard driver
parameters. The following table lists the standard driver parameters.
The columns are defined as follows:

-  **Parameter index variable:** The variable name for this parameter
   index in the driver. There are several EPICS records in
   ADBase.template that do not have corresponding parameter indices, and
   these are indicated as Not Applicable (N/A).
-  **asyn interface:** The asyn interface used to pass this parameter to
   the driver.
-  **Access:** Read-write (r/w) or read-only (r/o).
-  **drvInfo string:** The string used to look up the parameter in the
   driver through the drvUser interface. This string is used in the
   EPICS database file for generic asyn device support to associate a
   record with a particular parameter. It is also used to associate a
   `paramAttribute <areaDetectorDoxygenHTML/classparam_attribute.html>`__
   with a driver parameter in the XML file that is read by
   asynNDArrayDriver::readNDAttributesFile
-  **EPICS record name:** The name of the record in ADBase.template.
   Each record name begins with the two macro parameters $(P) and $(R).
   In the case of read/write parameters there are normally two records,
   one for writing the value, and a second, ending in \_RBV, that
   contains the actual value (Read Back Value) of the parameter.
-  **EPICS record type:** The record type of the record. Waveform
   records are used to hold long strings, with length (NELM) = 256 bytes
   and EPICS data type (FTVL) = UCHAR. This removes the 40 character
   restriction string lengths that arise if an EPICS "string" PV is
   used. MEDM allows one to edit and display such records correctly.
   EPICS clients will typically need to convert such long strings from a
   string to an integer or byte array before sending the path name to
   EPICS. In IDL this is done as follows:

.. code::

    
    ; Convert a string to a null-terminated byte array and write with caput
    IDL> t = caput('13PS1:TIFF1:FilePath', [byte('/home/epics/scratch'),0B])
    ; Read a null terminated byte array 
    IDL> t = caget('13PS1:TIFF1:FilePath', v)
    ; Convert to a string 
    IDL> s = string(v) 


In SPEC this is done as follows:

.. code::

    array _temp[256]
    # Setting the array to "" will zero-fill it
    _temp = ""
    # Copy the string to the array.  Note, this does not null terminate, so if array already contains
    # a longer string it needs to first be zeroed by setting it to "".
    _temp = "/home/epics/scratch"
    epics_put("13PS1:TIFF1:FilePath", _temp)
        

Note that for parameters whose values are defined by enum values (e.g
NDDataType, NDColorMode, etc.), drivers can use a different set of enum
values for these parameters. They can override the enum menu in
ADBase.template with driver-specific choices by loading a
driver-specific template file that redefines that record field after
loading ADBase.template.

.. raw:: html

  <table class="table table-bordered">
    <tbody>
      <tr>
        <td align="center" colspan="7">
          <b>Parameter Definitions in asynNDArrayDriver.h and EPICS Record Definitions in NDArrayBase.template
            (file-related records are in NDFile.template)</b> </td>
      </tr>
      <tr>
        <th>
          Parameter index variable
        </th>
        <th>
          asyn interface
        </th>
        <th>
          Access
        </th>
        <th>
          Description
        </th>
        <th>
          drvInfo string
        </th>
        <th>
          EPICS record name
        </th>
        <th>
          EPICS record type
        </th>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Information about the version of ADCore and the plugin or driver</b> </td>
      </tr>
      <tr>
        <td>
          NDADCoreVersion </td>
        <td>
          asynOctet </td>
        <td>
          r/o </td>
        <td>
          ADCore version number. This can be used by Channel Access clients to alter their
          behavior depending on the version of ADCore that was used to build this driver or
          plugin. </td>
        <td>
          ADCORE_VERSION </td>
        <td>
          $(P)$(R)ADCoreVersion_RBV </td>
        <td>
          stringin </td>
      </tr>
      <tr>
        <td>
          NDDriverVersion </td>
        <td>
          asynOctet </td>
        <td>
          r/o </td>
        <td>
          Driver or plugin version number. This can be used by Channel Access clients to alter
          their behavior depending on the version of the plugin or driver. </td>
        <td>
          DRIVER_VERSION </td>
        <td>
          $(P)$(R)DriverVersion_RBV </td>
        <td>
          stringin </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Information about the asyn port</b> </td>
      </tr>
      <tr>
        <td>
          NDPortNameSelf </td>
        <td>
          asynOctet </td>
        <td>
          r/o </td>
        <td>
          asyn port name </td>
        <td>
          PORT_NAME_SELF </td>
        <td>
          $(P)$(R)PortName_RBV </td>
        <td>
          stringin </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Data type</b> </td>
      </tr>
      <tr>
        <td>
          NDDataType </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Data type (NDDataType_t). </td>
        <td>
          DATA_TYPE </td>
        <td>
          $(P)$(R)DataType<br />
          $(P)$(R)DataType_RBV </td>
        <td>
          mbbo<br />
          mbbi </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Color mode</b> </td>
      </tr>
      <tr>
        <td>
          NDColorMode </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Color mode (NDColorMode_t). </td>
        <td>
          COLOR_MODE </td>
        <td>
          $(P)$(R)ColorMode<br />
          $(P)$(R)ColorMode_RBV </td>
        <td>
          mbbo<br />
          mbbi </td>
      </tr>
      <tr>
        <td>
          NDBayerPattern </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          Bayer pattern (NDBayerPattern_t) of NDArray data. </td>
        <td>
          BAYER_PATTERN </td>
        <td>
          $(P)$(R)BayerPattern_RBV </td>
        <td>
          mbbi </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Actual dimensions of array data</b> </td>
      </tr>
      <tr>
        <td>
          NDNDimensions </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Number of dimensions in the array </td>
        <td>
          ARRAY_NDIMENSIONS </td>
        <td>
          $(P)$(R)NDimensions<br />
          $(P)$(R)NDimensions_RBV </td>
        <td>
          longout
          <br />
          longin </td>
      </tr>
      <tr>
        <td>
          NDDimensions </td>
        <td>
          asynInt32Array </td>
        <td>
          r/w </td>
        <td>
          Size of each dimension in the array </td>
        <td>
          ARRAY_DIMENSIONS </td>
        <td>
          $(P)$(R)Dimensions<br />
          $(P)$(R)Dimensions_RBV </td>
        <td>
          waveform (out)<br />
          waveform (in) </td>
      </tr>
      <tr>
        <td>
          N.A. </td>
        <td>
          N.A </td>
        <td>
          r/o </td>
        <td>
          Size of each array dimension, extracted from the $(P)$(R)Dimensions and $(P)$(R)Dimensions_RBV
          waveform records. Note that these are both longin record, i.e. readonly values using
          subarray records. In the future longout records may be added to write to the individual
          values in $(P)$(R)Dimensions. </td>
        <td>
          N.A. </td>
        <td>
          $(P)$(R)ArraySize[N] N=0-9
          <br />
          (P)$(R)ArraySize[N]_RBV </td>
        <td>
          longin
          <br />
          longin </td>
      </tr>
      <tr>
        <td>
          NDArraySizeX </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          Size of the array data in the X direction </td>
        <td>
          ARRAY_SIZE_X </td>
        <td>
          $(P)$(R)ArraySizeX_RBV </td>
        <td>
          longin </td>
      </tr>
      <tr>
        <td>
          NDArraySizeY </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          Size of the array data in the Y direction </td>
        <td>
          ARRAY_SIZE_Y </td>
        <td>
          $(P)$(R)ArraySizeY_RBV </td>
        <td>
          longin </td>
      </tr>
      <tr>
        <td>
          NDArraySizeZ </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          Size of the array data in the Z direction </td>
        <td>
          ARRAY_SIZE_Z </td>
        <td>
          $(P)$(R)ArraySizeZ_RBV </td>
        <td>
          longin </td>
      </tr>
      <tr>
        <td>
          NDArraySize </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          Total size of the array data in bytes </td>
        <td>
          ARRAY_SIZE </td>
        <td>
          $(P)$(R)ArraySize_RBV </td>
        <td>
          longin </td>
      </tr>
      <tr>
        <td>
          NDCodec </td>
        <td>
          asynOctet </td>
        <td>
          r/o </td>
        <td>
          The codec used to compress this array </td>
        <td>
          CODEC </td>
        <td>
          $(P)$(R)Codec_RBV </td>
        <td>
          stringin </td>
      </tr>
      <tr>
        <td>
          NDCompressedSize </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          Compressed size of the array data in bytes. Only meaningful if NDCodec is not empty
          string. </td>
        <td>
          COMPRESSED_SIZE </td>
        <td>
          $(P)$(R)Compressed_RBV </td>
        <td>
          longin </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Array data</b> </td>
      </tr>
      <tr>
        <td>
          NDArrayCallbacks </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Controls whether the driver or plugin does callbacks with the array data to registered
          plugins. 0=No, 1=Yes. Setting this to 0 in a driver can reduce overhead in the case
          that the driver is being used only to control the device, and not to make the data
          available to plugins or to EPICS clients. Setting this to 0 in a plugin can reduce
          overhead by eliminating the need to copy the NDArray if that plugin is not being
          used as a source of NDArrays to other plugins. </td>
        <td>
          ARRAY_CALLBACKS </td>
        <td>
          $(P)$(R)ArrayCallbacks<br />
          $(P)$(R)ArrayCallbacks_RBV </td>
        <td>
          bo<br />
          bi </td>
      </tr>
      <tr>
        <td>
          NDArrayData </td>
        <td>
          asynGenericPointer </td>
        <td>
          r/w </td>
        <td>
          The array data as an NDArray object </td>
        <td>
          NDARRAY_DATA </td>
        <td>
          N/A. EPICS access to array data is through NDStdArrays plugin. </td>
        <td>
          N/A </td>
      </tr>
      <tr>
        <td>
          NDArrayCounter </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Counter that increments by 1 each time an array is acquired. Can be reset by writing
          a value to it. </td>
        <td>
          ARRAY_COUNTER </td>
        <td>
          $(P)$(R)ArrayCounter<br />
          $(P)$(R)ArrayCounter_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td>
          N/A </td>
        <td>
          N/A </td>
        <td>
          r/o </td>
        <td>
          Rate at which arrays are being acquired. Computed in the ADBase.template database.
        </td>
        <td>
          N/A </td>
        <td>
          $(P)$(R)ArrayRate_RBV </td>
        <td>
          calc </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Array attributes</b> </td>
      </tr>
      <tr>
        <td>
          NDAttributesFile </td>
        <td>
          asynOctet </td>
        <td>
          r/w </td>
        <td>
          The name of an XML file defining the NDAttributes to be added to each NDArray by
          this driver or plugin. The format of the XML file is described in the documentation
          for <a href="areaDetectorDoxygenHTML/classasyn_n_d_array_driver.html">asynNDArrayDriver::readNDAttributesFile().</a>
        </td>
        <td>
          ND_ATTRIBUTES_FILE </td>
        <td>
          $(P)$(R)NDAttributesFile </td>
        <td>
          waveform </td>
      </tr>
      <tr>
        <td>
          NDAttributesMacros </td>
        <td>
          asynOctet </td>
        <td>
          r/w </td>
        <td>
          A macro definition string that can be used to do macro substitution in the XML file.
          For example if this string is "CAMERA=13SIM1:cam1:,ID=ID13us:" then all $(CAMERA)
          and $(ID) strings in the XML file will be replaced with 13SIM1:cam1: and ID13us:
          respectively. </td>
        <td>
          ND_ATTRIBUTES_MACROS </td>
        <td>
          $(P)$(R)NDAttributesMacros </td>
        <td>
          waveform </td>
      </tr>
      <tr>
        <td>
          NDAttributesStatus </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          The status of reading and parsing the XML attributes file. This is used to indicate
          if the file cannot be found, if there is an XML syntax error, or if there is a macro
          substitutions error. </td>
        <td>
          ND_ATTRIBUTES_STATUS </td>
        <td>
          $(P)$(R)NDAttributesStatus </td>
        <td>
          mbbi </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Array pool status</b> </td>
      </tr>
      <tr>
        <td>
          NDPoolMaxMemory </td>
        <td>
          asynFloat64 </td>
        <td>
          r/o </td>
        <td>
          The maximum number of NDArrayPool memory bytes that can be allocated. 0=unlimited.
        </td>
        <td>
          POOL_MAX_MEMORY </td>
        <td>
          $(P)$(R)PoolMaxMem </td>
        <td>
          ai </td>
      </tr>
      <tr>
        <td>
          NDPoolUsedMemory </td>
        <td>
          asynFloat64 </td>
        <td>
          r/o </td>
        <td>
          The number of NDArrayPool memory bytes currently allocated. The SCAN rate of this
          record controls the scanning of all of the dynamic NDArrayPool status records.
        </td>
        <td>
          POOL_USED_MEMORY </td>
        <td>
          $(P)$(R)PoolUsedMem </td>
        <td>
          ai </td>
      </tr>
      <tr>
        <td>
          NDPoolAllocBuffers </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          The number of NDArrayPool buffers currently allocated. </td>
        <td>
          POOL_ALLOC_BUFFERS </td>
        <td>
          $(P)$(R)PoolAllocBuffers </td>
        <td>
          longin </td>
      </tr>
      <tr>
        <td>
          NDPoolFreeBuffers </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          The number of NDArrayPool buffers currently allocated but free. </td>
        <td>
          POOL_FREE_BUFFERS </td>
        <td>
          $(P)$(R)PoolFreeBuffers </td>
        <td>
          longin </td>
      </tr>
      <tr>
        <td>
          N.A. </td>
        <td>
          N.A. </td>
        <td>
          r/o </td>
        <td>
          The number of NDArrayPool buffers currently in use. This is calculated as NDPoolAllocBuffers
          - NDPoolFreeBuffers. </td>
        <td>
          N.A. </td>
        <td>
          $(P)$(R)PoolUsedBuffers </td>
        <td>
          calc </td>
      </tr>
      <tr>
        <td>
          NDPoolEmptyFreeList </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Processing this record deletes all of the NDArrays on the freelist and sets the
          freelist size to 0. This provides a mechanism to free large amounts of memory and
          return it to the operating system, for example after a rapid acquisition with large
          plugin queues. On Windows the memory is returned to the operating system immediately.
          On Linux the freed memory may not actually be returned to the operating system even
          though it has been freed in the areaDetector process. On Centos7 (and presumably
          many other versions of Linux) setting the value of the environment variable MALLOC_TRIM_THRESHOLD_
          to a small value will allow the memory to actually be returned to the operating
          system. </td>
        <td>
          POOL_EMPTY_FREELIST </td>
        <td>
          $(P)$(R)EmptyFreeList </td>
        <td>
          bo </td>
      </tr>
      <tr>
        <td>
          NDNumQueuedArrays </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          The number of NDArrays from this driver's NDArrayPool that are currently queued
          for processing by plugins. When this number goes to 0 the plugins have all completed
          processing. </td>
        <td>
          NUM_QUEUED_ARRAYS </td>
        <td>
          $(P)$(R)NumQueuedArrays </td>
        <td>
          longin </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Debugging control</b> </td>
      </tr>
      <tr>
        <td>
          N/A </td>
        <td>
          N/A </td>
        <td>
          N/A </td>
        <td>
          asyn record to control debugging (asynTrace) </td>
        <td>
          N/A </td>
        <td>
          $(P)$(R)AsynIO </td>
        <td>
          asyn </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>File saving parameters (records are defined in NDFile.template)</b> </td>
      </tr>
      <tr>
        <td>
          NDFilePath </td>
        <td>
          asynOctet </td>
        <td>
          r/w </td>
        <td>
          File path </td>
        <td>
          FILE_PATH </td>
        <td>
          $(P)$(R)FilePath<br />
          $(P)$(R)FilePath_RBV </td>
        <td>
          waveform<br />
          waveform </td>
      </tr>
      <tr>
        <td>
          NDFilePathExists </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          Flag indicating if file path exists </td>
        <td>
          FILE_PATH_EXISTS </td>
        <td>
          $(P)$(R)FilePathExists_RBV </td>
        <td>
          bi </td>
      </tr>
      <tr>
        <td>
          NDFileName </td>
        <td>
          asynOctet </td>
        <td>
          r/w </td>
        <td>
          File name </td>
        <td>
          FILE_NAME </td>
        <td>
          $(P)$(R)FileName<br />
          $(P)$(R)FileName_RBV </td>
        <td>
          waveform<br />
          waveform </td>
      </tr>
      <tr>
        <td>
          NDFileNumber </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          File number </td>
        <td>
          FILE_NUMBER </td>
        <td>
          $(P)$(R)FileNumber<br />
          $(P)$(R)FileNumber_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td>
          NDFileTemplate </td>
        <td>
          asynOctet </td>
        <td>
          r/w </td>
        <td>
          Format string for constructing NDFullFileName from NDFilePath, NDFileName, and NDFileNumber.
          The final file name (which is placed in NDFullFileName) is created with the following
          code:
          <pre>epicsSnprintf(
    FullFilename, 
    sizeof(FullFilename), 
    FileTemplate, FilePath, 
    Filename, FileNumber);
        </pre>
          FilePath, Filename, FileNumber are converted in that order with FileTemplate. An
          example file format is <code>"%s%s%4.4d.tif"</code>. The first %s converts the FilePath,
          followed immediately by another %s for Filename. FileNumber is formatted with %4.4d,
          which results in a fixed field with of 4 digits, with leading zeros as required.
          Finally, the .tif extension is added to the file name. This mechanism for creating
          file names is very flexible. Other characters, such as _ can be put in Filename
          or FileTemplate as desired. If one does not want to have FileNumber in the file
          name at all, then just omit the %d format specifier from FileTemplate. If the client
          wishes to construct the complete file name itself, then it can just put that file
          name into NDFileTemplate with no format specifiers at all, in which case NDFilePath,
          NDFileName, and NDFileNumber will be ignored. </td>
        <td>
          FILE_TEMPLATE </td>
        <td>
          $(P)$(R)FileTemplate<br />
          $(P)$(R)FileTemplate_RBV </td>
        <td>
          waveform<br />
          waveform </td>
      </tr>
      <tr>
        <td>
          NDFullFileName </td>
        <td>
          asynOctet </td>
        <td>
          r/o </td>
        <td>
          Full file name constructed using the algorithm described in NDFileTemplate </td>
        <td>
          FULL_FILE_NAME </td>
        <td>
          $(P)$(R)FullFileName_RBV </td>
        <td>
          waveform<br />
          waveform </td>
      </tr>
      <tr>
        <td>
          NDAutoIncrement </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Auto-increment flag. Controls whether FileNumber is automatically incremented by
          1 each time a file is saved (0=No, 1=Yes) </td>
        <td>
          AUTO_INCREMENT </td>
        <td>
          $(P)$(R)AutoIncrement<br />
          $(P)$(R)AutoIncrement_RBV </td>
        <td>
          bo<br />
          bi </td>
      </tr>
      <tr>
        <td>
          NDAutoSave </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Auto-save flag (0=No, 1=Yes) controlling whether a file is automatically saved each
          time acquisition completes. </td>
        <td>
          AUTO_SAVE </td>
        <td>
          $(P)$(R)AutoSave<br />
          $(P)$(R)AutoSave_RBV </td>
        <td>
          bo<br />
          bi </td>
      </tr>
      <tr>
        <td>
          NDFileFormat </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          File format. The format to write/read data in (e.g. TIFF, netCDF, etc.) </td>
        <td>
          FILE_FORMAT </td>
        <td>
          $(P)$(R)FileFormat<br />
          $(P)$(R)FileFormat_RBV </td>
        <td>
          mbbo<br />
          mbbi </td>
      </tr>
      <tr>
        <td>
          NDWriteFile </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Manually save the most recent array to a file when value=1 </td>
        <td>
          WRITE_FILE </td>
        <td>
          $(P)$(R)WriteFile<br />
          $(P)$(R)WriteFile_RBV </td>
        <td>
          busy<br />
          bi </td>
      </tr>
      <tr>
        <td>
          NDReadFile </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Manually read a file when value=1 </td>
        <td>
          READ_FILE </td>
        <td>
          $(P)$(R)ReadFile<br />
          $(P)$(R)ReadFile_RBV </td>
        <td>
          busy<br />
          bi </td>
      </tr>
      <tr>
        <td>
          NDFileWriteMode </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          File saving mode (Single, Capture, Stream)(NDFileMode_t) </td>
        <td>
          WRITE_MODE </td>
        <td>
          $(P)$(R)FileWriteMode<br />
          $(P)$(R)FileWriteMode_RBV </td>
        <td>
          mbbo<br />
          mbbi </td>
      </tr>
      <tr>
        <td>
          NDFileWriteStatus </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          File write status. Gives status information on last file open or file write operation.
          Values are WriteOK (0) and WriteError (1). </td>
        <td>
          WRITE_STATUS </td>
        <td>
          $(P)$(R)FileWriteStatus </td>
        <td>
          mbbi </td>
      </tr>
      <tr>
        <td>
          NDFileWriteMessage </td>
        <td>
          asynOctet </td>
        <td>
          r/o </td>
        <td>
          File write error message. An error message string if the previous file open or file
          write operation resulted in an error. </td>
        <td>
          WRITE_MESSAGE </td>
        <td>
          $(P)$(R)FileWriteMessage </td>
        <td>
          waveform </td>
      </tr>
      <tr>
        <td>
          NDFileCapture </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Start (1) or stop (0) file capture or streaming </td>
        <td>
          CAPTURE </td>
        <td>
          $(P)$(R)Capture<br />
          $(P)$(R)Capture_RBV </td>
        <td>
          busy<br />
          bi </td>
      </tr>
      <tr>
        <td>
          NDFileNumCapture </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Number of frames to acquire in capture or streaming mode </td>
        <td>
          NUM_CAPTURE </td>
        <td>
          $(P)$(R)NumCapture<br />
          $(P)$(R)NumCapture_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td>
          NDFileNumCaptured </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          Number of arrays currently acquired capture or streaming mode </td>
        <td>
          NUM_CAPTURED </td>
        <td>
          $(P)$(R)NumCaptured_RBV </td>
        <td>
          longin </td>
      </tr>
      <tr>
        <td>
          NDFileDeleteDriverFile </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Flag to enable deleting original driver file after a plugin has re-written the file
          in a different format. This can be useful for detectors that must write the data
          to disk in order for the areaDetector driver to read it back. Once a file-writing
          plugin has rewritten the data in another format it can be desireable to then delete
          the original file. </td>
        <td>
          DELETE_DRIVER_FILE </td>
        <td>
          $(P)$(R)DeleteDriverFile<br />
          $(P)$(R)DeleteDriverFile_RBV </td>
        <td>
          bo<br />
          bi </td>
      </tr>
      <tr>
        <td>
          NDFileLazyOpen </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Flag to defer the creation of a new file until the first NDArray to write has been
          received. This removes the need for passing an extra NDArray through the file writing
          plugin to initialise dimensions and possibly NDAttribute list before opening the
          file. The downside is that file creation can potentially be time-consuming so processing
          the first NDArray may be slower than subsequent ones.
          <br />
          Only makes sense to use with file plugins which support multiple frames per file
          and only in "Stream" mode. </td>
        <td>
          FILE_LAZY_OPEN </td>
        <td>
          $(P)$(R)LazyOpen<br />
          $(P)$(R)LazyOpen_RBV </td>
        <td>
          bo<br />
          bi </td>
      </tr>
      <tr>
        <td>
          NDFileCreateDir </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          This parameter is used to automatically create directories if they don't exist.
          If it is zero (default), no directories are created. If it is negative, then the
          absolute value is the maximum of directories that will be created (i.e. -1 will
          create a maximum of one directory to complete the path, -2 will create a maximum
          of 2 directories). If it is positive, then at least that many directories in the
          path must exist (i.e. a value of 1 will create all directories below the root directory
          and 2 will not create a directory in the root directory). </td>
        <td>
          CREATE_DIR </td>
        <td>
          $(P)$(R)CreateDirectory<br />
          $(P)$(R)CreateDirectory_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td>
          NDFileTempSuffix </td>
        <td>
          asynOctet </td>
        <td>
          r/w </td>
        <td>
          If this string is non-null, the file is opened with this suffix temporarily appended
          to the file name. When the file is closed it is then renamed to the correct file
          name without the suffix. This is useful for processing software watching for the
          file to appear since the file appears as an atomic operation when it is ready to
          be opened. </td>
        <td>
          FILE_TEMP_SUFFIX </td>
        <td>
          $(P)$(R)TempSuffix<br />
          $(P)$(R)TempSuffix_RBV </td>
        <td>
          stringout<br />
          stringin </td>
      </tr>
    </tbody>
  </table>


