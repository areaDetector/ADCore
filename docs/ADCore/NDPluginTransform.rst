NDPluginTransform
=================
:authors: Christian Roehrig, Mark Rivers, John Hammonds
:affiliation: Argonne National Laboratory

.. contents:: Contents

Overview
--------

This plugin provides 8 choices for image transforms that involve
rotations by multiples of 90 degrees and mirror reflections about the
central vertical line of the image. The plugin supports only 2-D
monochrome and color images (RGB1, RGB2, and RGB3).

NDPluginTransform inherits from NDPluginDriver. The `NDPluginTransform
class
documentation <../areaDetectorDoxygenHTML/class_n_d_plugin_transform.html>`__
describes this class in detail.

``NDPluginTransform.h`` defines the following parameters. It also implements
all of the standard plugin parameters from
:doc:`NDPluginDriver`. The EPICS database
``NDransform.template`` provide access to these parameters, listed in the
following table. Note that to reduce the width of this table the
parameter index variable names have been split into 2 lines, but these
are just a single name, for example ``NDPluginTransformType``.

.. raw:: html


  <table class="table table-bordered">
    <tbody>
      <tr>
        <td align="center" colspan="7,">
          <b>Parameter Definitions in NDPluginTransform.h and EPICS Record Definitions in NDTransform.template</b>
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
          NDPluginTransform<br />
          Type</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Type of transform</td>
        <td>
          TRANSFORM_TYPE</td>
        <td>
          $(P)$(R)Type. Choices are:
          <ul>
            <li>None: No transform, the output image is the same as the input image.</li>
            <li>Rot90: Rotate clockwise 90 degrees.</li>
            <li>Rot180: Rotate clockwise 180 degrees.</li>
            <li>Rot270: Rotate clockwise 270 degrees; equivalent to counter-clockwise rotation
              by 90 degrees.</li>
            <li>Mirror: Mirror reflection about the central column in the image.</li>
            <li>Rot90Mirror: Rot90 followed by Mirror. Equivalent to image transpose, swapping
              rows and columns.</li>
            <li>Rot180Mirror: Rot180 followed by Mirror. Equivalent to a mirror reflection about
              the central row in the image.</li>
            <li>Rot270Mirror: Rot270 followed by Mirror. Equivalent to image transpose followed
              by mirror reflection about the central column in the image.</li>
          </ul>
        </td>
        <td>
          mbbo</td>
      </tr>
    </tbody>
  </table>


Configuration
-------------

The NDPluginTransform plugin is created with the ``NDTransformConfigure``
command, either from C/C++ or from the EPICS IOC shell.

::

   NDTransformConfigure(const char *portName, int queueSize, int blockingCallbacks,
                  const char *NDArrayPort, int NDArrayAddr,
                  int maxBuffers, size_t maxMemory,
                  int priority, int stackSize)
     

For details on the meaning of the parameters to this function refer to
the detailed documentation on the NDTransformConfigure function in the
`NDPluginROI.cpp
documentation <../areaDetectorDoxygenHTML/_n_d_plugin_transform_8cpp.html>`__
and in the documentation for the constructor for the `NDPluginTransform
class <../areaDetectorDoxygenHTML/class_n_d_plugin_transform.html>`__.

Screen shots
------------

The following MEDM screen provides access to the parameters in
``NDPluginDriver.h`` and ``NDPluginTransform.h`` through records in
``NDPluginBase.template`` and ``NDTransform.template``. The orientation of the
letter F on the screen shows what each transform type does.

.. figure:: NDTransform.png
    :align: center

Performance
-----------

The following is a measurement of the performance of the
NDPluginTransform plugin in release R2-1. The measurements were done
with the simDetector on an 8-core Linux machine. All plugins except the
NDPluginTransform plugin were disabled. The simDetector was generating
about 680 frames/s in mono mode and about 190 frames/s in RGB1 mode. The
plugin was thus always dropping frames except when the transformation
was None in mono and RGB1 mode, and when the transformation was
Rot180Mirror in mono mode.

.. raw:: html

  <table class="table table-bordered">
    <tbody>
      <tr>
        <td align="center" colspan="4">
          <b>Performance (frames/s)</b> </td>
      </tr>
      <tr>
        <th>
          Dimensions</th>
        <th>
          Transformation</th>
        <th>
          8-bit Mono</th>
        <th>
          8-bit RGB1</th>
      </tr>
      <tr>
        <td>
          1024 x 1024</td>
        <td>
          None</td>
        <td>
          680</td>
        <td>
          190</td>
      </tr>
      <tr>
        <td>
          1024 x 1024</td>
        <td>
          Rot90</td>
        <td>
          115</td>
        <td>
          40</td>
      </tr>
      <tr>
        <td>
          1024 x 1024</td>
        <td>
          Rot180</td>
        <td>
          145</td>
        <td>
          52</td>
      </tr>
      <tr>
        <td>
          1024 x 1024</td>
        <td>
          Rot270</td>
        <td>
          105</td>
        <td>
          41</td>
      </tr>
      <tr>
        <td>
          1024 x 1024</td>
        <td>
          Mirror</td>
        <td>
          152</td>
        <td>
          56</td>
      </tr>
      <tr>
        <td>
          1024 x 1024</td>
        <td>
          Rot90Mirror</td>
        <td>
          116</td>
        <td>
          40</td>
      </tr>
      <tr>
        <td>
          1024 x 1024</td>
        <td>
          Rot180Mirror</td>
        <td>
          680</td>
        <td>
          75</td>
      </tr>
      <tr>
        <td>
          1024 x 1024</td>
        <td>
          Rot270Mirror</td>
        <td>
          111</td>
        <td>
          41</td>
      </tr>
    </tbody>
  </table>


Note that this performance with ADCore R2-1 and later is dramatically
improved from R2-0 and earlier. For example, in R2-0 with 1024 x 1024
8-bit mono images the frame rate for all transformations (including
None) was only 8 frames/s. With 8-bit RGB1 the frame rate for all
transformations was only 3 frames/s. Thus, R2-1 improves the performance
by a factor of 13-85 compared to previous versions.

