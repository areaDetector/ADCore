ADDriver
~~~~~~~~

ADDriver inherits from asynNDArrayDriver. This is the class from which
area detector drivers are directly derived. It provides parameters and
methods that are specific to area detectors, while asynNDArrayDriver is
a general NDArray driver. The `ADDriver class
documentation <areaDetectorDoxygenHTML/class_a_d_driver.html>`__\ describes
this class in detail.

The file `ADDriver.h <areaDetectorDoxygenHTML/_a_d_driver_8h.html>`__
defines the parameters that all areaDetector drivers should implement if
possible.

.. raw:: html

  <table border="1" cellpadding="2" cellspacing="2" style="text-align: left">
    <tbody>
      <tr>
        <td align="center" colspan="7">
          <b>Parameter Definitions in ADDriver.h and EPICS Record Definitions in ADBase.template</b>
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
        <td align="center" colspan="7" style="height: 25px">
          <b>Information about the detector</b> </td>
      </tr>
      <tr>
        <td class="style1">
          ADManufacturer </td>
        <td class="style1">
          asynOctet </td>
        <td class="style1">
          r/o </td>
        <td class="style1">
          Detector manufacturer name </td>
        <td class="style1">
          MANUFACTURER </td>
        <td class="style1">
          $(P)$(R)Manufacturer_RBV </td>
        <td class="style1">
          stringin </td>
      </tr>
      <tr>
        <td>
          ADModel </td>
        <td>
          asynOctet </td>
        <td>
          r/o </td>
        <td>
          Detector model name </td>
        <td>
          MODEL </td>
        <td>
          $(P)$(R)Model_RBV </td>
        <td>
          stringin </td>
      </tr>
      <tr>
        <td>
          ADSerialNumber </td>
        <td>
          asynOctet </td>
        <td>
          r/o </td>
        <td>
          Detector serial number </td>
        <td>
          SERIAL_NUMBER </td>
        <td>
          $(P)$(R)SerialNumber_RBV </td>
        <td>
          stringin </td>
      </tr>
      <tr>
        <td>
          ADFirmwareVersion </td>
        <td>
          asynOctet </td>
        <td>
          r/o </td>
        <td>
          Detector firmware version </td>
        <td>
          FIRMWARE_VERSION </td>
        <td>
          $(P)$(R)FirmwareVersion_RBV </td>
        <td>
          stringin </td>
      </tr>
      <tr>
        <td>
          ADSDKVersion </td>
        <td>
          asynOctet </td>
        <td>
          r/o </td>
        <td>
          Detector vendor's Software Development Kit (SDK) version number. </td>
        <td>
          SDK_VERSION </td>
        <td>
          $(P)$(R)SDKVersion_RBV </td>
        <td>
          stringin </td>
      </tr>
      <tr>
        <td>
          ADFirmwareVersion </td>
        <td>
          asynOctet </td>
        <td>
          r/o </td>
        <td>
          Detector firmeare version number. </td>
        <td>
          FIRMWARE_VERSION </td>
        <td>
          $(P)$(R)FirmwareVersion_RBV </td>
        <td>
          stringin </td>
      </tr>
      <tr>
        <td>
          ADMaxSizeX </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          Maximum (sensor) size in the X direction </td>
        <td>
          MAX_SIZE_X </td>
        <td>
          $(P)$(R)MaxSizeX_RBV </td>
        <td>
          longin </td>
      </tr>
      <tr>
        <td>
          ADMaxSizeY </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          Maximum (sensor) size in the Y direction </td>
        <td>
          MAX_SIZE_Y </td>
        <td>
          $(P)$(R)MaxSizeY_RBV </td>
        <td>
          longin </td>
      </tr>
      <tr>
        <td>
          ADTemperature </td>
        <td>
          asynFloat64 </td>
        <td>
          r/w </td>
        <td>
          Detector temperature </td>
        <td>
          TEMPERATURE </td>
        <td>
          $(P)$(R)Temperature<br />
          $(P)$(R)Temperature_RBV<br />
        </td>
        <td>
          ao<br />
          ai </td>
      </tr>
      <tr>
        <td>
          ADTemperatureActual </td>
        <td>
          asynFloat64 </td>
        <td>
          r/o </td>
        <td>
          Actual detector temperature </td>
        <td>
          TEMPERATURE_ACTUAL </td>
        <td>
          $(P)$(R)Temperature_Actual </td>
        <td>
          ai </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Detector readout control including gain, binning, region start and size, reversal</b>
        </td>
      </tr>
      <tr>
        <td>
          ADGain </td>
        <td>
          asynFloat64 </td>
        <td>
          r/w </td>
        <td>
          Detector gain </td>
        <td>
          GAIN </td>
        <td>
          $(P)$(R)Gain<br />
          $(P)$(R)Gain_RBV </td>
        <td>
          ao<br />
          ai </td>
      </tr>
      <tr>
        <td>
          ADBinX </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Binning in the X direction </td>
        <td>
          BIN_X </td>
        <td>
          $(P)$(R)BinX<br />
          $(P)$(R)BinX_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td>
          ADBinY </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Binning in the Y direction </td>
        <td>
          BIN_Y </td>
        <td>
          $(P)$(R)BinY<br />
          $(P)$(R)BinY_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td>
          ADMinX </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          First pixel to read in the X direction.
          <br />
          0 is the first pixel on the detector. </td>
        <td>
          MIN_X </td>
        <td>
          $(P)$(R)MinX<br />
          $(P)$(R)MinX_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td>
          ADMinY </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          First pixel to read in the Y direction.<br />
          0 is the first pixel on the detector. </td>
        <td>
          MIN_Y </td>
        <td>
          $(P)$(R)MinY<br />
          $(P)$(R)MinY_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td>
          ADSizeX </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Size of the region to read in the X direction </td>
        <td>
          SIZE_X </td>
        <td>
          $(P)$(R)SizeX<br />
          $(P)$(R)SizeX_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td>
          ADSizeY </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Size of the region to read in the Y direction </td>
        <td>
          SIZE_Y </td>
        <td>
          $(P)$(R)SizeY<br />
          $(P)$(R)SizeY_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td>
          ADReverseX </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Reverse array in the X direction<br />
          (0=No, 1=Yes) </td>
        <td>
          REVERSE_X </td>
        <td>
          $(P)$(R)ReverseX<br />
          $(P)$(R)ReverseX_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td>
          ADReverseY </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Reverse array in the Y direction<br />
          (0=No, 1=Yes) </td>
        <td>
          REVERSE_Y </td>
        <td>
          $(P)$(R)ReverseY<br />
          $(P)$(R)ReverseY_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Image and trigger modes</b> </td>
      </tr>
      <tr>
        <td>
          ADImageMode </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Image mode (ADImageMode_t). </td>
        <td>
          IMAGE_MODE </td>
        <td>
          $(P)$(R)ImageMode<br />
          $(P)$(R)ImageMode_RBV </td>
        <td>
          mbbo<br />
          mbbi </td>
      </tr>
      <tr>
        <td>
          ADTriggerMode </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Trigger mode (ADTriggerMode_t). </td>
        <td>
          TRIGGER_MODE </td>
        <td>
          $(P)$(R)TriggerMode<br />
          $(P)$(R)TriggerMode_RBV </td>
        <td>
          mbbo<br />
          mbbi </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Frame type</b> </td>
      </tr>
      <tr>
        <td>
          ADFrameType </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Frame type (ADFrameType_t). </td>
        <td>
          FRAME_TYPE </td>
        <td>
          $(P)$(R)FrameType<br />
          $(P)$(R)FrameType_RBV </td>
        <td>
          mbbo<br />
          mbbi </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Acquisition time and period</b> </td>
      </tr>
      <tr>
        <td>
          ADAcquireTime </td>
        <td>
          asynFloat64 </td>
        <td>
          r/w </td>
        <td>
          Acquisition time per image </td>
        <td>
          ACQ_TIME </td>
        <td>
          $(P)$(R)AcquireTime<br />
          $(P)$(R)AcquireTime_RBV </td>
        <td>
          ao<br />
          ai </td>
      </tr>
      <tr>
        <td>
          ADAcquirePeriod </td>
        <td>
          asynFloat64 </td>
        <td>
          r/w </td>
        <td>
          Acquisition period between images </td>
        <td>
          ACQ_PERIOD </td>
        <td>
          $(P)$(R)AcquirePeriod<br />
          $(P)$(R)AcquirePeriod_RBV </td>
        <td>
          ao<br />
          ai </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Number of exposures and number of images</b> </td>
      </tr>
      <tr>
        <td>
          ADNumExposures </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Number of exposures per image to acquire </td>
        <td>
          NEXPOSURES </td>
        <td>
          $(P)$(R)NumExposures<br />
          $(P)$(R)NumExposures_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td>
          ADNumImages </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Number of images to acquire in one acquisition sequence </td>
        <td>
          NIMAGES </td>
        <td>
          $(P)$(R)NumImages<br />
          $(P)$(R)NumImages_RBV </td>
        <td>
          longout<br />
          longin </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Acquisition control</b> </td>
      </tr>
      <tr>
        <td>
          ADAcquire </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Start (1) or stop (0) image acquisition. This record forward links to $(P)$(R)AcquireBusy
          which is an EPICS busy record that does not process its forward link until acquisition
          is complete. Clients should write 1 to the Acquire record to start acquisition,
          and wait for AcquireBusy to go to 0 to know that acquisition is complete. This can
          be done automatically with ca_put_callback. </td>
        <td>
          ACQUIRE </td>
        <td>
          $(P)$(R)Acquire<br />
          $(P)$(R)Acquire_RBV </td>
        <td>
          bo<br />
          bi </td>
      </tr>
      <tr>
        <td>
          N.A. </td>
        <td>
          N.A. </td>
        <td>
          r/o </td>
        <td>
          This is an EPICS busy record that is set to 1 when Acquire is set to 1 and not process
          its forward link until acquisition is complete. </td>
        <td>
          N.A. </td>
        <td>
          $(P)$(R)AcquireBusy </td>
        <td>
          busy </td>
      </tr>
      <tr>
        <td>
          N.A. </td>
        <td>
          N.A. </td>
        <td>
          r/o </td>
        <td>
          This record controls whether AcquireBusy goes to 0 when the detector is done (Acquire=0),
          or whether it waits until $(P)$(R)NumQueuedArrays also goes to 0, i.e. that all
          plugins are also done. Choices are No (0) and Yes(1). </td>
        <td>
          N.A. </td>
        <td>
          $(P)$(R)WaitForPlugins </td>
        <td>
          bo </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Status information</b> </td>
      </tr>
      <tr>
        <td>
          ADStatus </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          Acquisition status (ADStatus_t) </td>
        <td>
          STATUS </td>
        <td>
          $(P)$(R)DetectorState_RBV </td>
        <td>
          mbbi </td>
      </tr>
      <tr>
        <td>
          ADStatusMessage </td>
        <td>
          asynOctet </td>
        <td>
          r/o </td>
        <td>
          Status message string </td>
        <td>
          STATUS_MESSAGE </td>
        <td>
          $(P)$(R)StatusMessage_RBV </td>
        <td>
          waveform </td>
      </tr>
      <tr>
        <td>
          ADStringToServer </td>
        <td>
          asynOctet </td>
        <td>
          r/o </td>
        <td>
          String from driver to string-based vendor server </td>
        <td>
          STRING_TO_SERVER </td>
        <td>
          $(P)$(R)StringToServer_RBV </td>
        <td>
          waveform </td>
      </tr>
      <tr>
        <td>
          ADStringFromServer </td>
        <td>
          asynOctet </td>
        <td>
          r/o </td>
        <td>
          String from string-based vendor server to driver </td>
        <td>
          STRING_FROM_SERVER </td>
        <td>
          $(P)$(R)StringFromServer_RBV </td>
        <td>
          waveform </td>
      </tr>
      <tr>
        <td>
          ADNumExposuresCounter </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          Counter that increments by 1 each time an exposure is acquired for the current image.
          Driver resets to 0 when acquisition is started. </td>
        <td>
          NUM_EXPOSURES_COUNTER </td>
        <td>
          $(P)$(R)NumExposuresCounter_RBV </td>
        <td>
          longin </td>
      </tr>
      <tr>
        <td>
          ADNumImagesCounter </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          Counter that increments by 1 each time an image is acquired in the current acquisition
          sequence. Driver resets to 0 when acquisition is started. Drivers can use this as
          the loop counter when ADImageMode=ADImageMultiple. </td>
        <td>
          NUM_IMAGES_COUNTER </td>
        <td>
          $(P)$(R)NumImagesCounter_RBV </td>
        <td>
          longin </td>
      </tr>
      <tr>
        <td>
          ADTimeRemaining </td>
        <td>
          asynFloat64 </td>
        <td>
          r/o </td>
        <td>
          Time remaining for current image. Drivers should update this value if they are doing
          the exposure timing internally, rather than in the detector hardware. </td>
        <td>
          TIME_REMAINING </td>
        <td>
          $(P)$(R)TimeRemaining_RBV </td>
        <td>
          ai </td>
      </tr>
      <tr>
        <td>
          ADReadStatus </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Write a 1 to this parameter to force a read of the detector status. Detector drivers
          normally read the status as required, so this is usually not necessary, but there
          may be some circumstances under which forcing a status read may be needed. </td>
        <td>
          READ_STATUS </td>
        <td>
          $(P)$(R)ReadStatus </td>
        <td>
          bo </td>
      </tr>
      <tr>
        <td align="center" colspan="7">
          <b>Shutter control</b> </td>
      </tr>
      <tr>
        <td>
          ADShutterMode </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Shutter mode (None, detector-controlled or EPICS-controlled) (ADShutterMode_t)
        </td>
        <td>
          SHUTTER_MODE </td>
        <td>
          $(P)$(R)ShutterMode<br />
          $(P)$(R)ShutterMode_RBV </td>
        <td>
          mbbo<br />
          mbbi </td>
      </tr>
      <tr>
        <td>
          ADShutterControl </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          Shutter control for the selected (detector or EPICS) shutter (ADShutterStatus_t)
        </td>
        <td>
          SHUTTER_CONTROL </td>
        <td>
          $(P)$(R)ShutterControl<br />
          $(P)$(R)ShutterControl_RBV </td>
        <td>
          bo<br />
          bi </td>
      </tr>
      <tr>
        <td>
          ADShutterControlEPICS </td>
        <td>
          asynInt32 </td>
        <td>
          r/w </td>
        <td>
          This record processes when it receives a callback from the driver to open or close
          the EPICS shutter. It triggers the records below to actually open or close the EPICS
          shutter. </td>
        <td>
          SHUTTER_CONTROL_EPICS </td>
        <td>
          $(P)$(R)ShutterControlEPICS </td>
        <td>
          bi </td>
      </tr>
      <tr>
        <td>
          N/A </td>
        <td>
          N/A </td>
        <td>
          r/w </td>
        <td>
          This record writes its OVAL field to its OUT field when the EPICS shutter is told
          to open. The OCAL (and hence OVAL) and OUT fields are user-configurable, so any
          EPICS-controllable shutter can be used. </td>
        <td>
          N/A </td>
        <td>
          $(P)$(R)ShutterOpenEPICS </td>
        <td>
          calcout </td>
      </tr>
      <tr>
        <td>
          N/A </td>
        <td>
          N/A </td>
        <td>
          r/w </td>
        <td>
          This record writes its OVAL field to its OUT field when the EPICS shutter is told
          to close. The OCAL (and hence OVAL) and OUT fields are user-configurable, so any
          EPICS-controllable shutter can be used. </td>
        <td>
          N/A </td>
        <td>
          $(P)$(R)ShutterCloseEPICS </td>
        <td>
          calcout </td>
      </tr>
      <tr>
        <td>
          ADShutterStatus </td>
        <td>
          asynInt32 </td>
        <td>
          r/o </td>
        <td>
          Status of the detector-controlled shutter (ADShutterStatus_t) </td>
        <td>
          SHUTTER_STATUS </td>
        <td>
          $(P)$(R)ShutterStatus_RBV </td>
        <td>
          bi </td>
      </tr>
      <tr>
        <td>
          N/A </td>
        <td>
          N/A </td>
        <td>
          r/o </td>
        <td>
          Status of the EPICS-controlled shutter. This record should have its input link (INP)
          set to a record that contains the open/close status information for the shutter.
          The link should have the "CP" attribute, so this record processes when the input
          changes. The ZRVL field should be set to the value of the input link when the shutter
          is closed, and the ONVL field should be set to the value of the input link when
          the shutter is open. </td>
        <td>
          N/A </td>
        <td>
          $(P)$(R)ShutterStatusEPICS_RBV </td>
        <td>
          mbbi </td>
      </tr>
      <tr>
        <td>
          ADShutterOpenDelay </td>
        <td>
          asynFloat64 </td>
        <td>
          r/w </td>
        <td>
          Time required for the shutter to actually open (ADShutterStatus_t) </td>
        <td>
          SHUTTER_OPEN_DELAY </td>
        <td>
          $(P)$(R)ShutterOpenDelay<br />
          $(P)$(R)ShutterOpenDelay_RBV </td>
        <td>
          ao<br />
          ai </td>
      </tr>
      <tr>
        <td>
          ADShutterCloseDelay </td>
        <td>
          asynFloat64 </td>
        <td>
          r/w </td>
        <td>
          Time required for the shutter to actually close (ADShutterStatus_t) </td>
        <td>
          SHUTTER_CLOSE_DELAY </td>
        <td>
          $(P)$(R)ShutterCloseDelay<br />
          $(P)$(R)ShutterCloseDelay_RBV </td>
        <td>
          ao<br />
          ai </td>
      </tr>
    </tbody>
  </table>


