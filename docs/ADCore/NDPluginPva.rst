NDPluginPva
===========
:author: Bruno Martins, Brookhaven National Laboratory

.. contents:: Contents

Overview
--------

This plugin converts NDArray data produced by asynNDArrayDrivers into
the EPICSv4 normative type NTNDArray. An embedded EPICSv4 server is
created to serve the new NTNDArray structure as an EPICSv4 PV. A
`description <http://epics-pvdata.sourceforge.net/alpha/normativeTypes/normativeTypesNDArray.html>`__
of the structure of the NTNDArray normative type is available.

NDPluginPva defines the following parameters.

.. raw:: html


  <table class="table table-bordered">
    <tbody>
      <tr>
        <td align="center" colspan="7,">
          <b>Parameter Definitions in NDPluginPva.h and EPICS Record Definitions in NDPva.template</b>
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
          NDPluginPvaPvName</td>
        <td>
          asynOctet</td>
        <td>
          r/o</td>
        <td>
          Name of the EPICSv4 PV being served</td>
        <td>
          PV_NAME</td>
        <td>
          $(P)$(R)PvName_RBV</td>
        <td>
          waveform</td>
      </tr>
    </tbody>
  </table>


Configuration
-------------

The NDPluginPva plugin is created with the ``NDPvaConfigure`` command,
either from C/C++ or from the EPICS IOC shell.

::

   NDPvaConfigure (const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr, const char *pvName,
                         size_t maxMemory, int priority, int stackSize)
     

For details on the meaning of the parameters to this function refer to
the detailed documentation on the NDPvaConfigure function in the
`NDPluginPva.cpp
documentation <../areaDetectorDoxygenHTML/_n_d_plugin_pva_8cpp.html>`__ and
in the documentation for the constructor for the `NDPluginPva
class <../areaDetectorDoxygenHTML/class_n_d_plugin_pva.html>`__.

The following MEDM screen for the NDPluginPva. The only item not in the
base class screen is the readback of the EPICS V4 PV name.

.. image:: NDPva.png
    :align: center

Starting the pvAccess server
----------------------------

In order to actually serve the EPICSv4 PV created by this plugin it is
necessary to call ``startPVAServer``.

Anedoctal Performance Numbers
-----------------------------

A performance test was conducted at NSLS-II to evaluate the benefits of
using NDPva instead of NDStdArrays to transport images for visualization
purposes. Eight AVT cameras of four different models were used:

+---------------------------+-------------------------------------+------------+
| Model                     | Resolution                          | Frame Rate |
+===========================+=====================================+============+
| Manta G125B (3 instances) | 1292x964x1                          | 30 Hz      |
+---------------------------+-------------------------------------+------------+
| GT2450 (3 instances)      | 2448x2050x1                         | 15 Hz      |
+---------------------------+-------------------------------------+------------+
| GT3400C (1 instance)      | 3384x2704x3 (binned to 1692x1352x3) | 17 Hz      |
+---------------------------+-------------------------------------+------------+
| Mako G131C (1 instance)   | 1280x1024x3                         | 28 Hz      |
+---------------------------+-------------------------------------+------------+

All camera IOCs were concurrently running and acquiring on a HP ProLiant
DL360 Gen9 server with 32GB of memory and a 12-core Xeon E5-2620 @ 2.40
GHz CPU. No other resource intensive process was running during the
tests. This server was connected to a switch via a 10Gbps fiber link, as
was the client computer. All cameras were individually connected via
1Gbps copper links to the switch. The client computer was a HP Z640
Workstation with 32GB of memory and 12-core Xeon E5-1650 @ 3.5GHz CPU,
running CS-Studio version 4.3.3. Five tests were performed:

- **Baseline:** Cameras acquiring, no plugins enabled
- **NDPva, no CS-Studio:** Cameras acquiring, only NDPva enabled,
  images not being displayed in CS-Studio
- **NDPva + CS-Studio:** Cameras acquiring, only NDPva enabled, images
  being displayed in CS-Studio
- **NDStdArrays, no CS-Studio:** Cameras acquiring, only NDStdArrays
  enabled, images not being displayed in CS-Studio
- **NDStdArrays + CS-Studio:** Cameras acquiring, only NDStdArrays
  enabled, images being displayed in CS-Studio

The results of these tests are tabulated as follows:

.. raw:: html

  <table class="table table-bordered">
    <tr>
      <th> Model </th>
      <th> Resolution </th>
      <th> Frame Rate </th>
    </tr>
    <tr>
      <td> Manta G125B (3 instances)</td>
      <td> 1292x964x1 </td>
      <td> 30 Hz </td>
    </tr>
    <tr>
      <td> GT2450 (3 instances)</td>
      <td> 2448x2050x1 </td>
      <td> 15 Hz </td>
    </tr>
    <tr>
      <td> GT3400C (1 instance)</td>
      <td> 3384x2704x3  (binned to 1692x1352x3)</td>
      <td> 17 Hz </td>
    </tr>
    <tr>
      <td> Mako G131C (1 instance)</td>
      <td> 1280x1024x3 </td>
      <td> 28 Hz </td>
    </tr>
  </table>


And, in form of a graph:


.. image:: NDPva_Performance.png
    :align: center


