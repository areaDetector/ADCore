# Must have loaded envPaths via st.cmd.linux or st.cmd.win32

errlogInit(20000)

dbLoadDatabase("$(TOP)/dbd/simDetectorApp.dbd")
simDetectorApp_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("PREFIX", "13SIM1:")
epicsEnvSet("PORT",   "SIM1")
epicsEnvSet("QSIZE",  "20")
epicsEnvSet("XSIZE",  "300")
epicsEnvSet("YSIZE",  "300")
epicsEnvSet("NELM",   "90000")

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES", "100000")
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")

# Create a simDetector driver
# simDetectorConfig(portName, maxSizeX, maxSizeY, dataType, maxBuffers, maxMemory,
#                   priority, stackSize)
simDetectorConfig("$(PORT)", $(XSIZE), $(YSIZE), 1, 0, 0)
dbLoadRecords("ADBase.template",     "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")
dbLoadRecords("simDetector.template","P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")

# Create a V4 server
# NDV4ServerConfigure(portName, maxBuffers, maxMemory, ndarrayPort, ndarrayAddr, pvName,
#                     maxMemory, priority, stackSize)
NDV4ServerConfigure("V4", 3, 0, "$(PORT)", 0, "testMP")
dbLoadRecords("NDPluginBase.template","P=$(PREFIX),R=V4:,PORT=V4,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=0")

NDStdArraysConfigure("Image1", 3, 0, "$(PORT)", 0)
dbLoadRecords("NDPluginBase.template","P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=0")
dbLoadRecords("NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Int8,FTVL=UCHAR,NELEMENTS=$(NELM)")

iocInit()
