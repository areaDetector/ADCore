# Must have loaded envPaths via st.cmd.linux or st.cmd.win32

errlogInit(20000)

dbLoadDatabase("$(AREA_DETECTOR)/dbd/simDetectorApp.dbd")
simDetectorApp_registerRecordDeviceDriver(pdbbase) 

epicsEnvSet("PREFIX", "13SIM1:")
epicsEnvSet("PORT",   "SIM1")
epicsEnvSet("QSIZE",  "20")
epicsEnvSet("XSIZE",  "1024")
epicsEnvSet("YSIZE",  "1024")
epicsEnvSet("NCHANS", "2048")

# Create a simDetector driver
# simDetectorConfig(const char *portName, int maxSizeX, int maxSizeY, int dataType,
#                   int maxBuffers, int maxMemory, int priority, int stackSize)
simDetectorConfig("$(PORT)", $(XSIZE), $(YSIZE), 1, 0, 0)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/ADBase.template",     "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/simDetector.template","P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")

# Create a second simDetector driver
simDetectorConfig("SIM2", 300, 200, 1, 50, 50000000)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/ADBase.template",     "P=$(PREFIX),R=cam2:,PORT=SIM2,ADDR=0,TIMEOUT=1")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/simDetector.template","P=$(PREFIX),R=cam2:,PORT=SIM2,ADDR=0,TIMEOUT=1")

# Create a standard arrays plugin, set it to get data from first simDetector driver.
NDStdArraysConfigure("Image1", 3, 0, "$(PORT)", 0)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=0")

# This creates a waveform large enough for 1024x1024x3 (e.g. RGB color) arrays.
# This waveform only allows transporting 8-bit images
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,TYPE=Int8,FTVL=UCHAR,NELEMENTS=3145728")
# This waveform only allows transporting 16-bit images
#dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,TYPE=Int16,FTVL=USHORT,NELEMENTS=3145728")
# This waveform allows transporting 32-bit images
#dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,TYPE=Int32,FTVL=LONG,NELEMENTS=3145728")

# Create a standard arrays plugin, set it to get data from second simDetector driver.
NDStdArraysConfigure("Image2", 1, 0, "SIM2", 0)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=$(PREFIX),R=image2:,PORT=Image2,ADDR=0,TIMEOUT=1,NDARRAY_PORT=SIM2,NDARRAY_ADDR=0")

# This creates a waveform large enough for 640x480x3 (e.g. RGB color) arrays.
# This waveform allows transporting 64-bit images, so it can handle any detector data type at the expense of more memory and bandwidth
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDStdArrays.template", "P=$(PREFIX),R=image2:,PORT=Image2,ADDR=0,TIMEOUT=1,TYPE=Float64,FTVL=DOUBLE,NELEMENTS=921600")


# This loads a database to tie the ROI size to the detector readout size
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDROI_sync.template", "P=$(PREFIX),CAM=cam1:,ROI=ROI1:")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDROI_sync.template", "P=$(PREFIX),CAM=cam1:,ROI=ROI2:")

# Load all other plugins using commonPlugins.cmd
< ../commonPlugins.cmd

#asynSetTraceIOMask("$(PORT)",0,2)
#asynSetTraceMask("$(PORT)",0,255)
#asynSetTraceIOMask("FileNetCDF",0,2)
#asynSetTraceMask("FileNetCDF",0,255)
#asynSetTraceMask("FileNexus",0,255)
#asynSetTraceMask("SIM2",0,255)

iocInit()

# save things every thirty seconds
create_monitor_set("auto_settings.req", 30, "P=$(PREFIX)")
