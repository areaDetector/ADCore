# Create a netCDF file saving plugin.
NDFileNetCDFConfigure("FileNetCDF1", $(QSIZE), 0, "$(PORT)", 0)
dbLoadRecords("NDFileNetCDF.template","P=$(PREFIX),R=netCDF1:,PORT=FileNetCDF1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")

# Create a TIFF file saving plugin
NDFileTIFFConfigure("FileTIFF1", $(QSIZE), 0, "$(PORT)", 0)
dbLoadRecords("NDFileTIFF.template",  "P=$(PREFIX),R=TIFF1:,PORT=FileTIFF1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")

# Create a JPEG file saving plugin
NDFileJPEGConfigure("FileJPEG1", $(QSIZE), 0, "$(PORT)", 0)
dbLoadRecords("NDFileJPEG.template",  "P=$(PREFIX),R=JPEG1:,PORT=FileJPEG1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")

# Create a NeXus file saving plugin
NDFileNexusConfigure("FileNexus1", $(QSIZE), 0, "$(PORT)", 0)
dbLoadRecords("NDFileNexus.template", "P=$(PREFIX),R=Nexus1:,PORT=FileNexus1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")

# Create an HDF5 file saving plugin
NDFileHDF5Configure("FileHDF1", $(QSIZE), 0, "$(PORT)", 0)
dbLoadRecords("NDFileHDF5.template",  "P=$(PREFIX),R=HDF1:,PORT=FileHDF1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")

# Create a Magick file saving plugin
NDFileMagickConfigure("FileMagick1", $(QSIZE), 0, "$(PORT)", 0)
dbLoadRecords("NDFileMagick.template","P=$(PREFIX),R=Magick1:,PORT=FileMagick1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")

# Create 4 ROI plugins
NDROIConfigure("ROI1", $(QSIZE), 0, "$(PORT)", 0, 0, 0)
dbLoadRecords("NDROI.template",       "P=$(PREFIX),R=ROI1:,  PORT=ROI1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")
NDROIConfigure("ROI2", $(QSIZE), 0, "$(PORT)", 0, 0, 0)
dbLoadRecords("NDROI.template",       "P=$(PREFIX),R=ROI2:,  PORT=ROI2,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")
NDROIConfigure("ROI3", $(QSIZE), 0, "$(PORT)", 0, 0, 0)
dbLoadRecords("NDROI.template",       "P=$(PREFIX),R=ROI3:,  PORT=ROI3,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")
NDROIConfigure("ROI4", $(QSIZE), 0, "$(PORT)", 0, 0, 0)
dbLoadRecords("NDROI.template",       "P=$(PREFIX),R=ROI4:,  PORT=ROI4,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")

# Create 8 ROIStat plugins
NDROIStatConfigure("ROISTAT1", $(QSIZE), 0, "$(PORT)", 0, 8, 0, 0)
dbLoadRecords("NDROIStat.template",   "P=$(PREFIX),R=ROIStat1:  ,PORT=ROISTAT1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NCHANS=$(NCHANS)")
dbLoadRecords("NDROIStatN.template",  "P=$(PREFIX),R=ROIStat1:1:,PORT=ROISTAT1,ADDR=0,TIMEOUT=1,NCHANS=$(NCHANS)")
dbLoadRecords("NDROIStatN.template",  "P=$(PREFIX),R=ROIStat1:2:,PORT=ROISTAT1,ADDR=1,TIMEOUT=1,NCHANS=$(NCHANS)")
dbLoadRecords("NDROIStatN.template",  "P=$(PREFIX),R=ROIStat1:3:,PORT=ROISTAT1,ADDR=2,TIMEOUT=1,NCHANS=$(NCHANS)")
dbLoadRecords("NDROIStatN.template",  "P=$(PREFIX),R=ROIStat1:4:,PORT=ROISTAT1,ADDR=3,TIMEOUT=1,NCHANS=$(NCHANS)")
dbLoadRecords("NDROIStatN.template",  "P=$(PREFIX),R=ROIStat1:5:,PORT=ROISTAT1,ADDR=4,TIMEOUT=1,NCHANS=$(NCHANS)")
dbLoadRecords("NDROIStatN.template",  "P=$(PREFIX),R=ROIStat1:6:,PORT=ROISTAT1,ADDR=5,TIMEOUT=1,NCHANS=$(NCHANS)")
dbLoadRecords("NDROIStatN.template",  "P=$(PREFIX),R=ROIStat1:7:,PORT=ROISTAT1,ADDR=6,TIMEOUT=1,NCHANS=$(NCHANS)")
dbLoadRecords("NDROIStatN.template",  "P=$(PREFIX),R=ROIStat1:8:,PORT=ROISTAT1,ADDR=7,TIMEOUT=1,NCHANS=$(NCHANS)")

# Create a processing plugin
NDProcessConfigure("PROC1", $(QSIZE), 0, "$(PORT)", 0, 0, 0)
dbLoadRecords("NDProcess.template",   "P=$(PREFIX),R=Proc1:,  PORT=PROC1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")

# Create 5 statistics plugins
NDStatsConfigure("STATS1", $(QSIZE), 0, "$(PORT)", 0, 0, 0)
dbLoadRecords("NDStats.template",     "P=$(PREFIX),R=Stats1:,  PORT=STATS1,ADDR=0,TIMEOUT=1,HIST_SIZE=256,XSIZE=$(XSIZE),YSIZE=$(YSIZE),NCHANS=$(NCHANS),NDARRAY_PORT=$(PORT)")
NDStatsConfigure("STATS2", $(QSIZE), 0, "ROI1", 0, 0, 0)
dbLoadRecords("NDStats.template",     "P=$(PREFIX),R=Stats2:,  PORT=STATS2,ADDR=0,TIMEOUT=1,HIST_SIZE=256,XSIZE=$(XSIZE),YSIZE=$(YSIZE),NCHANS=$(NCHANS),NDARRAY_PORT=$(PORT)")
NDStatsConfigure("STATS3", $(QSIZE), 0, "ROI2", 0, 0, 0)
dbLoadRecords("NDStats.template",     "P=$(PREFIX),R=Stats3:,  PORT=STATS3,ADDR=0,TIMEOUT=1,HIST_SIZE=256,XSIZE=$(XSIZE),YSIZE=$(YSIZE),NCHANS=$(NCHANS),NDARRAY_PORT=$(PORT)")
NDStatsConfigure("STATS4", $(QSIZE), 0, "ROI3", 0, 0, 0)
dbLoadRecords("NDStats.template",     "P=$(PREFIX),R=Stats4:,  PORT=STATS4,ADDR=0,TIMEOUT=1,HIST_SIZE=256,XSIZE=$(XSIZE),YSIZE=$(YSIZE),NCHANS=$(NCHANS),NDARRAY_PORT=$(PORT)")
NDStatsConfigure("STATS5", $(QSIZE), 0, "ROI4", 0, 0, 0)
dbLoadRecords("NDStats.template",     "P=$(PREFIX),R=Stats5:,  PORT=STATS5,ADDR=0,TIMEOUT=1,HIST_SIZE=256,XSIZE=$(XSIZE),YSIZE=$(YSIZE),NCHANS=$(NCHANS),NDARRAY_PORT=$(PORT)")

# Create a transform plugin
NDTransformConfigure("TRANS1", $(QSIZE), 0, "$(PORT)", 0, 0, 0)
dbLoadRecords("NDTransform.template", "P=$(PREFIX),R=Trans1:,  PORT=TRANS1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")

# Create an overlay plugin with 8 overlays
NDOverlayConfigure("OVER1", $(QSIZE), 0, "$(PORT)", 0, 8, 0, 0)
dbLoadRecords("NDOverlay.template", "P=$(PREFIX),R=Over1:, PORT=OVER1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")
dbLoadRecords("NDOverlayN.template","P=$(PREFIX),R=Over1:1:,NAME=ROI1,   SHAPE=1,O=Over1:,XPOS=$(PREFIX)ROI1:MinX_RBV,YPOS=$(PREFIX)ROI1:MinY_RBV,XSIZE=$(PREFIX)ROI1:SizeX_RBV,YSIZE=$(PREFIX)ROI1:SizeY_RBV,PORT=OVER1,ADDR=0,TIMEOUT=1")
dbLoadRecords("NDOverlayN.template","P=$(PREFIX),R=Over1:2:,NAME=ROI2,   SHAPE=1,O=Over1:,XPOS=$(PREFIX)ROI2:MinX_RBV,YPOS=$(PREFIX)ROI2:MinY_RBV,XSIZE=$(PREFIX)ROI2:SizeX_RBV,YSIZE=$(PREFIX)ROI2:SizeY_RBV,PORT=OVER1,ADDR=1,TIMEOUT=1")
dbLoadRecords("NDOverlayN.template","P=$(PREFIX),R=Over1:3:,NAME=ROI3,   SHAPE=1,O=Over1:,XPOS=$(PREFIX)ROI3:MinX_RBV,YPOS=$(PREFIX)ROI3:MinY_RBV,XSIZE=$(PREFIX)ROI3:SizeX_RBV,YSIZE=$(PREFIX)ROI3:SizeY_RBV,PORT=OVER1,ADDR=2,TIMEOUT=1")
dbLoadRecords("NDOverlayN.template","P=$(PREFIX),R=Over1:4:,NAME=ROI4,   SHAPE=1,O=Over1:,XPOS=$(PREFIX)ROI4:MinX_RBV,YPOS=$(PREFIX)ROI4:MinY_RBV,XSIZE=$(PREFIX)ROI4:SizeX_RBV,YSIZE=$(PREFIX)ROI4:SizeY_RBV,PORT=OVER1,ADDR=3,TIMEOUT=1")
dbLoadRecords("NDOverlayN.template","P=$(PREFIX),R=Over1:5:,NAME=Cursor1,SHAPE=1,O=Over1:,XPOS=junk,                  YPOS=junk,                  XSIZE=junk,                   YSIZE=junk,                   PORT=OVER1,ADDR=4,TIMEOUT=1")
dbLoadRecords("NDOverlayN.template","P=$(PREFIX),R=Over1:6:,NAME=Cursor2,SHAPE=1,O=Over1:,XPOS=junk,                  YPOS=junk,                  XSIZE=junk,                   YSIZE=junk,                   PORT=OVER1,ADDR=5,TIMEOUT=1")
dbLoadRecords("NDOverlayN.template","P=$(PREFIX),R=Over1:7:,NAME=Box1,   SHAPE=1,O=Over1:,XPOS=junk,                  YPOS=junk,                  XSIZE=junk,                   YSIZE=junk,                   PORT=OVER1,ADDR=6,TIMEOUT=1")
dbLoadRecords("NDOverlayN.template","P=$(PREFIX),R=Over1:8:,NAME=Box2,   SHAPE=1,O=Over1:,XPOS=junk,                  YPOS=junk,                  XSIZE=junk,                   YSIZE=junk,                   PORT=OVER1,ADDR=7,TIMEOUT=1")

# Create 2 color conversion plugins
NDColorConvertConfigure("CC1", $(QSIZE), 0, "$(PORT)", 0, 0, 0)
dbLoadRecords("NDColorConvert.template", "P=$(PREFIX),R=CC1:,  PORT=CC1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")
NDColorConvertConfigure("CC2", $(QSIZE), 0, "$(PORT)", 0, 0, 0)
dbLoadRecords("NDColorConvert.template", "P=$(PREFIX),R=CC2:,  PORT=CC2,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")

# Create a circular buffer plugin
NDCircularBuffConfigure("CB1", $(QSIZE), 0, "$(PORT)", 0, $(CBUFFS), 0)
dbLoadRecords("NDCircularBuff.template", "P=$(PREFIX),R=CB1:,  PORT=CB1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")

# Create an NDAttribute plugin with 8 attributes
NDAttrConfigure("ATTR1", $(QSIZE), 0, "$(PORT)", 0, 8, 0, 0, 0)
dbLoadRecords("NDAttribute.template",  "P=$(PREFIX),R=Attr1:,    PORT=ATTR1,ADDR=0,TIMEOUT=1,NCHANS=$(NCHANS),NDARRAY_PORT=$(PORT)")
dbLoadRecords("NDAttributeN.template", "P=$(PREFIX),R=Attr1:1:,  PORT=ATTR1,ADDR=0,TIMEOUT=1,NCHANS=$(NCHANS)")
dbLoadRecords("NDAttributeN.template", "P=$(PREFIX),R=Attr1:2:,  PORT=ATTR1,ADDR=1,TIMEOUT=1,NCHANS=$(NCHANS)")
dbLoadRecords("NDAttributeN.template", "P=$(PREFIX),R=Attr1:3:,  PORT=ATTR1,ADDR=2,TIMEOUT=1,NCHANS=$(NCHANS)")
dbLoadRecords("NDAttributeN.template", "P=$(PREFIX),R=Attr1:4:,  PORT=ATTR1,ADDR=3,TIMEOUT=1,NCHANS=$(NCHANS)")
dbLoadRecords("NDAttributeN.template", "P=$(PREFIX),R=Attr1:5:,  PORT=ATTR1,ADDR=4,TIMEOUT=1,NCHANS=$(NCHANS)")
dbLoadRecords("NDAttributeN.template", "P=$(PREFIX),R=Attr1:6:,  PORT=ATTR1,ADDR=5,TIMEOUT=1,NCHANS=$(NCHANS)")
dbLoadRecords("NDAttributeN.template", "P=$(PREFIX),R=Attr1:7:,  PORT=ATTR1,ADDR=6,TIMEOUT=1,NCHANS=$(NCHANS)")
dbLoadRecords("NDAttributeN.template", "P=$(PREFIX),R=Attr1:8:,  PORT=ATTR1,ADDR=7,TIMEOUT=1,NCHANS=$(NCHANS)")

set_requestfile_path("./")
set_requestfile_path("$(ADCORE)/ADApp/Db")
set_requestfile_path("$(ADCORE)/iocBoot")
set_savefile_path("./autosave")
set_pass0_restoreFile("auto_settings.sav")
set_pass1_restoreFile("auto_settings.sav")
save_restoreSet_status_prefix("$(PREFIX)")
dbLoadRecords("$(AUTOSAVE)/asApp/Db/save_restoreStatus.db", "P=$(PREFIX)")

# Optional: load NDPluginEdge plugin
#NDEdgeConfigure("EDGE1", $(QSIZE), 0, "$(PORT)", 0, 0, 0, 0)
#dbLoadRecords("NDEdge.template",  "P=$(PREFIX),R=Edge1:, PORT=EDGE1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")
#set_requestfile_path("$(ADPLUGINEDGE)/edgeApp/Db")

# Optional: load scan records
#dbLoadRecords("$(SSCAN)/sscanApp/Db/scan.db", "P=$(PREFIX),MAXPTS1=2000,MAXPTS2=200,MAXPTS3=20,MAXPTS4=10,MAXPTSH=10")
#set_requestfile_path("$(SSCAN)/sscanApp/Db")

# Optional: load sseq record for acquisition sequence
#dbLoadRecords("$(CALC)/calcApp/Db/yySseq.db", "P=$(PREFIX), S=AcquireSequence")
#set_requestfile_path("$(CALC)/calcApp/Db")

# Optional: load devIocStats records (requires DEVIOCSTATS module)
#dbLoadRecords("$(DEVIOCSTATS)/db/iocAdminSoft.db", "IOC=$(PREFIX)")

# Optional: load alive record (requires ALIVE module)
#dbLoadRecords("$(ALIVE)/aliveApp/Db/alive.db", "P=$(PREFIX),RHOST=192.168.1.254")

