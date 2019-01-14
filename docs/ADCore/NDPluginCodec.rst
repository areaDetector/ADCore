NDPluginCodec
=============
:author: Bruno Martins, Facility for Rare Isotope Beams
         and Mark Rivers, University of Chicago

.. contents:: Contents

Overview
--------

NDPluginCodec is a tool for compressing and decomppressing NDArray data
according to a user selectable codec. Compression information is stored
in the ``codec`` and ``compressionSize`` fields of the NDArray.

Compressed NDArray Semantics
----------------------------

The new NDArray field ``codec`` is used to indicate if an NDArray holds
compressed or uncompressed data.

Uncompressed NDArrays
~~~~~~~~~~~~~~~~~~~~~

-  ``codec`` is empty (``codec.empty()==true``).
-  ``compressedSize`` is equal to ``dataSize``.

Compressed NDArrays
~~~~~~~~~~~~~~~~~~~

-  ``codec`` holds the name of the codec that was used to compress the
   data. This plugin currently supports two codecs: "jpeg" and "blosc".
-  ``compressedSize`` holds the length of the compressed data in
   ``pData``.
-  ``dataSize`` holds the length of the allocated ``pData`` buffer, as
   usual.
-  ``pData`` holds the compressed data as ``unsigned char``.
-  ``dataType`` holds the data type of the **compressed** data. This
   will be used for decompression.

Compression
-----------

To compress the data, the Mode parameter must be set to Compress. Also,
the parameter Compressor must be set to something other than None. After
the compression is done, the CompFactor parameter will be updated with
the compression factor achieved. CompFactor is calculated according to
the following formula:

``dataSize/compressedSize``

Currently, three choices are available for the Compressor parameter:

-  None: No compression will be performed. The NDArray will be passed
   forward as-is.
-  JPEG: The compression will be performed according to the JPEG format.
   ``pData`` will contain a full, valid JPEG file in memory after the
   compression is done. JPEG compression is controlled with the
   following parameters:

   -  JPEGQuality: The image quality to be used for the compression. The
      quality value must be between 1 and 100, with 100 meaning best
      quality (and worst compression factor).

-  Blosc: The compression will be performed according to the Blosc
   format. The compression is controlled via the following parameters:

   -  BloscCompressor: which compression algorithm to use. Available
      choices: BloscLZ, LZ4, LZ4HC, Snappy, ZLIB and ZSTD.
   -  BloscCLevel: the compression level for the selected algorithm.
   -  BloscShuffle: controls whether data will be shuffled before
      compression. Choices are None, Bit, and Byte.
   -  BloscNumThreads: controls how many threads will be used by the
      Blosc compressor to improve performance.

Note that BloscNumThreads controls the number of threads created from a
single NDPluginCodec thread. The performance of both the JPEG and Blosc
compressors can also be increased by running multiple NDPluginCodec
threads within a single plugin instance. This is controlled with the
NumThreads record, as for most other plugins.

It is important to note that plugins downstream of NDCodec that are
receiving compressed NDArrays **must** have been constructed with
NDPluginDriver's ``compressionAware=true``, otherwise compressed arrays
**will be dropped** by them at runtime. Currently only NDPluginCodec and
NDPluginPva are able to handle compressed NDArrays.

Decompression
-------------

If Mode is set to Decompress, decompression happens automatically and
transparently if the codec is supported. No other parameter needs to be
set for the decompression to work.

Parameters
----------

NDPluginCodec inherits from NDPluginDriver. The `NDPluginCodec class
documentation <areaDetectorDoxygenHTML/class_n_d_plugin_codec.html>`__
describes this class in detail.

NDPluginCodec defines the following parameters. It also implements all
of the standard plugin parameters from
:doc:`NDPluginDriver`. The EPICS database
NDCodec.template provides access to these parameters, listed in the
following table.

.. raw:: html

  <table class="table table-bordered">
    <tbody>
      <tr>
        <td align="center" colspan="7,">
          <b>Parameter Definitions in NDPluginCodec.h and EPICS Record Definitions in NDCodec.template</b>
        </td>
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
        <td>
          NDCodecMode</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          The plugin mode (NDCodecMode_t).</td>
        <td>
          MODE</td>
        <td>
          $(P)$(R)Mode<br />
          $(P)$(R)Mode_RBV </td>
        <td>
          mbbo<br />
          mbbi </td>
      </tr>
      <tr>
        <td>
          NDCodecCompressor</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Which compressor to use (NDCodecCompressor_t).</td>
        <td>
          COMPRESSOR</td>
        <td>
          $(P)$(R)Compressor<br />
          $(P)$(R)Compressor_RBV </td>
        <td>
          mbbo<br />
          mbbi </td>
      </tr>
      <tr>
        <td>
          NDCodecCompFactor</td>
        <td>
          asynFloat64</td>
        <td>
          r/w</td>
        <td>
          Compression factor.</td>
        <td>
          COMP_FACTOR</td>
        <td>
          $(P)$(R)CompFactor</td>
        <td>
          ai</td>
      </tr>
      <tr>
        <td align="center" colspan="7,">
          <b>Parameters for the JPEG Compressor</b> </td>
      </tr>
      <tr>
        <td>
          NDCodecJPEGQuality</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          JPEG compression quality.</td>
        <td>
          JPEG_QUALITY</td>
        <td>
          $(P)$(R)JPEGQuality<br />
          $(P)$(R)JPEGQuality_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td align="center" colspan="7,">
          <b>Parameters for the Blosc Compressor</b> </td>
      </tr>
      <tr>
        <td>
          NDCodecBloscCompressor</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Which Blosc compressor to use (NDCodecBloscComp_t).</td>
        <td>
          BLOSC_COMPRESSOR</td>
        <td>
          $(P)$(R)BloscCompressor<br />
          $(P)$(R)BloscCompressor_RBV </td>
        <td>
          mbbo<br />
          mbbi </td>
      </tr>
      <tr>
        <td>
          NDCodecBloscCLevel</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Blosc compression level.</td>
        <td>
          BLOSC_CLEVEL</td>
        <td>
          $(P)$(R)BloscCLevel<br />
          $(P)$(R)BloscCLevel_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td>
          NDCodecBloscShuffle</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Blosc shuffle data before compression:<br />
          <ul>
            <li>None</li>
            <li>Bit Shuffle</li>
            <li>Byte Shuffle</li>
          </ul>
        </td>
        <td>
          BLOSC_SHUFFLE</td>
        <td>
          $(P)$(R)BloscShuffle<br />
          $(P)$(R)BloscShuffle_RBV </td>
        <td>
          mbbo<br />
          mbbi </td>
      </tr>
      <tr>
        <td>
          NDCodecBloscNumThreads</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Blosc number of threads for compression/decompression.</td>
        <td>
          BLOSC_NUMTHREADS</td>
        <td>
          $(P)$(R)BloscNumThreads<br />
          $(P)$(R)BloscNumThreads_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td align="center" colspan="7,">
          <b>Parameters for Diagnostics</b> </td>
      </tr>
      <tr>
        <td>
          NDCodecCodecStatus</td>
        <td>
          asynInt32</td>
        <td>
          r/o</td>
        <td>
          Status of the compression/decompression. Values are "Success", "Warning", and "Error".
        </td>
        <td>
          CODEC_STATUS</td>
        <td>
          $(P)$(R)CodecStatus</td>
        <td>
          mbbi</td>
      </tr>
      <tr>
        <td>
          NDCodecCodecError</td>
        <td>
          asynOctet</td>
        <td>
          r/o</td>
        <td>
          Error message if CodecStatus is "Warning" or "Error". </td>
        <td>
          CODEC_ERROR</td>
        <td>
          $(P)$(R)CodecError</td>
        <td>
          waveform</td>
      </tr>
    </tbody>
  </table>

Configuration
-------------

The NDPluginCodec plugin is created with the following command, either
from C/C++ or from the EPICS IOC shell.

::

    int NDCodecConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                const char *NDArrayPort, int NDArrayAddr,
                                int maxBuffers, size_t maxMemory,
                                int priority, int stackSize)
     

For details on the meaning of the parameters to this function refer to
the detailed documentation on the NDCodecConfigure function in the
`NDPluginCodec.cpp
documentation <areaDetectorDoxygenHTML/_n_d_plugin_codec_8cpp.html>`__
and in the documentation for the constructor for the `NDPluginCodec
class <areaDetectorDoxygenHTML/class_n_d_plugin_codec.html>`__.

Screen shots
------------

The following is the MEDM screen that provides access to the parameters
in NDPluginDriver.h and NDPluginCodec.h through records in
NDPluginBase.template and NDCodec.template.

.. figure:: NDCodec.png
    :align: center

    NDCodec.adl

Performance
-----------

The following screens show the performance that can be achieved with
NDPluginCodec. For this test the simDetector driver was generating
1024x1024 UInt32 arrays at ~1280 arrays/s. These were compressed using
Blosc LZ4 compression with Bit shuffle and 6 Blosc threads. The
compression factor was ~42, so the output arrays were 98 KB, compared to
the input size of 4 MB. When running with a single plugin thread
(NumThreads=1) the plugin sometimes could not keep up. By increasing
numThreads to 2 the plugin could always process the full 1280 arrays/s
without dropping any arrays. The test was run on a 20-core Linux
machine, and the simDetector IOC was using ~7 cores. NDPluginCodec was
using ~6 of these. Since each array is 4 MB, this is a compression rate
of ~5.0 GB/s, or about 5 times the capacity of 10 Gbit Ethernet.

.. figure:: NDCodec_Performance.png
    :align: center

    NDCodec performance with ~1280 32-bit frames/s

.. figure:: NDCodec_Performance_More.png 
    :align: center

    NDPluginBaseFull.adl showing that NumThreads was set to 2

