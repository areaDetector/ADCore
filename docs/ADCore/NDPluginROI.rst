NDPluginROI

:author: Mark Rivers, University of Chicago

.. contents:: Contents

Overview
--------

NDPluginROI selects a rectangular "Region-Of-Interest" (ROI) from the
NDArray callback data. The ROI can be any size, from a single array
element to the entire array. NDPluginROI  does the following operations,
in this order:

#. Extracts the ROI as defined by its offset (starting array element)
   and size in each dimension.
#. Optional binning in any dimension.
#. Optional orientation reversal (mirroring) in any dimension.
#. Optional scaling (dividing) by a scale factor.
#. Optional conversion to a new data type.
#. Optional collapsing (removing) of dimensions whose value is 1.
#. Export the ROI as a new NDArray object. The NDPluginROI is both a
   **recipient** of callbacks and a **source** of NDArray callbacks, as
   a driver is. This means that other plugins like NDPluginStdArrays and
   NDPluginFile can be connected to an NDPluginROI plugin, in which case
   they will display or save the selected ROI rather than the full
   detector driver data.

If scaling is enabled then the array is promoted to a double when it is
extracted and binned. The scaling is done on this double-precision
array, and then the array is converted back to the desired output data
type. This makes scaling relatively computationally intensive, but
ensures that correct results are obtained, without integer truncation
problems.

Note that while the NDPluginROI should be N-dimensional, the EPICS
interface to the definition of the ROI is currently limited to a maximum
of 3-D. This limitation may be removed in a future release.

NDPluginROI inherits from NDPluginDriver. The `NDPluginROI class
documentation <../areaDetectorDoxygenHTML/class_n_d_plugin_r_o_i.html>`__
describes this class in detail.

NDPluginROI.h defines the following parameters. It also implements all
of the standard plugin parameters from
`NDPluginDriver <pluginDoc.html#NDPluginDriver>`__. The EPICS database
NDROI.template provide access to these parameters, listed in the
following table. Note that to reduce the width of this table the
parameter index variable names have been split into 2 lines, but these
are just a single name, for example ``NDPluginROIName``.

.. raw:: html

  <table border="1" cellpadding="2" cellspacing="2" style="text-align: left">
    <tbody>
      <tr>
        <td align="center" colspan="7,">
          <b>Parameter Definitions in NDPluginROI.h and EPICS Record Definitions in NDROI.template</b>
        </td>
      </tr>
      <tr>
        <th>
          Parameter index variable</th>
        <th>
          asyn interface</th>
        <th>
          Access</th>
        <th>
          Description</th>
        <th>
          drvInfo string</th>
        <th>
          EPICS record name</th>
        <th>
          EPICS record type</th>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Name</td>
        <td>
          asynOctet</td>
        <td>
          r/w</td>
        <td>
          Name of this ROI</td>
        <td>
          NAME</td>
        <td>
          $(P)$(R)Name<br />
          $(P)$(R)Name_RBV</td>
        <td>
          stringout<br />
          stringin</td>
      </tr>
      <tr>
        <td align="center" colspan="7,">
          <b>ROI definition</b></td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim0Enable</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Enable ROI calculations in the X dimension. If not enabled then the start, size,
          binning, and reverse operations are disabled in the X dimension, and the values
          from the input array are used.</td>
        <td>
          DIM0_ENABLE</td>
        <td>
          $(P)$(R)EnableX<br />
          $(P)$(R)EnableX_RBV</td>
        <td>
          bo<br />
          bi</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim1Enable</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Enable ROI calculations in the Y dimension. If not enabled then the start, size,
          binning, and reverse operations are disabled in the Y dimension, and the values
          from the input array are used.</td>
        <td>
          DIM1_ENABLE</td>
        <td>
          $(P)$(R)EnableY<br />
          $(P)$(R)EnableY_RBV</td>
        <td>
          bo<br />
          bi</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim2Enable</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Enable ROI calculations in the Z dimension. If not enabled then the start, size,
          binning, and reverse operations are disabled in the Z dimension, and the values
          from the input array are used.</td>
        <td>
          DIM2_ENABLE</td>
        <td>
          $(P)$(R)EnableZ<br />
          $(P)$(R)EnableZ_RBV</td>
        <td>
          bo<br />
          bi</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim0Bin</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Binning in the X dimension</td>
        <td>
          DIM0_BIN</td>
        <td>
          $(P)$(R)BinX<br />
          $(P)$(R)BinX_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim1Bin</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Binning in the Y dimension</td>
        <td>
          DIM1_BIN</td>
        <td>
          $(P)$(R)BinY<br />
          $(P)$(R)BinY_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim2Bin</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Binning in the Z dimension</td>
        <td>
          DIM2_BIN</td>
        <td>
          $(P)$(R)BinZ<br />
          $(P)$(R)BinZ_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim0Min</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          First pixel in the ROI in the X dimension. 0 is the first pixel in the array.</td>
        <td>
          DIM0_MIN</td>
        <td>
          $(P)$(R)MinX<br />
          $(P)$(R)MinX_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim1Min</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          First pixel in the ROI in the Y dimension.<br />
          0 is the first pixel in the array.</td>
        <td>
          DIM1_MIN</td>
        <td>
          $(P)$(R)MinY<br />
          $(P)$(R)MinY_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim2Min</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          First pixel in the ROI in the Z dimension.<br />
          0 is the first pixel in the array.</td>
        <td>
          DIM2_MIN</td>
        <td>
          $(P)$(R)MinZ<br />
          $(P)$(R)MinZ_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim0Size</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Size of the ROI in the X dimension</td>
        <td>
          DIM0_SIZE</td>
        <td>
          $(P)$(R)SizeX<br />
          $(P)$(R)SizeX_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim1Size</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Size of the ROI in the Y dimension</td>
        <td>
          DIM1_SIZE</td>
        <td>
          $(P)$(R)SizeY<br />
          $(P)$(R)SizeY_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim2Size</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Size of the ROI in the Z dimension</td>
        <td>
          DIM2_SIZE</td>
        <td>
          $(P)$(R)SizeZ<br />
          $(P)$(R)SizeZ_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim0AutoSize</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Automatically set SizeX to the input array size minus MinX</td>
        <td>
          DIM0_AUTO_SIZE</td>
        <td>
          $(P)$(R)AutoSizeX<br />
          $(P)$(R)AutoSizeX_RBV</td>
        <td>
          bo<br />
          bi</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim1AutoSize</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Automatically set SizeY to the input array size minus MinY</td>
        <td>
          DIM1_AUTO_SIZE</td>
        <td>
          $(P)$(R)AutoSizeY<br />
          $(P)$(R)AutoSizeY_RBV</td>
        <td>
          bo<br />
          bi</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim2AutoSize</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Automatically set SizeZ to the input array size minus MinZ</td>
        <td>
          DIM2_AUTO_SIZE</td>
        <td>
          $(P)$(R)AutoSizeZ<br />
          $(P)$(R)AutoSizeZ_RBV</td>
        <td>
          bo<br />
          bi</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim0MaxSize</td>
        <td>
          asynInt32</td>
        <td>
          r/o</td>
        <td>
          Maximum size of the ROI in the X dimension</td>
        <td>
          DIM0_MAX_SIZE</td>
        <td>
          $(P)$(R)MaxSizeX_RBV</td>
        <td>
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim1MaxSize</td>
        <td>
          asynInt32</td>
        <td>
          r/o</td>
        <td>
          Maximum size of the ROI in the Y dimension</td>
        <td>
          DIM1_MAX_SIZE</td>
        <td>
          $(P)$(R)MaxSizeY_RBV</td>
        <td>
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim2MaxSize</td>
        <td>
          asynInt32</td>
        <td>
          r/o</td>
        <td>
          Maximum size of the ROI in the Z dimension</td>
        <td>
          DIM2_MAX_SIZE</td>
        <td>
          $(P)$(R)MaxSizeZ_RBV</td>
        <td>
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim0Reverse</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Reverse ROI in the X dimension. (0=No, 1=Yes)</td>
        <td>
          DIM0_REVERSE</td>
        <td>
          $(P)$(R)ReverseX<br />
          $(P)$(R)ReverseX_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim1Reverse</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Reverse ROI in the Y dimension. (0=No, 1=Yes)</td>
        <td>
          DIM1_REVERSE</td>
        <td>
          $(P)$(R)ReverseY<br />
          $(P)$(R)ReverseY_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Dim2Reverse</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Reverse ROI in the Z dimension. (0=No, 1=Yes)</td>
        <td>
          DIM2_REVERSE</td>
        <td>
          $(P)$(R)ReverseZ<br />
          $(P)$(R)ReverseZ_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          DataType</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Data type of the ROI (NDDataType_t). This can be different from the data type of
          the NDArray callback data.</td>
        <td>
          ROI_DATA_TYPE</td>
        <td>
          $(P)$(R)DataType<br />
          $(P)$(R)DataType_RBV</td>
        <td>
          mbbo<br />
          mbbi</td>
      </tr>
      <tr>
        <td>
          NDArraySizeX</td>
        <td>
          asynInt32</td>
        <td>
          r/o</td>
        <td>
          Size of the ROI data in the X dimension</td>
        <td>
          ARRAY_SIZE_X</td>
        <td>
          $(P)$(R)ArraySizeX_RBV</td>
        <td>
          longin</td>
      </tr>
      <tr>
        <td>
          NDArraySizeY</td>
        <td>
          asynInt32</td>
        <td>
          r/o</td>
        <td>
          Size of the ROI data in the Y dimension</td>
        <td>
          ARRAY_SIZE_Y</td>
        <td>
          $(P)$(R)ArraySizeY_RBV</td>
        <td>
          longin</td>
      </tr>
      <tr>
        <td>
          NDArraySizeZ</td>
        <td>
          asynInt32</td>
        <td>
          r/o</td>
        <td>
          Size of the ROI data in the Z dimension</td>
        <td>
          ARRAY_SIZE_Z</td>
        <td>
          $(P)$(R)ArraySizeZ_RBV</td>
        <td>
          longin</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          EnableScale</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Enable dividing by the Scale value. (0=Disable, 1=Enable). This is very useful when
          binning or when converting from a higher precision data type to a lower precision
          data type. For example when binning 2x2, then Scale=4 (dividing by 4) will prevent
          integer overflow. Similarly, when converting from 16-bit to 8-bit integers one might
          scale by 256, or perhaps a smaller number if the 16-bit data does not use the full
          16-bit range.</td>
        <td>
          ENABLE_SCALE</td>
        <td>
          $(P)$(R)EnableScale<br />
          $(P)$(R)EnableScale_RBV</td>
        <td>
          bo<br />
          bi</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          Scale</td>
        <td>
          asynFloat64</td>
        <td>
          r/w</td>
        <td>
          The scale value to divide by if EnableScale is enabled.</td>
        <td>
          SCALE_VALUE</td>
        <td>
          $(P)$(R)Scale<br />
          $(P)$(R)Scale_RBV</td>
        <td>
          ao<br />
          ai</td>
      </tr>
      <tr>
        <td>
          NDPluginROI<br />
          CollapseDims</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Collapse (remove) output array dimensions whose value is 1. Consider the case when
          the input array to the ROI plugin has dimensions [4, 256, 256] and the plugin is
          configured with MinX=1, SizeX=1, MinY=0, SizeY=256, MinZ=0, SizeZ=256. If CollapseDims=Disable
          then the output arrays will be of size [1, 256, 256]. If CollapseDims=Enable then
          the output array will be [256, 256]. This is convenient for some purposes. For example
          file plugins like JPEG or TIFF will not work with arrays of [1, 256, 256], nor will
          the ImageJ display plugin. They all require that the array have only 2 dimensions
          unless it is the special case of 3-D color arrays (RGB1, RGB2, RGB3) where one of
          the dimensons is 3.</td>
        <td>
          COLLAPSE_DIMS</td>
        <td>
          $(P)$(R)CollapseDims<br />
          $(P)$(R)CollapseDims_RBV</td>
        <td>
          bo<br />
          bi</td>
      </tr>
    </tbody>
  </table>


A special case is made when the NDArray data has
``colorMode=NDColorModeRGB1`` or ``NDColorModeRGB2``. In these cases the user
interface to the array dimensions is changed so that the Z PVs always
refer to the color dimension (as for ``NDColorModeRGB3``), the X dimension
refers to the horizontal dimension, and the Y dimension refers to the
vertical dimension. This is very convenient, because it means that the
ROI does not need to redefined if, for example, the color mode is
changed from Mono to RGB1, which would be required if the X, Y and Z
dimensions were not automatically switched.

Configuration
-------------

The NDPluginROI plugin is created with the ``NDROIConfigure`` command,
either from C/C++ or from the EPICS IOC shell.

::

   NDROIConfigure(const char *portName, int queueSize, int blockingCallbacks,
                  const char *NDArrayPort, int NDArrayAddr,
                  int maxBuffers, size_t maxMemory,
                  int priority, int stackSize, int maxThreads)
     

For details on the meaning of the parameters to this function refer to
the detailed documentation on the ``NDROIConfigure`` function in the
`NDPluginROI.cpp
documentation <../areaDetectorDoxygenHTML/_n_d_plugin_r_o_i_8cpp.html>`__
and in the documentation for the constructor for the `NDPluginROI
class <../areaDetectorDoxygenHTML/class_n_d_plugin_r_o_i.html>`__.

Screen shots
------------

The following MEDM screen provides access to the parameters in
``NDPluginDriver.h`` and ``NDPluginROI.h`` through records in
``NDPluginBase.template`` and ``NDROI.template``.

.. image:: NDROI.png
    :align: center


The following MEDM screen provides access to 4 ROIs at once.

.. image:: NDROI4.png

