; This program tests the NDDriverStdArrays driver by writing writing to
; EPICS PVs to cause the driver to run

prefix = '13NDSA1:cam1:'

; Start acquire

nDimensions = 2
maxDimensions = 10
dimensions = intarr(maxDimensions)

xSize = 512
ySize = 512
dimensions[0] = xSize
dimensions[1] = ySize
dataType = 'Float32'
colorMode = 'Mono'
appendMode = 'Disable'
imageMode = 'Continuous'

t = caput(prefix + 'NDimensions', nDimensions)
t = caput(prefix + 'Dimensions', dimensions)
t = caput(prefix + 'DataType', dataType)
t = caput(prefix + 'ColorMode', colorMode)
t = caput(prefix + 'AppendMode', appendMode)
t = caput(prefix + 'ImageMode', imageMode)
t = caput(prefix + 'Acquire', 1)

for i=1, 100 do begin
  data = 100*sin(shift(dist(xSize, ySize)/i, xsize/2, ysize/2))
  t = caput(prefix + 'ArrayIn', data)
  wait, .1
endfor

end
