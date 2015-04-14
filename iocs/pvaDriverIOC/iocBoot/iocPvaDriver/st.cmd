# Must have loaded envPaths via st.cmd.linux or st.cmd.win32

errlogInit(20000)

dbLoadDatabase("$(TOP)/dbd/pvaDriverApp.dbd")
pvaDriverApp_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("PREFIX", "13PVA1:")
epicsEnvSet("PORT",   "PVA1")
epicsEnvSet("QSIZE",  "20")
epicsEnvSet("PVNAME", "testMP")
epicsEnvSet("NCHANS", "2048")
epicsEnvSet("CBUFFS", "500")
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES", "1000000")

# Create a pvaDriver
# pvaDriverConfig(const char *portName, const char *pvName, int maxBuffers,
#                 int maxMemory, int priority, int stackSize)
pvaDriverConfig("$(PORT)", "$(PVNAME)", 5, 0, 0)
dbLoadRecords("pvaDriver.template","P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")

# Create a standard arrays plugin, set it to get data from pvaDriver
NDStdArraysConfigure("Image1", 3, 1, "$(PORT)", 0)

# 300x300 8-bit
dbLoadRecords("NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Int8,FTVL=UCHAR,NELEMENTS=90000")

# Load all other plugins using commonPlugins.cmd
#< $(ADCORE)/iocBoot/commonPlugins.cmd
iocInit()

# save things every thirty seconds
#create_monitor_set("auto_settings.req", 30, "P=$(PREFIX)")

dbpf 13PVA1:cam1:EnableCallbacks 1
dbpf 13PVA1:image1:EnableCallbacks 1
