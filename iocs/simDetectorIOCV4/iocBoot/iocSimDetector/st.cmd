# Must have loaded envPaths via st.cmd.linux or st.cmd.win32

errlogInit(20000)

dbLoadDatabase("$(TOP)/dbd/simDetectorApp.dbd")
simDetectorApp_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("PREFIX", "13SIM1:")
epicsEnvSet("PORT",   "SIM1")
epicsEnvSet("QSIZE",  "20")
epicsEnvSet("XSIZE",  "300")
epicsEnvSet("YSIZE",  "300")
epicsEnvSet("NCHANS", "2048")

#epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES", "10000000")
epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES", "4000000")
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")

# Create a simDetector driver
# simDetectorConfig(const char *portName, int maxSizeX, int maxSizeY, int dataType,
#                   int maxBuffers, int maxMemory, int priority, int stackSize)
simDetectorConfig("$(PORT)", $(XSIZE), $(YSIZE), 1, 0, 0)
dbLoadRecords("ADBase.template",     "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")
dbLoadRecords("simDetector.template","P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")

# Create a V4 server
NDV4ServerConfigure("V4", 3, 0, "$(PORT)", 0, "testMP")
dbLoadRecords("NDPluginBase.template","P=$(PREFIX),R=V4:,PORT=V4,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=0")

# Create a standard arrays plugin, set it to get data from first simDetector driver.
NDStdArraysConfigure("Image1", 3, 0, "$(PORT)", 0)
dbLoadRecords("NDPluginBase.template","P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=0")

# This creates a waveform large enough for 1024x1024x1 (Mono)
# This waveform only allows transporting 8-bit images
dbLoadRecords("NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Int8,FTVL=UCHAR,NELEMENTS=1048576")

iocInit()

dbpf 13SIM1:cam1:Acquire 1
dbpf 13SIM1:image1:EnableCallbacks 1
dbpf 13SIM1:V4:EnableCallbacks 1
dbpf 13SIM1:cam1:ArrayCallbacks 1
