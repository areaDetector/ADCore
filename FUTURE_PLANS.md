ADcore Future Plans
===================
This is a list of some features that may be implemented in the future.  If features are
planned for a specific release then this is indicated.

* NDArrayPool
    - Add a driver parameter to log cases when buffer could not be allocated?

* NDPluginBase
    - Redo locking mechanism so plugins can reprocess data when something changes?
      One lock for parameter library, another lock for messageQueue, etc?
      
* ADBase
    - Add base class virtual methods to do operations that all drivers need to do
        - setUniqueID()
        - setTimeStamp()
        - setAttributes()
        - callPlugins()
        - A method that does all 4 of the above steps

* File plugins
    - Implement reading files back into NDArrays.
    - Document NeXus file plugin, document structure of XML file
    - Add File Overwrite Flag, don't allow overwrite if set
    - Add Create Directory flag.  Create new directory if new path is set and directory does not exist
    - Get the GraphicsMagick plugin working again.  This requires getting versions of GraphicsMagick that 
      work on static and dynamic, 32 and 64 bit, Linux and Windows.  
      This is also required to get URL driver working. (R2-2?)

* NDPluginOverlay
    - Add ellipse shape
    - Make overlay plugin do callbacks when values are changed even if new frame not acquired, etc.
    - New color management patches from Jason, add support for color bar?

* NDAttribute
    - Support macro substitution in attributes files
    
* simDetector
    - Remove iocsh commands from driver
    - See if driver can be built with only libCom and asynDriver core
    - Make an example C++ main program that acquires data with statistics plugin using nothing
      from base except libCom.  asyn needs to build a "core" library with only libCom
    - If this is successful then do this for all real drivers so areaDetector drivers can be used
      outside EPICS IOC





