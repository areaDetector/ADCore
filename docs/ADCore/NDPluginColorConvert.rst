NDPluginColorConvert
====================

:author: Mark Rivers, University of Chicago

.. contents:: Contents

Overview
--------

NDPluginColorConvert is a tool for converting the color mode of NDArray
data. It receives an input NDArray with one color mode and outputs
another NDArray with a (potentially) different color mode. All other
attributes of the array are preserved.

NDPluginColorConvert inherits from NDPluginDriver. The
`NDPluginColorConvert class
documentation <../areaDetectorDoxygenHTML/class_n_d_plugin_color_convert.html>`__
describes this class in detail.

NDPluginColorConvert defines the following parameters. It also
implements all of the standard plugin parameters from
:doc:`NDPluginDriver`. The EPICS database
NDColorConvert.template provides access to these parameters, listed in
the following table.

.. raw:: html

  <table class="table table-bordered">
    <tbody>
      <tr>
        <td align="center" colspan="7,">
          <b>Parameter Definitions in NDPluginColorConvert.h and EPICS Record Definitions in
            NDColorConvert.template</b></td>
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
          NDPluginColorConvertColorModeOut</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          The output color mode (NDColorMode_t).</td>
        <td>
          COLOR_MODE_OUT</td>
        <td>
          $(P)$(R)ColorModeOut
          <br />
          $(P)$(R)ColorModeOut_RBV </td>
        <td>
          mbbo
          <br />
          mbbi</td>
      </tr>
      <tr>
        <td>
          NDPluginColorConvertFalseColor</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          The false color map index. There are currently 2 false color maps defined, Rainbow
          and Iron. Additional color maps can easily be added in the future.</td>
        <td>
          FALSE_COLOR</td>
        <td>
          $(P)$(R)FalseColor
          <br />
          $(P)$(R)FalseColor_RBV </td>
        <td>
          mbbo
          <br />
          mbbi</td>
      </tr>
    </tbody>
  </table>

When converting from 8-bit mono to RGB1, RGB2 or RGB3 a false-color map
will be applied if FalseColor is not zero.

The Bayer color conversion supports the 4 Bayer formats (NDBayerRGGB,
NDBayerGBRG, NDBayerGRBG, NDBayerBGGR) defined in ``NDArray.h``. If the
input color mode and output color mode are not one of these supported
conversion combinations then the output array is simply a copy of the
input array and no conversion is performed.

Configuration
-------------

The NDPluginColorConvert plugin is created with the following command,
either from C/C++ or from the EPICS IOC shell.

::

    int NDColorConvertConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                                const char *NDArrayPort, int NDArrayAddr, 
                                int maxBuffers, size_t maxMemory,
                                int priority, int stackSize)
     

For details on the meaning of the parameters to this function refer to
the detailed documentation on the NDColorConvertConfigure function in
the `NDPluginColorConvert.cpp
documentation <../areaDetectorDoxygenHTML/_n_d_plugin_color_convert_8cpp.html>`__
and in the documentation for the constructor for the
`NDPluginColorConvert
class <../areaDetectorDoxygenHTML/class_n_d_plugin_color_convert.html>`__.

Screen shots
------------

The following is the MEDM screen that provides access to the parameters
in ``NDPluginDriver.h`` and ``NDPluginColorConvert.h`` through records in
``NDPluginBase.template`` and ``NDColorConvert.template``.

.. image:: NDColorConvert.png
    :align: center

Restrictions
------------

-  The Bayer color conversion is done using a library function provided
   in the Prosilica library. The source code for this function is not
   provided, and the binaries are only available on Linux and Windows.
   All other conversions are supported on all platforms.
-  YUV color conversion is not supported. This may be added in a future
   release.


