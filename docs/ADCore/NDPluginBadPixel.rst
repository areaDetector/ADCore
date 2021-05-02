NDPluginBadPixel
================
:author: Mark Rivers, University of Chicago

.. contents:: Contents

Overview
--------

This plugin replaces bad pixels in an NDArray. It supports 3 different 
replacement mechanisms:
- Set:  The pixel value is set to a user defined value.
- Replace: The pixel value is replaced by the value in a nearby pixel.
- Median: The pixel value is replaced by the median value of nearby pixels.

The list of bad pixels in the detector is defined in a JSON file like this::

  { "Bad pixels" :
    [
      {"Pixel" : [200,100], "Set" : 0},
      {"Pixel" : [300,100], "Set" : 1000},
      {"Pixel" : [400,100], "Set" : 2000},
      {"Pixel" : [200,200], "Replace" : [1,0]},
      {"Pixel" : [300,200], "Replace" : [2,0]},
      {"Pixel" : [400,200], "Replace" : [1,1]},
      {"Pixel" : [200,300], "Median" : 1},
      {"Pixel" : [300,300], "Median" : 2},
      {"Pixel" : [400,300], "Median" : 3}
    ]
  }

- The value of the top-level "Bad pixels" key is an array of bad pixels.
- For each bad pixel in the array the value of the "Pixel" key is a 2-D array
  with the [X,Y] coordinates of the bad pixel.
- The other key for each pixel must be one of "Set", "Replace", or "Median".  
  - The value of the "Set" key is the replacement value for the bad pixel.
  - The value of the "Replace" key is a 2-D array containing the relative [X,Y] coordinates
  of the replacement pixel whose value will replace that of the bad pixel.
  [1,0] is the pixel to the right of the bad pixel, [1,0] is the pixel below the bad pixel.
  - The value of the "Median" key is the half-width size of the median filter, N.
  The actual size of the median filter is 2N+1, i.e. N=1 is a 3x3 filter and N=2 is a 5x5 filter.
  This ensures that the filter dimension is always odd and hence centered on the bad pixel.

The [X,Y] coordinates of the bad pixel are always defined in the raw detector coordinate system, i.e.
those in which the all of the following detector readout options are disabled:
- Reading a sub-region of the detector, rather than the entire detector.
- Binning in X or Y.
- Reversal in the X or Y dimensions.

If the detector is read out with any of these enabled then the plugin converts the raw
coordinates from the JSON file to the new coordinates being used.  This assumes that the detector driver
has correctly set the required metadata to the NDDimension structures in the NDArray.
This has been tested with ADSimDetector, but some drivers may need fixes to work correctly
if any of the above readout options are enabled.

The JSON file must defined to account for the following:
- A replacement pixel in "Replace" or "Median" mode should not use another bad pixel. The plugin does not check for this.
- The replacement pixel location in "Replace" mode must be a valid location, i.e. outside the NDArray bounds.'
The plugin does check for this, and if it is not a valid location the replacement is not performed.
- In "Median" mode the median will use fewer pixels if the bad pixel is too close to the border. 
For example with a medianSize=1 the plugin normally uses 8 pixels in calculating the median
(3x3 minus the bad pixel itself). 
A bad pixel on any edge of the image will only use 5 pixels to calculate the median, 
and a bad pixel in one of the 4 corners of the image will only use 3 pixels.

NDPluginBadPixel defines the following parameters.

.. cssclass:: table-bordered table-striped table-hover
.. flat-table::
  :header-rows: 2
  :widths: 5 5 5 70 5 5 5

  * -
    -
    - **Parameter Definitions in NDPluginBadPixel.h and EPICS Record Definitions in NDBadPixel.template**
  * - Parameter index variable
    - asyn interface
    - Access
    - Description
    - drvInfo string
    - EPICS record name
    - EPICS record type
  * - NDPluginBadPixelFileName
    - asynOctet
    - r/o
    - Name of the JSON bad pixel file.
    - BAD_PIXEL_FILE_NAME
    - $(P)$(R)FileName
    - waveform


Configuration
-------------

The NDPluginBadPixel plugin is created with the ``NDBadPixelConfigure`` command,
either from C/C++ or from the EPICS IOC shell.

::

   NDBadPixelConfigure (const char *portName, int queueSize, int blockingCallbacks,
                        const char *NDArrayPort, int NDArrayAddr, const char *pvName,
                        size_t maxMemory, int priority, int stackSize, int maxThreads)
     

For details on the meaning of the parameters to this function refer to
the detailed documentation on the NDBadPixelConfigure function in the
`NDPluginBadPixel.cpp
documentation <../areaDetectorDoxygenHTML/_n_d_plugin_bad_pixel_8cpp.html>`__ and
in the documentation for the constructor for the `NDPluginBadPixel
class <../areaDetectorDoxygenHTML/class_n_d_plugin_bad_pixel.html>`__.

The following MEDM screen for the NDPluginBadPixel. The only item not in the
base class screen is the name of the JSON bad pixel file.

.. image:: NDBadPixel.png
    :align: center
