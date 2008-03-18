rem This batch file starts the software for controlling Roper detectors from EPICS

rem Comment out the following line if you don't want the batch file to start WinView.
rem Edit it if WinSpec rather than WinView should be started
start WinView

rem Start the Roper server
start ..\..\bin\cygwin-x86\roperServer

rem Start MEDM
start medm -x -macro "P=roperCCD:, C=det1:" ccd.adl

rem Start IOC
..\..\bin\cygwin-x86\roperCCDApp st.cmd

