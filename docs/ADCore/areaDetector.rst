areaDetector: EPICS software for area detectors
===============================================

Module Owner: Mark Rivers: University of Chicago

This page is the home of **areaDetector**, an application for
controlling area (2-D) detectors, including CCDs, pixel array detectors,
and online imaging plates.

**NOTE:** This module replaces the
`ccd <http://cars.uchicago.edu/software/epics/ccd.html>`__ and
`pilatusROI <http://cars.uchicago.edu/software/epics/epilatusROI.html>`__
modules. These older modules will no longer be supported, and users are
encouraged to convert to this new areaDetector software.

Devices supported in **areaDetector** include:

From `ADSC <http://www.adsc-xray.com/>`__

-  Large CCD detectors for x-ray diffraction. This support is from Lewis
   Muir of IMCA-CAT at the APS.

From `Allied Vision Technologies (formerly
Prosilica) <http://www.alliedvisiontec.com>`__

-  High-speed, high-resolution CCD and CMOS cameras. These use GigE and
   Firewire interfaces.

From `Andor Technology <http://www.andor.com>`__

-  Cooled CCD cameras with USB and PCI interfaces.
-  Scientific CMOS (sCMOS) cameras with CameraLink interfaces.

From `Axis Communications <http://www.axis.com/>`__ and many other
manufacturers.

-  The areaDetector URL driver can read images from any URL, including
   Web cameras, video servers, disk files, etc.

From `Bruker <http://www.bruker-axs.de/>`__

-  Detectors that run under the Bruker Instrument Server (BIS) server.
-  The old SMART CCD camera line with Photometrics cameras. This
   requires the PCI interface card, replacing the old ISA bus card, and
   requires controlling the detector with WinView, not the SMART
   software.

From `Dectris <http://www.dectris.com>`__

-  The `Pilatus <http://www.dectris.com/sites/pilatus100k.html>`__
   pixel-array detector.

From `Marresearch GmbH <http://www.marresearch.com/>`__

-  mar345 online image plate reader

From `Perkin Elmer <http://optoelectronics.perkinelmer.com/>`__

-  Amorphous silicon flat panel detectors. This support was originally
   written by Brian Tieman and John Hammonds from the APS.
-  Dexela CMOS flat panel detectors.

From `Pixirad <http://pixirad.com>`__

-  CdTe pixel-array detectors

From `Photonic Science <http://photonic-science.co.uk/>`__

-  CCD and CMOS cameras.

From `Point Grey Research <http://www.ptgrey.com/products/index.asp>`__
and many other manufacturers

-  All Firewire (IEEE 1394) cameras that follow the
   `IIDC/DCAM <http://damien.douxchamps.net/ieee1394/libdc1394/iidc/IIDC_1.31.pdf>`__
   specification. There is support for Firewire cameras under Windows in
   the ADFireWireWin module, and support for Firewire cameras under
   Linux in the firewireDCAM module.

From `Point Grey Research/FLIR <http://www.ptgrey.com>`__

-  All Point Grey cameras (Firewire, USB 2.0 and 3.0, GigE) using their
   FlyCapture2 SDK.

From `Point Grey Research/FLIR <http://www.ptgrey.com>`__

-  Point Grey cameras (USB-3, GigE, 10GigE) using their Spinnaker SDK.

From `Rayonix (formely Mar-USA) <http://www.rayonix.com/>`__

-  Cooled x-ray CCD detectors

From `Roper <http://www.roperscientific.com/>`__

-  All CCD detectors supported by the WinView and WinSpec programs. This
   includes all `Princeton
   Instruments <http://www.princetoninstruments.com/>`__ detectors, as
   well as most `Photometrics <http://www.photomet.com/>`__ detectors.
-  All CCD detectors supported by the PVCam library. This includes all
   `Photometrics <http://www.photomet.com/>`__ detectors, as well as
   many `Princeton Instruments <http://www.princetoninstruments.com/>`__
   detectors.
-  All CCD detectors supported by the LightField program. This includes
   many of the newer `Princeton
   Instruments <http://www.princetoninstruments.com/>`__ detectors.

Please email any comments and bug reports to `Mark
Rivers <mailto:rivers@cars.uchicago.edu>`__ who is responsible for
coordinating development and releases.

areaDetector camera drivers supplied by 3rd parties
---------------------------------------------------

Some areaDetector support have been developed by others. These are not
distributed with the areaDetector releases (source or binary) and are
not directly supported by the areaDetector working group, but may be
useful for users:

From `ImXPAD <http://www.imxpad.com>`__

-  XPAD photon counting detectors.
-  areaDetector driver source, info and documentation on
   `github <https://github.com/ImXPAD/ADXpad>`__

Where to find it
----------------

Beginning with release R2-0 (March 2014) the areaDetector module is in
the `areaDetector project on
Github <https://github.com/areaDetector>`__. This project is organized
as a `top level module <https://github.com/areaDetector/areaDetector>`__
and a set of submodules, e.g.
`ADCore <https://github.com/areaDetector/ADCore>`__,
`ADProsilica <https://github.com/areaDetector/ADProsilica>`__,
`ADPilatus <https://github.com/areaDetector/ADPilatus>`__, etc.

The following table provides links to the github repository, the
documentation, and pre-built binaries.

.. raw:: html

  <table class="table table-bordered" border="1" summary="Where to find the software">
    <tbody>
      <tr align="center">
        <th>
          Github repository
        </th>
        <th>
          Description
        </th>
        <th>
          Documentation
        </th>
        <th>
          Release Notes
        </th>
        <th>
          Pre-built binaries
        </th>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/areaDetector">areaDetector</a>
        </td>
        <td>
          Top-level module; ADCore, ADSupport, ADProsilica, etc. go under this
        </td>
        <td>
          <a href="areaDetectorDoc.html">areaDetectorDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/areaDetector/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          N.A.
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADCore">ADCore</a>
        </td>
        <td>
          Base classes, plugins, simulation detector
        </td>
        <td>
          <a href="areaDetectorDoc.html">areaDetectorDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADCore/blob/master/RELEASE.md">Release Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADCore">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADSupport">ADSupport</a>
        </td>
        <td>
          Source code for support libraries (TIFF, JPEG, NETCDF, HDF5, etc.)
        </td>
        <td>
          N.A.
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADSupport/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          N.A.
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADADSC">ADADSC</a>
        </td>
        <td>
          Driver for ADSC detectors
        </td>
        <td>
          <a href="ADSCDoc.html">adscDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADADSC/blob/master/RELEASE.md">Release Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADADSC">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADAndor">ADAndor</a>
        </td>
        <td>
          Driver for Andor CCD detectors
        </td>
        <td>
          <a href="andorDoc.html">andorDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADAndor/blob/master/RELEASE.md">Release Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADAndor">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADAndor3">ADAndor3</a>
        </td>
        <td>
          Driver for Andor sCMOS detectors
        </td>
        <td>
          <a href="andor3Doc.html">andor3Doc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADAndor3/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADAndor3">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADBruker">ADBruker</a>
        </td>
        <td>
          Driver for Bruker detectors using the Bruker Instrument Server (BIS)
        </td>
        <td>
          <a href="BrukerDoc.html">BrukerDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADBruker/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADBruker">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADCameraLink">ADCameraLink</a>
        </td>
        <td>
          Drivers for Silicon Software and Dalsa/Coreco frame grabbers
        </td>
        <td>
          <a href="ADCameraLinkDriver.html">ADCameraLinkDriver</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADCameraLink/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          N.A.
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADCSimDetector">ADCSimDetector</a>
        </td>
        <td>
          Driver for ADC simulation
        </td>
        <td>
          <a href="ADCSimDetectorDoc.html">ADCSimDetectorDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADCSimDetector/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADCSimDetector">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADDexela">ADDexela</a>
        </td>
        <td>
          Driver for Perkin Elmer Dexela detectors
        </td>
        <td>
          <a href="DexelaDoc.html">DexelaDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADDexela/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADDexela">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADFastCCD">ADFastCCD</a>
        </td>
        <td>
          Driver for APS/LBL Fast CCD detector
        </td>
        <td>
          N.A.
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADFastCCD/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          N.A.
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADFireWireWin">ADFireWireWin</a>
        </td>
        <td>
          Driver for Firewire DCAM detectors on Windows using the Carnegie Mellon Firewire
          driver
        </td>
        <td>
          <a href="FirewireWinDoc.html">FirewireWinDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADFireWireWin/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADFireWireWin">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADLambda">ADLambda</a>
        </td>
        <td>
          Driver for Lambda detectors
        </td>
        <td>
          N.A.
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADLambda/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          N.A.
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADLightField">ADLightField</a>
        </td>
        <td>
          Driver for Princeton Instruments detectors using their LightField application
        </td>
        <td>
          <a href="LightFieldDoc.html">LightFieldDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADLightField/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          N.A.
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADmar345">ADmar345</a>
        </td>
        <td>
          Driver for the mar345 image plate detector
        </td>
        <td>
          <a href="Mar345Doc.html">Mar345Doc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADmar345/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADmar345">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADmarCCD">ADmarCCD</a>
        </td>
        <td>
          Driver for CCD detectors from Rayonix (formerly Mar-USA)
        </td>
        <td>
          <a href="MarCCDDoc.html">MarCCDDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADmarCCD/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADmarCCD">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADMerlin">ADMerlin</a>
        </td>
        <td>
          Driver for Merlin detectors from Quantum Detectors
        </td>
        <td>
          N.A.
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADMerlin/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          N.A.
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADMythen">ADMythen</a>
        </td>
        <td>
          Driver for Mythen detectors from Dectris
        </td>
        <td>
          N.A.
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADMythen/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          N.A.
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADnED">ADnED</a>
        </td>
        <td>
          Driver for neutron event data
        </td>
        <td>
          N.A.
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADnED/blob/master/RELEASE.md">Release Notes</a>
        </td>
        <td>
          N.A.
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADPCO">ADPCO</a>
        </td>
        <td>
          Driver for PCO detectors
        </td>
        <td>
          <a href="PCODriver.html">PCODriver</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADPCO/blob/master/RELEASE.md">Release Notes</a>
        </td>
        <td>
          N.A.
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADPerkinElmer">ADPerkinElmer</a>
        </td>
        <td>
          Driver for Perkin Elmer flat-panel detectors
        </td>
        <td>
          <a href="PerkinElmerDoc.html">PerkinElmerDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADPerkinElmer/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADPerkinElmer">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADPICam">ADPICam</a>
        </td>
        <td>
          Driver for Princeton Instruments detectors using the PICam library
        </td>
        <td>
          <a href="PICamDoc.html">PICamDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADPICam/blob/master/RELEASE.md">Release Notes</a>
        </td>
        <td>
          N.A.
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADPilatus">ADPilatus</a>
        </td>
        <td>
          Driver for Pilatus pixel-array detectors
        </td>
        <td>
          <a href="PilatusDoc.html">PilatusDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADPilatus/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADPilatus">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADPixirad">ADPixirad</a>
        </td>
        <td>
          Driver for Pixirad pixel-array detectors
        </td>
        <td>
          <a href="PixiradDoc.html">PixiradDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADPixirad/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADPixirad">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADPointGrey">ADPointGrey</a>
        </td>
        <td>
          Driver for Point Grey Research cameras
        </td>
        <td>
          <a href="PointGreyDoc.html">PointGreyDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADPointGrey/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADPointGrey">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADProsilica">ADProsilica</a>
        </td>
        <td>
          Driver for Allied Vision Technologies (formerly Prosilica) cameras
        </td>
        <td>
          <a href="prosilicaDoc.html">prosilicaDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADProsilica/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADProsilica">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADPSL">ADPSL</a>
        </td>
        <td>
          Driver for Photonic Science detectors
        </td>
        <td>
          <a href="PSLDoc.html">PSLDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADPSL/blob/master/RELEASE.md">Release Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADPSL">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADPvCam">ADPvCam</a>
        </td>
        <td>
          Driver for Photometics and Princeton Instruments detectors using the PvCam library
        </td>
        <td>
          <a href="pvcamDoc.html">pvcamDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADPvCam/blob/master/RELEASE.md">Release Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADPvCam">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADQImaging">ADQImaging</a>
        </td>
        <td>
          Driver for QImaging detectors
        </td>
        <td>
          <a href="QImagingDoc.html">QImagingDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADQImaging/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADQImaging">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADRoper">ADRoper</a>
        </td>
        <td>
          Driver for Princeton Instruments and Photometics detectors using the WinView/WinSpec
          programs
        </td>
        <td>
          <a href="RoperDoc.html">RoperDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADRoper/blob/master/RELEASE.md">Release Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADRoper">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADSimDetector">ADSimDetector</a>
        </td>
        <td>
          Driver for simulation detector
        </td>
        <td>
          <a href="simDetectorDoc.html">simDetectorDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADSimDetector/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADSimDetector">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ADURL">ADURL</a>
        </td>
        <td>
          Driver for reading images from any URL using the GraphicsMagick library
        </td>
        <td>
          <a href="URLDriverDoc.html">URLDriverDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/ADURL/blob/master/RELEASE.md">Release Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/ADURL">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/aravisGigE">aravisGigE</a>
        </td>
        <td>
          Driver using the GNOME Aravis library for Genicam GigE cameras
        </td>
        <td>
          <a href="https://github.com/areaDetector/aravisGigE/blob/master/README.md">README</a>
        </td>
        <td>
          <a href="http://controls.diamond.ac.uk/downloads/support/aravisGigE/">Release Notes</a>
        </td>
        <td>
          N.A.
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ffmpegViewer">ffmpegViewer</a>
        </td>
        <td>
          A stand-alone Qt4 application to display a stream of ffmpeg compressed images
        </td>
        <td>
          <a href="https://github.com/areaDetector/ffmpegViewer/blob/master/README.md">README</a>
        </td>
        <td>
          N.A
        </td>
        <td>
          N.A.
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/ffmpegServer">ffmpegServer</a>
        </td>
        <td>
          Plugin that use the ffmpeg libraries to compress a stream of images to files or
          via an html service
        </td>
        <td>
          <a href="http://controls.diamond.ac.uk/downloads/support/ffmpegServer/">ffmpegServer</a>
        </td>
        <td>
          <a href="http://controls.diamond.ac.uk/downloads/support/ffmpegServer/">Release Notes</a>
        </td>
        <td>
          N.A.
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/firewireDCAM">firewireDCAM</a>
        </td>
        <td>
          Driver for Firewire DCAM detectors on Linux
        </td>
        <td>
          <a href="https://github.com/areaDetector/firewireDCAM/blob/master/README.md">README</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/firewireDCAM/blob/master/RELEASE_NOTES.md">
            Release Notes</a>
        </td>
        <td>
          N.A.
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/NDDriverStdArrays">NDDriverStdArrays</a>
        </td>
        <td>
          Driver that allows EPICS Channel Access clients to create NDArrays in an IOC
        </td>
        <td>
          <a href="NDDriverStdArraysDoc.html">NDDriverStdArraysDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/NDDriverStdArrays/blob/master/RELEASE.md">
            Release Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/NDDriverStdArrays">Pre-built binaries</a>
        </td>
      </tr>
      <tr>
        <td>
          <a href="https://github.com/areaDetector/pvaDriver">pvaDriver</a>
        </td>
        <td>
          Driver that receives EPICS V4 NTNDArrays and converts them to NDArrays in an IOC
        </td>
        <td>
          <a href="pvaDriverDoc.html">pvaDriverDoc</a>
        </td>
        <td>
          <a href="https://github.com/areaDetector/pvaDriver/blob/master/RELEASE.md">Release
            Notes</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/pvaDriver">Pre-built binaries</a>
        </td>
      </tr>
    </tbody>
  </table>

Prior to release R2-0 the areaDetector module is in the `synApps
Subversion repository at the
APS <https://subversion.xray.aps.anl.gov/synApps/areaDetector>`__. The
"trunk" directory contains the most recent unreleased code, while the
"tags" directory contains the released versions.

For releases prior to R2-0 the software can be downloaded from the links
in the table below. The software is available both in source code form,
and in pre-built form so that it can be used without an EPICS build
system. There are separate pre-built binary packages for linux-x86,
linux-x86_64, win32-x86, windows-x64, and cygwin-x86.

.. raw:: html

  <table class="table table-bordered" border="1" summary="Where to find the software">
    <tbody>
      <tr align="center">
        <th>
          Module Version
        </th>
        <th>
          Release Date
        </th>
        <th>
          Source Code Filename
        </th>
        <th>
          Pre-built Filename
        </th>
        <th>
          Documentation
        </th>
        <th>
          Release Notes
        </th>
        <th>
          Known Problems
        </th>
      </tr>
      <tr>
        <td>
          1-9-1
        </td>
        <td>
          11-March-2013
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorR1-9-1.tgz">areaDetectorR1-9-1.tgz</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-9-1_linux-x86.tgz">
            areaDetectorPrebuilt_R1-9-1_linux-x86.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-9-1_linux-x86-gcc43.tgz">
            areaDetectorPrebuilt_R1-9-1_linux-x86-gcc43.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-9-1_linux-x86_64.tgz">
            areaDetectorPrebuilt_R1-9-1_linux-x86_64.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-9-1_linux-x86_64-gcc42.tgz">
            areaDetectorPrebuilt_R1-9-1_linux-x86_64-gcc42.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-9-1_win32-x86.tgz">
            areaDetectorPrebuilt_R1-9-1_win32-x86.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-9-1_windows_x64.tgz">
            areaDetectorPrebuilt_R1-9-1_windows_x64.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-9-1_cygwin-x86.tgz">
            areaDetectorPrebuilt_R1-9-1_cygwin-x86.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-9-1_darwin-x86.tgz">
            areaDetectorPrebuilt_R1-9-1_darwin-x86.tgz</a>
        </td>
        <td>
          <a href="areaDetectorDoc.html">areaDetectorDoc</a>
        </td>
        <td>
          <a href="areaDetectorReleaseNotes.html">Release notes</a>
        </td>
        <td>
          See release notes
        </td>
      </tr>
      <tr>
        <td>
          1-9
        </td>
        <td>
          27-February-2013
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorR1-9.tgz">areaDetectorR1-9.tgz</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-9_linux-x86.tgz">
            areaDetectorPrebuilt_R1-9_linux-x86.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-9_linux-x86-gcc43.tgz">
            areaDetectorPrebuilt_R1-9_linux-x86-gcc43.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-9_linux-x86_64.tgz">
            areaDetectorPrebuilt_R1-9_linux-x86_64.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-9_linux-x86_64-gcc42.tgz">
            areaDetectorPrebuilt_R1-9_linux-x86_64-gcc42.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-9_win32-x86.tgz">
            areaDetectorPrebuilt_R1-9_win32-x86.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-9_windows_x64.tgz">
            areaDetectorPrebuilt_R1-9_windows_x64.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-9_cygwin-x86.tgz">
            areaDetectorPrebuilt_R1-9_cygwin-x86.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-9_darwin-x86.tgz">
            areaDetectorPrebuilt_R1-9_darwin-x86.tgz</a>
        </td>
        <td>
          <a href="areaDetectorDoc.html">areaDetectorDoc</a>
        </td>
        <td>
          <a href="areaDetectorReleaseNotes.html">Release notes</a>
        </td>
        <td>
          See release notes
        </td>
      </tr>
      <tr>
        <td>
          1-8
        </td>
        <td>
          6-October-2012
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorR1-8.tgz">areaDetectorR1-8.tgz</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-8_linux-x86.tgz">
            areaDetectorPrebuilt_R1-8_linux-x86.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-8_linux-x86-gcc43.tgz">
            areaDetectorPrebuilt_R1-8_linux-x86-gcc43.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-8_linux-x86_64.tgz">
            areaDetectorPrebuilt_R1-8_linux-x86_64.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-8_linux-x86_64-gcc42.tgz">
            areaDetectorPrebuilt_R1-8_linux-x86_64-gcc42.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-8_win32-x86.tgz">
            areaDetectorPrebuilt_R1-8_win32-x86.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-8_windows_x64.tgz">
            areaDetectorPrebuilt_R1-8_windows_x64.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-8_cygwin-x86.tgz">
            areaDetectorPrebuilt_R1-8_cygwin-x86.tgz</a>
        </td>
        <td>
          <a href="areaDetectorDoc.html">areaDetectorDoc</a>
        </td>
        <td>
          <a href="areaDetectorReleaseNotes.html">Release notes</a>
        </td>
        <td>
          See release notes
        </td>
      </tr>
      <tr>
        <td>
          1-7
        </td>
        <td>
          9-Aug-2011
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorR1-7.tgz">areaDetectorR1-7.tgz</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-7_linux-x86.tgz">
            areaDetectorPrebuilt_R1-7_linux-x86.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-7_linux-x86_64.tgz">
            areaDetectorPrebuilt_R1-7_linux-x86_64.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-7_win32-x86.tgz">
            areaDetectorPrebuilt_R1-7_win32-x86.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-7_windows_x64.tgz">
            areaDetectorPrebuilt_R1-7_windows_x64.tgz</a><br />
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-7_cygwin-x86.tgz">
            areaDetectorPrebuilt_R1-7_cygwin-x86.tgz</a>
        </td>
        <td>
          <a href="areaDetectorDoc.html">areaDetectorDoc</a>
        </td>
        <td>
          <a href="areaDetectorReleaseNotes.html">Release notes</a>
        </td>
        <td>
          See release notes
        </td>
      </tr>
      <tr>
        <td>
          1-6
        </td>
        <td>
          20-May-2010
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorR1-6.tgz">areaDetectorR1-6.tgz</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-6.tgz">areaDetectorPrebuilt_R1-6.tgz</a>
        </td>
        <td>
          <a href="areaDetectorDoc.html">areaDetectorDoc</a>
        </td>
        <td>
          <a href="areaDetectorReleaseNotes.html">Release notes</a>
        </td>
        <td>
          See release notes
        </td>
      </tr>
      <tr>
        <td>
          1-5
        </td>
        <td>
          23-August-2009
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorR1-5.tgz">areaDetectorR1-5.tgz</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-5.tgz">areaDetectorPrebuilt_R1-5.tgz</a>
        </td>
        <td>
          <a href="areaDetectorDoc.html">areaDetectorDoc</a>
        </td>
        <td>
          <a href="areaDetectorReleaseNotes.html">Release notes</a>
        </td>
        <td>
          See release notes
        </td>
      </tr>
      <tr>
        <td>
          1-4
        </td>
        <td>
          30-Jan-2009
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorR1-4.tgz">areaDetectorR1-4.tgz</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-4.tgz">areaDetectorPrebuilt_R1-4.tgz</a>
        </td>
        <td>
          <a href="areaDetectorDoc.html">areaDetectorDoc</a>
        </td>
        <td>
          <a href="areaDetectorReleaseNotes.html">Release notes</a>
        </td>
        <td>
          See release notes
        </td>
      </tr>
      <tr>
        <td>
          1-3
        </td>
        <td>
          24-Nov-2008
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorR1-3.tgz">areaDetectorR1-3.tgz</a>
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorPrebuilt_R1-3.tgz">areaDetectorPrebuilt_R1-3.tgz</a>
        </td>
        <td>
          <a href="areaDetectorDoc.html">areaDetectorDoc</a>
        </td>
        <td>
          <a href="areaDetectorReleaseNotes.html">Release notes</a>
        </td>
        <td>
          See release notes
        </td>
      </tr>
      <tr>
        <td>
          1-2
        </td>
        <td>
          24-Oct-2008
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorR1-2.tgz">areaDetectorR1-2.tgz</a>
        </td>
        <td>
          N.A.
        </td>
        <td>
          <a href="areaDetectorDoc.html">areaDetectorDoc</a>
        </td>
        <td>
          <a href="areaDetectorReleaseNotes.html">Release notes</a>
        </td>
        <td>
          See release notes
        </td>
      </tr>
      <tr>
        <td>
          1-1
        </td>
        <td>
          10-May-2008
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorR1-1.tgz">areaDetectorR1-1.tgz</a>
        </td>
        <td>
          N.A.
        </td>
        <td>
          <a href="areaDetectorDoc.html">areaDetectorDoc</a>
        </td>
        <td>
          <a href="areaDetectorReleaseNotes.html">Release notes</a>
        </td>
        <td>
          See release notes
        </td>
      </tr>
      <tr>
        <td>
          1-0
        </td>
        <td>
          11-Apr-2008
        </td>
        <td>
          <a href="http://cars.uchicago.edu/software/pub/areaDetectorR1-0.tgz">areaDetectorR1-0.tgz</a>
        </td>
        <td>
          N.A.
        </td>
        <td>
          <a href="areaDetectorDoc.html">areaDetectorDoc</a>
        </td>
        <td>
          <a href="areaDetectorReleaseNotes.html">Release notes</a>
        </td>
        <td>
          See release notes
        </td>
      </tr>
    </tbody>
  </table>


Required Modules
----------------

.. raw:: html

  <table class="table table-bordered" border="1" summary="Required Modules for Source Code Version">
    <tbody>
      <tr align="center">
        <th>
          Module Version
        </th>
        <th>
          Requires module
        </th>
        <th>
          Release needed
        </th>
        <th>
          Required for
        </th>
      </tr>
      <tr>
        <td rowspan="6">
          1-9, 1-9-1
        </td>
        <td>
          EPICS base
        </td>
        <td>
          3.14.12.3
        </td>
        <td>
          Base support
        </td>
      </tr>
      <tr>
        <td>
          asyn
        </td>
        <td>
          4-21
        </td>
        <td>
          Socket and interface support
        </td>
      </tr>
      <tr>
        <td>
          busy
        </td>
        <td>
          1-4
        </td>
        <td>
          busy record
        </td>
      </tr>
      <tr>
        <td>
          calc
        </td>
        <td>
          3-0
        </td>
        <td>
          scalcout and sseq records, needed by sscan database and useful for other databases
        </td>
      </tr>
      <tr>
        <td>
          sscan
        </td>
        <td>
          2-8-1
        </td>
        <td>
          sscan record
        </td>
      </tr>
      <tr>
        <td>
          autosave
        </td>
        <td>
          5-0
        </td>
        <td>
          Save/restore
        </td>
      </tr>
      <tr>
        <td rowspan="6">
          1-8
        </td>
        <td>
          EPICS base
        </td>
        <td>
          3.14.12.2 (with all current patches)
        </td>
        <td>
          Base support
        </td>
      </tr>
      <tr>
        <td>
          asyn
        </td>
        <td>
          4-20
        </td>
        <td>
          Socket and interface support
        </td>
      </tr>
      <tr>
        <td>
          busy
        </td>
        <td>
          1-4
        </td>
        <td>
          busy record. This was formerly included in sscan, but now has its own support module.
        </td>
      </tr>
      <tr>
        <td>
          calc
        </td>
        <td>
          3-0
        </td>
        <td>
          scalcout and sseq records, needed by sscan database and useful for other databases
        </td>
      </tr>
      <tr>
        <td>
          sscan
        </td>
        <td>
          2-8-1
        </td>
        <td>
          sscan record
        </td>
      </tr>
      <tr>
        <td>
          autosave
        </td>
        <td>
          5-0
        </td>
        <td>
          Save/restore
        </td>
      </tr>
      <tr>
        <td rowspan="7">
          1-7
        </td>
        <td>
          EPICS base
        </td>
        <td>
          3.14.12.1
        </td>
        <td>
          Base support
        </td>
      </tr>
      <tr>
        <td>
          asyn
        </td>
        <td>
          4-17
        </td>
        <td>
          Socket and interface support
        </td>
      </tr>
      <tr>
        <td>
          busy
        </td>
        <td>
          1-3
        </td>
        <td>
          busy record. This was formerly included in sscan, but now has its own support module.
        </td>
      </tr>
      <tr>
        <td>
          calc
        </td>
        <td>
          2-8
        </td>
        <td>
          scalcout record, needed by sscan database and useful for other databases
        </td>
      </tr>
      <tr>
        <td>
          sscan
        </td>
        <td>
          2-6-6
        </td>
        <td>
          sscan record
        </td>
      </tr>
      <tr>
        <td>
          mca
        </td>
        <td>
          7-0
        </td>
        <td>
          mca record for getting time sequence of Total or Net counts from statistics plugin
        </td>
      </tr>
      <tr>
        <td>
          autosave
        </td>
        <td>
          4-7
        </td>
        <td>
          Save/restore
        </td>
      </tr>
      <tr>
        <td rowspan="7">
          1-6
        </td>
        <td>
          EPICS base
        </td>
        <td>
          3.14.11
        </td>
        <td>
          Base support
        </td>
      </tr>
      <tr>
        <td>
          asyn
        </td>
        <td>
          4-13-1
        </td>
        <td>
          Socket and interface support
        </td>
      </tr>
      <tr>
        <td>
          busy
        </td>
        <td>
          1-3
        </td>
        <td>
          busy record. This was formerly included in sscan, but now has its own support module.
        </td>
      </tr>
      <tr>
        <td>
          calc
        </td>
        <td>
          2-8
        </td>
        <td>
          scalcout record, needed by sscan database and useful for other databases
        </td>
      </tr>
      <tr>
        <td>
          sscan
        </td>
        <td>
          2-6-6
        </td>
        <td>
          sscan record
        </td>
      </tr>
      <tr>
        <td>
          mca
        </td>
        <td>
          6-12-1
        </td>
        <td>
          mca record for getting time sequence of Total or Net counts from statistics plugin
        </td>
      </tr>
      <tr>
        <td>
          autosave
        </td>
        <td>
          4-7
        </td>
        <td>
          Save/restore
        </td>
      </tr>
      <tr>
        <td rowspan="7">
          1-5
        </td>
        <td>
          EPICS base
        </td>
        <td>
          3.14.10
        </td>
        <td>
          Base support
        </td>
      </tr>
      <tr>
        <td>
          asyn
        </td>
        <td>
          4-12
        </td>
        <td>
          Socket and interface support
        </td>
      </tr>
      <tr>
        <td>
          busy
        </td>
        <td>
          1-2
        </td>
        <td>
          busy record. This was formerly included in sscan, but now has its own support module.
        </td>
      </tr>
      <tr>
        <td>
          calc
        </td>
        <td>
          2-7
        </td>
        <td>
          scalcout record, needed by sscan database and useful for other databases
        </td>
      </tr>
      <tr>
        <td>
          sscan
        </td>
        <td>
          2-6-3
        </td>
        <td>
          sscan record
        </td>
      </tr>
      <tr>
        <td>
          mca
        </td>
        <td>
          6-11
        </td>
        <td>
          mca record for getting time sequence of ROI counts
        </td>
      </tr>
      <tr>
        <td>
          autosave
        </td>
        <td>
          4-5
        </td>
        <td>
          Save/restore
        </td>
      </tr>
      <tr>
        <td rowspan="7">
          1-4
        </td>
        <td>
          EPICS base
        </td>
        <td>
          3.14.10
        </td>
        <td>
          Base support
        </td>
      </tr>
      <tr>
        <td>
          asyn
        </td>
        <td>
          4-10
        </td>
        <td>
          Socket and interface support
        </td>
      </tr>
      <tr>
        <td>
          busy
        </td>
        <td>
          1-1
        </td>
        <td>
          busy record. This was formerly included in sscan, but now has its own support module.
        </td>
      </tr>
      <tr>
        <td>
          calc
        </td>
        <td>
          2-7
        </td>
        <td>
          scalcout record, needed by sscan database and useful for other databases
        </td>
      </tr>
      <tr>
        <td>
          sscan
        </td>
        <td>
          2-6-3
        </td>
        <td>
          sscan record
        </td>
      </tr>
      <tr>
        <td>
          mca
        </td>
        <td>
          6-10
        </td>
        <td>
          mca record for getting time sequence of ROI counts
        </td>
      </tr>
      <tr>
        <td>
          autosave
        </td>
        <td>
          4-5
        </td>
        <td>
          Save/restore
        </td>
      </tr>
      <tr>
        <td rowspan="6">
          1-3
        </td>
        <td>
          EPICS base
        </td>
        <td>
          3.14.10
        </td>
        <td>
          Base support. 3.14.8.2 also works, but the bug in epicsRingPointer can be a problem
          on multi-processor Linux systems.
        </td>
      </tr>
      <tr>
        <td>
          asyn
        </td>
        <td>
          4-10
        </td>
        <td>
          Socket and interface support
        </td>
      </tr>
      <tr>
        <td>
          calc
        </td>
        <td>
          2-6-7
        </td>
        <td>
          scalcout record, needed by sscan database and useful for other databases
        </td>
      </tr>
      <tr>
        <td>
          sscan
        </td>
        <td>
          2-6-2
        </td>
        <td>
          sscan and busy records
        </td>
      </tr>
      <tr>
        <td>
          mca
        </td>
        <td>
          6-10
        </td>
        <td>
          mca record for getting time sequence of ROI counts
        </td>
      </tr>
      <tr>
        <td>
          autosave
        </td>
        <td>
          4-4
        </td>
        <td>
          Save/restore
        </td>
      </tr>
      <tr>
        <td rowspan="6">
          1-2
        </td>
        <td>
          EPICS base
        </td>
        <td>
          3.14.10
        </td>
        <td>
          Base support. 3.14.8.2 also works, but the bug in epicsRingPointer can be a problem
          on multi-processor Linux systems.
        </td>
      </tr>
      <tr>
        <td>
          asyn
        </td>
        <td>
          4-10
        </td>
        <td>
          Socket and interface support
        </td>
      </tr>
      <tr>
        <td>
          calc
        </td>
        <td>
          2-6-5
        </td>
        <td>
          scalcout record, needed by sscan database and useful for other databases
        </td>
      </tr>
      <tr>
        <td>
          sscan
        </td>
        <td>
          2-5-6
        </td>
        <td>
          Busy record
        </td>
      </tr>
      <tr>
        <td>
          mca
        </td>
        <td>
          6-10
        </td>
        <td>
          mca record for getting time sequence of ROI counts
        </td>
      </tr>
      <tr>
        <td>
          autosave
        </td>
        <td>
          4-3
        </td>
        <td>
          Save/restore
        </td>
      </tr>
      <tr>
        <td rowspan="4">
          1-1
        </td>
        <td>
          EPICS base
        </td>
        <td>
          3.14.8.2
        </td>
        <td>
          Base support
        </td>
      </tr>
      <tr>
        <td>
          asyn
        </td>
        <td>
          4-10
        </td>
        <td>
          Socket and interface support
        </td>
      </tr>
      <tr>
        <td>
          sscan
        </td>
        <td>
          2-5-6
        </td>
        <td>
          Busy record
        </td>
      </tr>
      <tr>
        <td>
          autosave
        </td>
        <td>
          4-3
        </td>
        <td>
          Save/restore
        </td>
      </tr>
      <tr>
        <td rowspan="4">
          1-0
        </td>
        <td>
          EPICS base
        </td>
        <td>
          3.14.8.2
        </td>
        <td>
          Base support
        </td>
      </tr>
      <tr>
        <td>
          asyn
        </td>
        <td>
          4-10
        </td>
        <td>
          Socket and interface support
        </td>
      </tr>
      <tr>
        <td>
          sscan
        </td>
        <td>
          2-5-6
        </td>
        <td>
          Busy record
        </td>
      </tr>
      <tr>
        <td>
          autosave
        </td>
        <td>
          4-3
        </td>
        <td>
          Save/restore
        </td>
      </tr>
    </tbody>
  </table>


Installation and Building
-------------------------

For R2-0 and later this is described in the
`INSTALL_GUIDE.md <https://github.com/areaDetector/areaDetector/blob/master/INSTALL_GUIDE.md>`__
on Github.

For releases prior to R2-0 this is described in detail in the
`installation section of the areaDetector
documentation <areaDetectorDoc.html#Installation>`__ for that particular
release.

Please email  `Mark Rivers <mailto:rivers@cars.uchicago.edu>`__  so that
a record can be kept of which sites are using this software.

In Use
------

This software was originally developed by Mark Rivers.

-  In use at APS, NSLS, DLS, SLS, BESSY, NSRRC and others.
