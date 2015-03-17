caput 13SIM1:cam1:SizeX 10
caput 13SIM1:cam1:SizeY 10
caput 13SIM1:cam1:AcquireTime .001
caput 13SIM1:cam1:AcquirePeriod 0.1
caput 13SIM1:cam1:ImageMode "Continuous"
caput 13SIM1:cam1:ArrayCallbacks "Enable"
caput -S 13SIM1:cam1:NDAttributesFile "./test3_attributes.xml"
caput 13SIM1:cam1:Acquire 1
caput 13SIM1:HDF1:EnableCallbacks Enable
caput -S 13SIM1:HDF1:FilePath "./"
caput -S 13SIM1:HDF1:FileName "test3"
caput -S 13SIM1:HDF1:FileTemplate "%s%s_%3.3d.h5"
caput -S 13SIM1:HDF1:XMLFileName "./test3_layout.xml"
caput 13SIM1:HDF1:AutoIncrement 1
caput 13SIM1:HDF1:FileNumber 1
caput 13SIM1:HDF1:FileWriteMode "Stream"
caput 13SIM1:HDF1:NumCapture "10"
caput 13SIM1:HDF1:Capture 1
sleep 5
h5dump test3_001.h5 > test3_001.txt

