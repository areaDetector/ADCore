ADcore Future Plans
===================
This is a list of some features that may be implemented in the future.  Some features are
planned for a specific release which is indicated.

* NDArrayPool
    - Add a driver parameter to log cases when buffer could not be allocated?

* NDPluginBase
    - Redo locking mechanism so plugins can reprocess data when something changes?
      One lock for parameter library, another lock for messageQueue, etc?

* File plugins
    - Add new HDF5 file plugin that can be configured with XML file.  This should replace the
      existing areaDetector HDF5 and Nexus plugins, and the 
      local APS HDF5 plugin written by Tim Madden. (R2-1).
    - Implement reading files back into NDArrays.
    - Document NeXus file plugin, document structure of XML file
    - Add File Overwrite Flag, don't allow overwrite if set
    - Add Create Directory flag.  Create new directory if new path is set and directory does not exist

* NDPluginTransform
    - Rewrite to do a single transformation rather than a sequence.  8 possible transformations:
      0, 90, 180, 270 degree rotation with or without transpose for each rotation.  
      This is what the IDL "rotate" function does.  It should be simpler, more general, and faster.
    - Write documentation

* NDPluginOverlay
    - Add width PV to make the lines more than 1 pixel wide
    - Add ellipse shape
    - Make overlay plugin do callbacks when values are changed even if new frame not acquired, etc.
    - New color management patches from Jason, add support for color bar?

* NDAttribute
    - Support macro substitution in attributes files





