; This program tests the NDDriverStdArrays driver by writing writing to
; EPICS PVs to cause the driver to run
; This version appends to the previous data

prefix = '13NDSA1:cam1:'

; Start acquire

nDimensions = 2
maxDimensions = 10
dimensions = intarr(maxDimensions)

xSize = 512
ySize = 512
xStep = 8
yStep = 1
dimensions[0] = xSize
dimensions[1] = ySize
dataType = 'Float32'
colorMode = 'Mono'
appendMode = 'Enable'
imageMode = 'Continuous'

; Note: we nust send contiguous pixels so yStep must be 1 if xStep is not xSize
if (xStep ne xSize) then yStep = 1

t = caput(prefix + 'NDimensions', nDimensions)
t = caput(prefix + 'Dimensions', dimensions)
t = caput(prefix + 'DataType', dataType)
t = caput(prefix + 'ColorMode', colorMode)
t = caput(prefix + 'AppendMode', appendMode)
t = caput(prefix + 'NextElement', 0)
t = caput(prefix + 'ImageMode', imageMode)
t = caput(prefix + 'Acquire', 1)

for i=1, 10 do begin
  t = caput(prefix + 'NewArray', 1)
  data = 100*sin(shift(dist(xSize, ySize)/i, xsize/2, ysize/2))
  for j=0, ySize-yStep, yStep do begin
    for k=0, xSize-xStep, xStep do begin
      t = caput(prefix + 'ArrayIn', data[k:k+xStep-1,j:j+yStep-1])
      wait, .001
    endfor
  endfor
  t = caput(prefix + 'ArrayComplete', 1)  
  wait, .1
endfor

end
