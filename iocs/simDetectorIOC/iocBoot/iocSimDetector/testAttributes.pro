; This program tests saving detector attributes to netCDF, Nexus, and HDF5 files
; simDetectorAttributes.xml and NexusTemplate.xml are both configured to save the value
; of the AcquireTime PV.  This IDL programs tests to make sure it saves the current
; value of that PV, not a stale value.

pro doTest, filePlugin, fileName
    ; Stop acquisition
    t = caput('13SIM1:cam1:Acquire', 0)
    
    ; Set acquire time to 0.01
    t = caput('13SIM1:cam1:AcquireTime', 0.01)
    
    ; Set small detector size to make it easier to dump the files in ASCII
    t = caput('13SIM1:cam1:SizeX', 100)
    t = caput('13SIM1:cam1:SizeY', 100)
    
    ; Set ImageMode to Multiple
    t = caput('13SIM1:cam1:ImageMode', 'Multiple')
    
    ; Set NumImages to 10
    t = caput('13SIM1:cam1:NumImages', 10)
    
    ; Set the file plugins to Single mode for first test.
    t = caput('13SIM1:'+filePlugin+':EnableCallbacks', 'Enable')
    t = caput('13SIM1:'+filePlugin+':AutoSave', 'No')
    t = caput('13SIM1:'+filePlugin+':AutoIncrement', 'Yes')
    t = caput('13SIM1:'+filePlugin+':FilePath', [byte('./'),0B])
    t = caput('13SIM1:'+filePlugin+':FileName', [byte(fileName),0B])
    t = caput('13SIM1:'+filePlugin+':FileNumber', 1)
    t = caput('13SIM1:'+filePlugin+':FileTemplate', [byte('%s%s_%d.hdf'),0B])
    t = caput('13SIM1:'+filePlugin+':FileWriteMode', 'Single')
    
    ; Start acquisition
    t = caput('13SIM1:cam1:Acquire', 1)
    ; Wait for it to get done.
    repeat begin
      wait, 0.1
      t = caget('13SIM1:cam1:Acquire', acquire)
    endrep until (acquire eq 0)
    
    ; Save file
    t = caput('13SIM1:'+filePlugin+':WriteFile', 1)
    
    ; Now switch plugin to stream mode and start streaming
    t = caput('13SIM1:'+filePlugin+':FileWriteMode', 'Stream')
    t = caput('13SIM1:'+filePlugin+':NumCapture', 10)
    t = caput('13SIM1:'+filePlugin+':Capture', 1)
    
    ; Change acquire time to 0.02
    t = caput('13SIM1:cam1:AcquireTime', 0.02)
    
    ; Start acquisition
    t = caput('13SIM1:cam1:Acquire', 1)
    ; Wait for it to get done.
    repeat begin
      wait, 0.1
      t = caget('13SIM1:cam1:Acquire', acquire)
    endrep until (acquire eq 0)
end

doTest, 'Nexus1',  'Nexus_test'
doTest, 'netCDF1', 'netCDF_test'
doTest, 'HDF1',    'HDF_test'
end
