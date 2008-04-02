< envPaths
errlogInit(20000)

dbLoadDatabase("$(AD)/dbd/simDetectorApp.dbd")
simDetectorApp_registerRecordDeviceDriver(pdbbase) 
dbLoadRecords("$(AD)/ADApp/Db/ADBase.template",     "P=13SIM1:,D=cam1:,PORT=SIM1,ADDR=0,TIMEOUT=1")
dbLoadRecords("$(AD)/ADApp/Db/simDetector.template","P=13SIM1:,D=cam1:,PORT=SIM1,ADDR=0,TIMEOUT=1")
dbLoadRecords("$(AD)/ADApp/Db/ADImage.template","P=13SIM1:,D=cam1:,PORT=SIM1Image,ADDR=0,TIMEOUT=1,SIZE=8,FTVL=UCHAR,NPIXELS=1392640")

simDetectorConfig("SIM1", 640, 480, 1)
drvADImageConfigure("SIM1Image", "SIM1")

#asynSetTraceMask("SIM1",0,255)

set_requestfile_path("./")
set_savefile_path("./autosave")
set_requestfile_path("$(AD)/ADApp/Db")
set_pass0_restoreFile("auto_settings.sav")
set_pass1_restoreFile("auto_settings.sav")
save_restoreSet_status_prefix("13SIM1:")
dbLoadRecords("$(AUTOSAVE)/asApp/Db/save_restoreStatus.db", "P=13SIM1:")

iocInit()

# save things every thirty seconds
create_monitor_set("auto_settings.req", 30, "P=13SIM1:, D=cam1:")
