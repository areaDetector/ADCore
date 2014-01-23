#!../../bin/linux-x86/hdf5LayoutXML

< envPaths

cd "$(TOP)"

epicsEnvSet "EPICS_CA_MAX_ARRAY_BYTES", '2000000'
epicsEnvSet "EPICS_TS_MIN_WEST", '0'


# Loading libraries
# -----------------

# Device initialisation
# ---------------------

cd "$(TOP)"

dbLoadDatabase "dbd/hdf5LayoutXML.dbd"
hdf5LayoutXML_registerRecordDeviceDriver(pdbbase)

# simDetectorConfig(portName, maxSizeX, maxSizeY, dataType, maxBuffers, maxMemory)
simDetectorConfig("CAM1.CAM", 512, 512, 1, 50, 0)
dbLoadRecords("$(ADCORE)/db/ADBase.template",     "P=TESTSIMDETECTOR,R=:CAM:,PORT=CAM1.CAM,ADDR=0,TIMEOUT=1")
dbLoadRecords("$(ADCORE)/db/simDetector.template","P=TESTSIMDETECTOR,R=:CAM:,PORT=CAM1.CAM,ADDR=0,TIMEOUT=1")

# NDFileHDF5Configure(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr)
NDFileHDF5Configure("CAM1.HDF", 16, 0, "CAM1.CAM", 0)

dbLoadRecords("$(ADCORE)/db/NDPluginBase.template", "P=TESTSIMDETECTOR,R=:HDF:,PORT=CAM1.HDF,TIMEOUT=1,ADDR=0,NDARRAY_PORT=CAM1.CAM,NDARRAY_ADDR=0,Enabled=1")
dbLoadRecords("$(ADCORE)/db/NDFile.template", "P=TESTSIMDETECTOR,R=:HDF:,PORT=CAM1.HDF,TIMEOUT=1,ADDR=0")
dbLoadRecords("$(ADCORE)/db/NDFileHDF5.template", "P=TESTSIMDETECTOR,R=:HDF:,PORT=CAM1.HDF")

# Final ioc initialisation
# ------------------------
cd "$(TOP)"
#dbLoadRecords 'db/hdf5LayoutXML_expanded.db'
#dbLoadRecords 'db/hdf5LayoutXML.db'
iocInit

#asynSetTraceIOMask("CAM1.STAT",0,2)
#asynSetTraceMask("CAM1.STAT",0,255)

# Extra post-init IOC commands
dbpf TESTSIMDETECTOR:HDF:FilePath "data"
dbpf TESTSIMDETECTOR:HDF:FileName "test"
dbpf TESTSIMDETECTOR:HDF:FileTemplate "%s%s_%d.hdf5"
dbpf TESTSIMDETECTOR:HDF:FileWriteMode "Stream"
dbpf TESTSIMDETECTOR:HDF:XMLPath "hdf5LayoutXMLApp/data"
dbpf TESTSIMDETECTOR:HDF:XMLFileName "layout.xml"


dbpf TESTSIMDETECTOR:CAM:NDAttributesFile, hdf5LayoutXMLApp/data/CAM1.CAM.xml
#dbpf TESTSIMDETECTOR:STAT:NDAttributesFile, hdf5LayoutXMLApp/data/CAM1.STAT.xml


