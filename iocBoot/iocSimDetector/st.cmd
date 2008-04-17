# Must have loaded envPaths via st.cmd.linux or st.cmd.win32

errlogInit(20000)

dbLoadDatabase("$(AD)/dbd/simDetectorApp.dbd")
simDetectorApp_registerRecordDeviceDriver(pdbbase) 

# Initialize the NDArrayBuff buffer allocation library
NDArrayBuffInit(50, 50000000)

# Create a simDetector driver
simDetectorConfig("SIM1", 640, 480, 1)
dbLoadRecords("$(AD)/ADApp/Db/ADBase.template",     "P=13SIM1:,D=cam1:,PORT=SIM1,ADDR=0,TIMEOUT=1")
dbLoadRecords("$(AD)/ADApp/Db/simDetector.template","P=13SIM1:,D=cam1:,PORT=SIM1,ADDR=0,TIMEOUT=1")

# Create a second simDetector driver
simDetectorConfig("SIM2", 300, 200, 1)
dbLoadRecords("$(AD)/ADApp/Db/ADBase.template",     "P=13SIM1:,D=cam2:,PORT=SIM2,ADDR=0,TIMEOUT=1")
dbLoadRecords("$(AD)/ADApp/Db/simDetector.template","P=13SIM1:,D=cam2:,PORT=SIM2,ADDR=0,TIMEOUT=1")

# Create a standard arrays plugin, set it to get data from first simDetector driver.
drvNDStdArraysConfigure("SIM1Image", 3, 0, "SIM1", 0)
dbLoadRecords("$(AD)/ADApp/Db/NDStdArrays.template","P=13SIM1:,A=image1:,PORT=SIM1Image,ADDR=0,TIMEOUT=1,SIZE=8,FTVL=UCHAR,NELEMENTS=1392640")

# Create a standard arrays plugin, set it to get data from second simDetector driver.
drvNDStdArraysConfigure("SIM2Image", 1, 0, "SIM2", 0)
dbLoadRecords("$(AD)/ADApp/Db/NDStdArrays.template","P=13SIM1:,A=image2:,PORT=SIM2Image,ADDR=0,TIMEOUT=1,SIZE=8,FTVL=UCHAR,NELEMENTS=1392640")

# Create a file saving plugin
drvADFileConfigure("SIM1File", 20, 0, "SIM1", 0)
dbLoadRecords("$(AD)/ADApp/Db/ADFile.template","P=13SIM1:,F=file1:,PORT=SIM1File,ADDR=0,TIMEOUT=1")

#asynSetTraceMask("SIM1",0,255)
#asynSetTraceMask("SIM2",0,255)

set_requestfile_path("./")
set_savefile_path("./autosave")
set_requestfile_path("$(AD)/ADApp/Db")
set_pass0_restoreFile("auto_settings.sav")
set_pass1_restoreFile("auto_settings.sav")
save_restoreSet_status_prefix("13SIM1:")
dbLoadRecords("$(AUTOSAVE)/asApp/Db/save_restoreStatus.db", "P=13SIM1:")

iocInit()

# save things every thirty seconds
create_monitor_set("auto_settings.req", 30, "P=13SIM1:")
