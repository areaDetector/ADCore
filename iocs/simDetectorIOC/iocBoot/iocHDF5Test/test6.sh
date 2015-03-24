caput 13SIM1:cam1:SizeX 10
caput 13SIM1:cam1:SizeY 10
caput 13SIM1:cam1:AcquireTime .001
caput 13SIM1:cam1:AcquirePeriod 0.1
caput 13SIM1:cam1:ImageMode "Continuous"
caput 13SIM1:cam1:ArrayCallbacks "Enable"
caput 13SIM1:cam1:Acquire 1
caput 13SIM1:HDF1:EnableCallbacks Enable
caput -S 13SIM1:HDF1:FilePath "./"
caput -S 13SIM1:HDF1:FileName "test6"
caput -S 13SIM1:HDF1:FileTemplate "%s%s_%3.3d.h5"
caput 13SIM1:HDF1:FileNumber 1
caput 13SIM1:HDF1:FileWriteMode "Single"
caput 13SIM1:HDF1:NumCapture "1"
caput 13SIM1:HDF1:AutoSave 1
sleep 1
caput 13SIM1:HDF1:AutoSave 0
sleep 5
h5dump test6_001.h5 > test6_001.txt

