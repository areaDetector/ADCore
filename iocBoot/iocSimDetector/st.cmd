# Must have loaded envPaths via st.cmd.linux or st.cmd.win32

errlogInit(20000)

dbLoadDatabase("$(AREA_DETECTOR)/dbd/simDetectorApp.dbd")
simDetectorApp_registerRecordDeviceDriver(pdbbase) 

# Create a simDetector driver
simDetectorConfig("SIM1", 640, 480, 1, 500, -1)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/ADBase.template",     "P=13SIM1:,R=cam1:,PORT=SIM1,ADDR=0,TIMEOUT=1")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/simDetector.template","P=13SIM1:,R=cam1:,PORT=SIM1,ADDR=0,TIMEOUT=1")

# Create a second simDetector driver
simDetectorConfig("SIM2", 300, 200, 1, 50, 50000000)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/ADBase.template",     "P=13SIM1:,R=cam2:,PORT=SIM2,ADDR=0,TIMEOUT=1")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/simDetector.template","P=13SIM1:,R=cam2:,PORT=SIM2,ADDR=0,TIMEOUT=1")

# Create a standard arrays plugin, set it to get data from first simDetector driver.
NDStdArraysConfigure("Image1", 3, 0, "SIM1", 0, -1)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=13SIM1:,R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=SIM1,NDARRAY_ADDR=0")
# This creates a waveform large enough for 640x480x3 (e.g. RGB color) arrays.
# This waveform only allows transporting 8-bit images
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDStdArrays.template", "P=13SIM1:,R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,TYPE=Int8,FTVL=UCHAR,NELEMENTS=921600")

# Create a standard arrays plugin, set it to get data from second simDetector driver.
NDStdArraysConfigure("Image2", 1, 0, "SIM2", 0, -1)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=13SIM1:,R=image2:,PORT=Image2,ADDR=0,TIMEOUT=1,NDARRAY_PORT=SIM2,NDARRAY_ADDR=0")
# This creates a waveform large enough for 640x480x3 (e.g. RGB color) arrays.
# This waveform allows transporting 64-bit images, so it can handle any detector data type at the expense of more memory and bandwidth
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDStdArrays.template", "P=13SIM1:,R=image2:,PORT=Image2,ADDR=0,TIMEOUT=1,TYPE=Float64,FTVL=DOUBLE,NELEMENTS=921600")
# Load the database to use with Stephen Mudie's IDL code
#dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/EPICS_AD_Viewer.template", "P=13SIM1:, R=image1:")

# Create a netCDF file saving plugin
NDFileNetCDFConfigure("FileNetCDF1", 450, 0, "SIM1", 0)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=13SIM1:,R=netCDF1:,PORT=FileNetCDF1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=SIM1,NDARRAY_ADDR=0")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDFile.template",      "P=13SIM1:,R=netCDF1:,PORT=FileNetCDF1,ADDR=0,TIMEOUT=1")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDFileNetCDF.template","P=13SIM1:,R=netCDF1:,PORT=FileNetCDF1,ADDR=0,TIMEOUT=1")

# Create a TIFF file saving plugin
NDFileTIFFConfigure("FileTIFF1", 20, 0, "SIM1", 0)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=13SIM1:,R=TIFF1:,PORT=FileTIFF1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=SIM1,NDARRAY_ADDR=0")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDFile.template",      "P=13SIM1:,R=TIFF1:,PORT=FileTIFF1,ADDR=0,TIMEOUT=1")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDFileTIFF.template",  "P=13SIM1:,R=TIFF1:,PORT=FileTIFF1,ADDR=0,TIMEOUT=1")

# Create a JPEG file saving plugin
NDFileJPEGConfigure("FileJPEG1", 20, 0, "SIM1", 0)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=13SIM1:,R=JPEG1:,PORT=FileJPEG1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=SIM1,NDARRAY_ADDR=0")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDFile.template",      "P=13SIM1:,R=JPEG1:,PORT=FileJPEG1,ADDR=0,TIMEOUT=1")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDFileJPEG.template",  "P=13SIM1:,R=JPEG1:,PORT=FileJPEG1,ADDR=0,TIMEOUT=1")

# Create a NeXus file saving plugin
NDFileNexusConfigure("FileNexus1", 20, 0, "SIM1", 0, 0, 80000)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=13SIM1:,R=Nexus1:,PORT=FileNexus1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=SIM1,NDARRAY_ADDR=0")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDFile.template",      "P=13SIM1:,R=Nexus1:,PORT=FileNexus1,ADDR=0,TIMEOUT=1")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDFileNexus.template", "P=13SIM1:,R=Nexus1:,PORT=FileNexus1,ADDR=0,TIMEOUT=1")

# Create 4 ROI plugins
NDROIConfigure("ROI1", 20, 0, "SIM1", 0, -1, -1)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=13SIM1:,R=ROI1:,  PORT=ROI1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=SIM1,NDARRAY_ADDR=0")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDROI.template",       "P=13SIM1:,R=ROI1:,  PORT=ROI1,ADDR=0,TIMEOUT=1")
NDROIConfigure("ROI2", 20, 0, "SIM1", 0, -1, -1)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=13SIM1:,R=ROI2:,  PORT=ROI2,ADDR=0,TIMEOUT=1,NDARRAY_PORT=SIM1,NDARRAY_ADDR=0")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDROI.template",       "P=13SIM1:,R=ROI2:,  PORT=ROI2,ADDR=0,TIMEOUT=1")
NDROIConfigure("ROI3", 20, 0, "SIM1", 0, -1, -1)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=13SIM1:,R=ROI3:,  PORT=ROI3,ADDR=0,TIMEOUT=1,NDARRAY_PORT=SIM1,NDARRAY_ADDR=0")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDROI.template",       "P=13SIM1:,R=ROI3:,  PORT=ROI3,ADDR=0,TIMEOUT=1")
NDROIConfigure("ROI4", 20, 0, "SIM1", 0, -1, -1)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=13SIM1:,R=ROI4:,  PORT=ROI4,ADDR=0,TIMEOUT=1,NDARRAY_PORT=SIM1,NDARRAY_ADDR=0")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDROI.template",       "P=13SIM1:,R=ROI4:,  PORT=ROI4,ADDR=0,TIMEOUT=1")

# Create a processing plugin
NDProcessConfigure("PROC1", 20, 0, "SIM1", 0, -1, -1)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=13SIM1:,R=Proc1:,  PORT=PROC1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=SIM1,NDARRAY_ADDR=0")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDProcess.template",   "P=13SIM1:,R=Proc1:,  PORT=PROC1,ADDR=0,TIMEOUT=1")

# Create 5 statistics plugins
NDStatsConfigure("STATS1", 20, 0, "SIM1", 0, -1, -1)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=13SIM1:,R=Stats1:,  PORT=STATS1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=SIM1,NDARRAY_ADDR=0")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDStats.template",     "P=13SIM1:,R=Stats1:,  PORT=STATS1,ADDR=0,TIMEOUT=1,HIST_SIZE=256")
NDStatsConfigure("STATS2", 20, 0, "ROI1", 0, -1, -1)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=13SIM1:,R=Stats2:,  PORT=STATS2,ADDR=0,TIMEOUT=1,NDARRAY_PORT=ROI1,NDARRAY_ADDR=0")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDStats.template",     "P=13SIM1:,R=Stats2:,  PORT=STATS2,ADDR=0,TIMEOUT=1,HIST_SIZE=256")
NDStatsConfigure("STATS3", 20, 0, "ROI2", 0, -1, -1)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=13SIM1:,R=Stats3:,  PORT=STATS3,ADDR=0,TIMEOUT=1,NDARRAY_PORT=ROI2,NDARRAY_ADDR=0")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDStats.template",     "P=13SIM1:,R=Stats3:,  PORT=STATS3,ADDR=0,TIMEOUT=1,HIST_SIZE=256")
NDStatsConfigure("STATS4", 20, 0, "ROI3", 0, -1, -1)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=13SIM1:,R=Stats4:,  PORT=STATS4,ADDR=0,TIMEOUT=1,NDARRAY_PORT=ROI3,NDARRAY_ADDR=0")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDStats.template",     "P=13SIM1:,R=Stats4:,  PORT=STATS4,ADDR=0,TIMEOUT=1,HIST_SIZE=256")
NDStatsConfigure("STATS5", 20, 0, "ROI4", 0, -1, -1)
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDPluginBase.template","P=13SIM1:,R=Stats5:,  PORT=STATS5,ADDR=0,TIMEOUT=1,NDARRAY_PORT=ROI4,NDARRAY_ADDR=0")
dbLoadRecords("$(AREA_DETECTOR)/ADApp/Db/NDStats.template",     "P=13SIM1:,R=Stats5:,  PORT=STATS5,ADDR=0,TIMEOUT=1,HIST_SIZE=256")

# Load scan records
dbLoadRecords("$(SSCAN)/sscanApp/Db/scan.db", "P=13SIM1:,MAXPTS1=2000,MAXPTS2=200,MAXPTS3=20,MAXPTS4=10,MAXPTSH=10")

#asynSetTraceIOMask("SIM1",0,2)
#asynSetTraceMask("SIM1",0,255)
#asynSetTraceIOMask("SIM1FileNetCDF",0,2)
#asynSetTraceMask("SIM1FileNetCDF",0,255)
#asynSetTraceMask("SIM1FileNexus",0,255)
#asynSetTraceMask("SIM2",0,255)

set_requestfile_path("./")
set_requestfile_path("$(SSCAN)/sscanApp/Db")
set_savefile_path("./autosave")
set_requestfile_path("$(AREA_DETECTOR)/ADApp/Db")
set_pass0_restoreFile("auto_settings.sav")
set_pass1_restoreFile("auto_settings.sav")
save_restoreSet_status_prefix("13SIM1:")
dbLoadRecords("$(AUTOSAVE)/asApp/Db/save_restoreStatus.db", "P=13SIM1:")

iocInit()

# save things every thirty seconds
create_monitor_set("auto_settings.req", 30, "P=13SIM1:")
