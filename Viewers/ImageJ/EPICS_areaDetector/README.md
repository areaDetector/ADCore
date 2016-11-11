This directory contains the following ImageJ plugins

- EPICS_AD_Viewer.java
  This plugin does real-time updates of images using the NDStdArrays plugin.
  It supports taking a snapshot of the current frame into a new window and
  collecting an image stack.

- EPICS_AD_Controller.java
  This plugin supports the following operations:
  - Defining the readout region of a detector from a rectangular ROI in ImageJ.
  - Defining the ROI for NDPluginROI from a rectangular ROI in ImageJ.
  - Defining a box or cross overlay in NDPluginOverlay using a rectangular or point
    ROI in ImageJ.
  - Defining a ellipse overlay in NDPluginOverlay using an oval or point ROI in ImageJ.

- Gaussian_Profiler.java
  This plugin does dynamic line profiles with real-time Gaussian profile
  fitting.  It is very useful for focusing and beam diagnostics.  It was
  written by Nouamane Laanait (previously at APS, currently at ORNL).

- Dynamic_Profiler.java
  This plugin does dynamic line profiles, i.e. line profiles where the plot
  updates automatically when the image changes or when the line or rectangle
  defining the profile is changed.  This plugin was required in old versions
  of ImageJ.  However, in new versions of ImageJ the standard profile plot
  tool has a "Live" option, which implements the capability of
  Dynamic_Profiler.java, so this plugin is no longer needed.

To uses these plugins in ImageJ do the following:
  - Copy this folder into the ImageJ/plugins folder
  - In ImageJ use the "Plugins/Compile and Run..." menu to compile the .java code
    for the plugin.  This produces .class files.  This only needs to be done once.
  - The plugin will appear in the Plugins/EPICS_areaDetector menu when ImageJ
    is restarted.
