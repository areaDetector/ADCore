; This program uses the simDetector and NDFileTIFF plugin to create TIFF files with each of the supported
; areaDetector data types.

; It then reads those files into IDL and displays information on the array

cd, '/home/epics/scratch'
cam  = '13SIM1:cam1:'
file = '13SIM1:TIFF1:'
data_types = ['Int8', $
              'UInt8', $
              'Int16', $
              'UInt16', $
              'Int32', $
              'UInt32', $
              'Float32', $
              'Float64']

for i=0, 7 do begin
   t = caput(cam+'DataType', data_types[i])
   t = caput(file+'FileName', [byte(data_types[i]),0])
   t = caput(cam+'Acquire', 1)
   wait, 1
   data = read_tiff(data_types[i]+'_1.tiff')
   help, data
endfor
end
