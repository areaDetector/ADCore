NDPluginFFT
===========
:author: Mark Rivers, University of Chicago

.. contents:: Contents

Overview
--------

This plugin computes 1-D or 2-D Fast Fourier Transforms (FFTs). It
exports 1-D or 2-D NDArrays containing the absolute value of the FFT. It
creates 1-D waveform records of the input, and the real, imaginary, and
absolute values of the first row of the FFT. It also creates 1-D
waveform records of the time and frequency axes, which are useful for
plotting if the 1-D input represents a time-series. The plugin
optionally does recursive averaging of the computed FFTs to increase the
signal to noise.

The FFT algorithm used requires that the input array dimensions be a
power of 2. The plugin will pad the array to the next larger power of 2
if the input array does not meet this requirement.

.. todo:: Fix links

The `ADCSimDetector <ADCSimDetectorDoc.html>`__ application simulates an
8-channel ADC with different waveforms. This application is useful for
testing and demonstrating the NDPluginFFT plugin with 1-D NDArray input.

The `simDetector <simDetectorDoc.html>`__ application has a Sine
simulation mode that generates images based on the sums and/or products
of 4 sine waves. This can be used to generate images with well-defined
frequency components in X and Y to test and demonstrate the NDPluginFFT
plugin, as shown in the images below. The Peaks simulation mode can also
be used to generate interesting frequency patterns, as shown in the
images below. This application is thus useful for testing and
demonstrating the NDPluginFFT plugin with 2-D NDArray input.

For 1-D FFTs the plugin exports a 1-D array containing the frequency
values for each point. In order to construct this the plugin requires
knowing the time interval between samples (TimePerPoint). This
information normally comes from a database link to a record in the
detector driver, but it can be manually specified as well.

NDPluginFFT inherits from NDPluginDriver. The `NDPluginFFT class
documentation <../areaDetectorDoxygenHTML/class_n_d_plugin_f_f_t.html>`__
describes this class in detail.

NDPluginFFT defines the following parameters. It also implements all of
the standard plugin parameters from
:doc:`NDPluginDriver`. The template files
listed above provide access to these parameters, listed in the following
tables.

.. raw:: html

  <table class="table table-bordered">
    <tbody>
      <tr>
        <td align="center" colspan="7">
          <b>Parameters for entire plugin.
            <br />
            Parameter Definitions in NDPluginFFT.h and EPICS Record Definitions in NDFFT.template</b>
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
          FFTTimePerPoint</td>
        <td>
          asynFloat64</td>
        <td>
          r/w</td>
        <td>
          The time interval between samples in the waveforms from the driver. This value is
          normally updated automatically using the FFTTimePerPointLink record described below.
          It can also be manually changed if there is no EPICS record available to provide
          this value automatically.</td>
        <td>
          FFT_TIME_PER_POINT</td>
        <td>
          $(P)$(R)FFTTimePerPoint<br />
          $(P)$(R)FFTTimePerPoint_RBV</td>
        <td>
          ao<br />
          ai</td>
      </tr>
      <tr>
        <td>
          N.A.</td>
        <td>
          N.A.</td>
        <td>
          r/w</td>
        <td>
          This record has OMSL="closed_loop" and DOL set to an record that contains the time
          between points from the driver. The link will normally have the CP attribute, so
          this record processes whenever the input record changes. The OUT field of this record
          is FFTTimePerPoint.</td>
        <td>
          N.A.</td>
        <td>
          $(P)$(R)FFTTimePerPointLink</td>
        <td>
          ao</td>
      </tr>
      <tr>
        <td>
          FFTTimeAxis</td>
        <td>
          asynFloat64ArrayIn</td>
        <td>
          r/o</td>
        <td>
          A waveform record containing the time value of each point in the TimeSeries waveforms.
          FFTTimeAxis[i] = FFTTimePerPoint * i. Note that this record is useful for 1-D FFTs
          where the input array is a time-series and the TimePerPoint value is correctly set.
        </td>
        <td>
          FFT_TIME_AXIS</td>
        <td>
          $(P)$(R)FFTTimeAxis</td>
        <td>
          waveform</td>
      </tr>
      <tr>
        <td>
          FFTFreqAxis</td>
        <td>
          asynFloat64ArrayIn</td>
        <td>
          r/o</td>
        <td>
          A waveform record containing the frequency value of each point in the FFT waveform
          records. FFTFreqAxis[i] = FrequencyStep * i, where FrequencyStep is controlled by
          TimePerPoint and the number of time points in the input array. Note that this record
          is useful for 1-D FFTs where the input array is a time-series and the TimePerPoint
          value is correctly set.</td>
        <td>
          FFT_FREQ_AXIS</td>
        <td>
          $(P)$(R)FFTFreqAxis</td>
        <td>
          waveform</td>
      </tr>
      <tr>
        <td>
          FFTDirection</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          The FFT direction. Choices are:<br />
          0: Time to frequency<br />
          1: Frequency to time<br />
          NOTE: This is not yet implemented because frequency to time requires complex data,
          and complex data is not yet supported in areaDetector. Currently only Time to frequency
          is supported, and the frequency output consists of float64 arrays containing the
          real part, imaginary part, and absolute value of the FFT.</td>
        <td>
          FFT_DIRECTION</td>
        <td>
          $(P)$(R)FFTDirection<br />
          $(P)$(R)FFTDirection_RBV</td>
        <td>
          bo<br />
          bi</td>
      </tr>
      <tr>
        <td>
          FFTSuppressDC</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          Option to set the zero frequency (DC) value in the output arrays to 0. Choices are:<br />
          0: Disable<br />
          1: Enable<br />
          If the signal has a large DC offset then setting the zero frequency component to
          0 in the output arrays can make plots look better. This is because the DC component
          can be much larger than all other frequency components.</td>
        <td>
          FFT_SUPPRESS_DC</td>
        <td>
          $(P)$(R)FFTSuppressDC<br />
          $(P)$(R)FFTSuppressDC_RBV</td>
        <td>
          bo<br />
          bi</td>
      </tr>
      <tr>
        <td>
          FFTNumAverage</td>
        <td>
          asynInt32</td>
        <td>
          r/w</td>
        <td>
          The characteristic number of FFTs in the recursive average. The equations for the
          FFT averaging are:<br />
          <code>Out = ((1-1/NumAveraged) * Old) + (1/NumAveraged * New)<br />
            Old = Out
            <br />
            if (NumAveraged < NumAverage) NumAveraged = NumAveraged + 1
            <br />
          </code>when Old is the previous output and New is the latest FFT calculation. If
          NumAverage=1 then there is no averaging. </td>
        <td>
          FFT_NUM_AVERAGE</td>
        <td>
          $(P)$(R)FFTNumAverage<br />
          $(P)$(R)FFTNumAverage_RBV</td>
        <td>
          longout<br />
          longin</td>
      </tr>
      <tr>
        <td>
          FFTNumAveraged</td>
        <td>
          asynInt32</td>
        <td>
          r/o</td>
        <td>
          The number of FFTs averaged in the current output. This value increases until it
          reaches the value of NumAverage. See the equations for NumAverage above. </td>
        <td>
          FFT_NUM_AVERAGED</td>
        <td>
          $(P)$(R)FFTNumAveraged</td>
        <td>
          longin</td>
      </tr>
      <tr>
        <td>
          FFTResetAverage</td>
        <td>
          asynInt32</td>
        <td>
          r/o</td>
        <td>
          Setting this record to 1 resets NumAveraged to 0, so the averaging process starts
          over. </td>
        <td>
          FFT_RESET_AVERAGE</td>
        <td>
          $(P)$(R)FFTResetAverage</td>
        <td>
          bo</td>
      </tr>
      <tr>
        <td>
          N.A.</td>
        <td>
          N.A.</td>
        <td>
          r/w</td>
        <td>
          The name for this signal.</td>
        <td>
          N.A.</td>
        <td>
          $(P)$(R)SignalName</td>
        <td>
          stringout</td>
      </tr>
      <tr>
        <td>
          FFTTimeSeries</td>
        <td>
          asynFloat64ArrayIn</td>
        <td>
          r/o</td>
        <td>
          The time series data array. </td>
        <td>
          FFT_TIME_SERIES </td>
        <td>
          $(P)$(R)TimeSeries</td>
        <td>
          waveform</td>
      </tr>
      <tr>
        <td>
          FFTReal</td>
        <td>
          asynFloat64ArrayIn</td>
        <td>
          r/o</td>
        <td>
          The real part of the FFT.
          <br />
          NOTE: this value is only available as a 1-D waveform. It is not exported as an NDArray.
          For 2-D FFTs it contains only the first row of the FFT.</td>
        <td>
          FFT_FFT_REAL </td>
        <td>
          $(P)$(R)FFTReal</td>
        <td>
          waveform</td>
      </tr>
      <tr>
        <td>
          FFTImaginary</td>
        <td>
          asynFloat64ArrayIn</td>
        <td>
          r/o</td>
        <td>
          The imaginary part of the FFT.
          <br />
          NOTE: this value is only available as a 1-D waveform. It is not exported as an NDArray.
          For 2-D FFTs it contains only the first row of the FFT.</td>
        <td>
          FFT_FFT_IMAGINARY </td>
        <td>
          $(P)$(R)FFTImaginary</td>
        <td>
          waveform</td>
      </tr>
      <tr>
        <td>
          FFTAbsValue</td>
        <td>
          asynFloat64ArrayIn</td>
        <td>
          r/o</td>
        <td>
          The absolute value of the FFT.
          <br />
          NOTE: this is exported as an NDArray, either 1-D or 2-D depending on the rank of
          the input NDArray. However, for 2-D arrays the waveform record contains only the
          first row of the FFT.</td>
        <td>
          FFT_ABS_VALUE</td>
        <td>
          $(P)$(R)FFTAbsValue</td>
        <td>
          waveform</td>
      </tr>
    </tbody>
  </table>

Configuration
-------------

The NDPluginFFT plugin is created with the NDFFTConfigure function,
either from C/C++ or from the EPICS IOC shell.

::

   NDFFTConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers,  size_t maxMemory, int priority, int stackSize)
     

All of theses parameters are from the NDPluginDriver base class.

For example:

::

    NDFFTConfigure("FFT1", 20, 0, "TS1", 0, 0, 0, 0, 0)

Screen shots
------------

The following MEDM screen provides access to the parameters in
``NDPluginDriver.h`` and ``NDPluginFFT.h`` through records in
``NDPluginBase.template``, and ``NDTimeSeries.template``.

.. image:: NDPluginFFT.png
    :align: center

The following MEDM screen provides access to NDPluginFFT.adl and the
plot screens show below for up to 16 signals.

.. image:: NDFFT16.png
    :align: center

The following MEDM screens show the time series and FFT plots for arrays
in NDFFT.template and NDPluginTimeSeriesN.template. These 1-D
time-series are produced using the ADCSimDetector driver in ADExample.

.. figure:: NDFFTTimeSeriesPlot.png
    :align: center

    Time-series plot. This is the Sawtooth waveform with 10 Hz frequency.

.. figure:: NDFFTPlotAbsValue.png
    :align: center

    Absolute value of FFT. Note that the FFT of a sawtooth has
    the 10 Hz fundamental frequency plus all even and odd harmonics.

.. figure:: NDFFTTimeSeriesPlotNoisy.png
    :align: center

    Time-series plot. This is the Sawtooth waveform with 10 Hz
    frequency with addition of noise.

.. figure:: NDFFTTimeSeriesPlotFFTAbsValueNoisyAvg1.png
    :align: center

    Absolute value of FFT of noisy waveform above with
    ``NumAverage=1`` (no averaging). Only the first peak in the power
    spectrum is clearly visible.

.. figure:: NDFFTTimeSeriesPlotFFTAbsValueNoisyAvg100.png
    :align: center

    Absolute value of FFT of noisy waveform above with
    NumAverage=100. The first 7 peaks in the power spectrum are now
    clearly visible.

.. figure:: NDFFTPlotAll.png
    :align: center

    Combined plot with time-series, FFT absolute value, FFT
    real, and FFT imaginary. This is the sin(x)*cos(x) waveform, with the
    sine frequency=20 Hz and the cosine frequency=1 Hz.

The following MEDM screens show the real-space images and 2-D FFT plots
for arrays in ``NDFFT.template`` and ``NDPluginTimeSeriesN.template``. These 2-D
images are produced using the simDetector driver in ADExample using the
settings show in the screen shot below.

.. figure:: NDFFTSimDetectorSetup.png
    :align: center

    Setup screen for the simDetector driver showing the values
    that were used to generate the images and FFTs shown below.

.. figure:: NDFFTPeaksImage.png
    :align: center

    Real space image. This is generated using the Peaks mode in
    the simDetector driver.

.. figure:: NDFFTPeaksAbsVal.png
    :align: center

    2-D FFT absolute value of the above image.
    
.. figure:: NDFFTSineImage.png
    :align: center

    Real space image. This is generated using the Sine mode in
    the simDetector driver.

.. figure:: NDFFTSineAbsVal.png
    :align: center

    2-D FFT absolute value of the above image.
   
Note in the above image that the two X freqencies are 2 and 50 Hz, which
are the values in the SimDetectorSetup screen shown above, because these
two sine waves are added together. Note, however, that the two Y
frequencies, 1 Hz and 20 Hz do not appear in the FFT because these two
sine waves are multiplied together. The frequencies in the FFT are the
sum and difference of sine wave frequencies, 19 and 21 Hz.

The FFT images above were captured by setting the NDStdArrays plugin to
get its data from the NDPluginFFT plugin, which is port FFT1 in the
simDetector example IOC in the ADExample module. The ImageJ viewer was
then used to view the EPICS waveform record from the NDStdArrays plugin.

