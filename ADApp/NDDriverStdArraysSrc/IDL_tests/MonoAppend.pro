; This program tests the NDDriverStdArrays driver by writing writing to
; EPICS PVs to cause the driver to run
; This version appends to the previous data

prefix = '13NDSA1:cam1:'

; Start acquire

nDimensions = 2
dimensions = intarr(nDimensions)

xSize = 512
ySize = 512
yStep = 1
dimensions[0] = xSize
dimensions[1] = ySize
dataType = 'Float32'
colorMode = 'Mono'
appendMode = 'Enable'
imageMode = 'Continuous'

t = caput(prefix + 'NDimensions', nDimensions)
t = caput(prefix + 'Dimensions', dimensions)
t = caput(prefix + 'DataType', dataType)
t = caput(prefix + 'ColorMode', colorMode)
t = caput(prefix + 'AppendMode', appendMode)
t = caput(prefix + 'NextElement', 0)
t = caput(prefix + 'ImageMode', imageMode)
t = caput(prefix + 'Acquire', 1)

for i=1, 10 do begin
  data = 100*sin(shift(dist(xSize, ySize)/i, xsize/2, ysize/2))
  for j=0, ySize-yStep, yStep do begin
     t = caput(prefix + 'ArrayIn', data[*,j:j+yStep-1])
     ;wait, .001
  endfor
  wait, .1
endfor

end
