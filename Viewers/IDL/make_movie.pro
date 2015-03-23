pro make_movie, vol, min=min, max=max, wait=wait, scale=scale, index=index, $
                start=start, stop=stop, step=step, mpeg_file=mpeg_file, mp4_file=mp4_file, $
                jpeg_file=jpeg_file, tiff_file=tiff_file, label=label, quality=quality, $
                fps=fps, bps=bps, unscaled_tiff=unscaled_tiff, $
                color=color, file=file, buffer_size=buffer_size, window=window, $
                abort_widget=abort_widget, status_widget=status_widget


;+
; NAME:
;   MAKE_MOVIE
;
; PURPOSE:
;   This procedure plays a 3-D array as a movie either:
;       - On the screen
;       - To an MPEG file on disk (deprecated)
;       - To an MP4 file on disk
;       - To a series of JPEG files on disk
;       - To a series of TIFF files on disk
;
; CATEGORY:
;   Image display.
;
; CALLING SEQUENCE:
;   MAKE_MOVIE, Volume
;
; INPUTS:
;   Volume: A 3-D array of any numeric data type.  This input is ignored if the FILE
;           keyword is used.
;
; KEYWORD PARAMETERS:
;   FILE:
;       The name of a file to be used as input, rather than passing the Volume array.
;       This file is read with READ_TOMO_VOLUME().
;   BUFFER_SIZE:
;       When reading a large file from disk with the FILE keyword, this keyword
;       controls the size of the memory buffer used for reading the file.  If the size
;       of the file is larger than the buffer size then the file is read in "chunks".
;       This allows displaying files larger than the physical or virtual memory of
;       the system would otherwise permit.  The units are voxels, not bytes.
;       The default is to read the entire file.
;   MIN:
;       The minimum value of the data display range.  This sets the low end
;       of the grey scale range.  All values less than this will be appear
;       black when using a linear grey scale.  The default is the minimum value
;       of the data in the entire Volume array.
;
;   MAX:
;       The maximum value of the data display range.  This sets the high end
;       of the grey scale range.  All values greater than this will be appear
;       white when using a linear grey scale.  The default is the maximum value
;       of the data in the entire Volume array.
;
;   SCALE:
;       A scale factor to increase or decrease the size of the displayed images
;       relative to the size of the Volume array.  SCALE=2 will double the
;       size of the displayed images, SCALE=3 will display the images at 3
;       times their normal size, etc.  SCALE=-2 will display the images at
;       half their normal size, SCALE=-3 one-third their normal size, etc.
;       The default is 1, i.e. no scaling.
;
;   INDEX:
;       This keyword controls which dimension of the Volume array is animated.
;       Allowed values are 1, 2 and 3.  The following table shows the result
;       of setting this keyword:
;
;           INDEX   frame[i]
;             1   Volume[i,*,*]
;             2   Volume[*,i,*]
;             3   Volume[*,*,i]
;       The default is 3, i.e. the last dimension of the Volume array is the
;       one which is animated.
;
;   START:
;       The first frame to display.  The default is 0.
;
;   STOP:
;       The last frame to display.  The default is the highest value of the
;       selected index.
;
;   STEP:
;       The array increment from one frame to the next.  The default is 1.
;
;   WAIT:
;       The delay time from one frame to the next in floating point seconds.
;       The default is 0., i.e. frames are diplayed as fast as possible.
;       This keyword has no effect if the JPEG_FILE or MPEG_FILE keywords are used.
;
;   WINDOW:
;       The window number if the movie is being displayed on the screen.  The
;       window will be created to be exactly the right size to display the movie.
;       The default is 0.
;
;   MPEG_FILE:
;       The name of an MPEG file to which the output should be written.  If
;       this keyword is used then this procedure does not display its output on
;       the screen but rather creates an MPEG file.
;       NOTE: IDL requires a special (free) license upgrade to write MPEG
;       files.  See the IDL Web site for details.
;
;   MP4_FILE:
;       The name of an MP4 file to which the output should be written.  If
;       this keyword is used then this procedure does not display its output on
;       the screen but rather creates an MP4 file.
;       MP4 files are written using the IDL WRITE_VIDEO procedure which was added
;       in IDL 8.2.3.
;
;   JPEG_FILE:
;       The base name of an JPEG file to which the output should be written.  If
;       this keyword is used then this procedure does not display its output on
;       the screen but rather creates a series of JPEG files. A sequence number and
;       the extension .jpg are appended to this base name. The file names are thus
;       of the form 'my_jpeg_name_0001.jpg', 'my_jpeg_name_0002.jpg', etc.  The system
;       variable !order is used to determine whether the files are written bottom to
;       top or top to bottom.
;
;   TIFF_FILE:
;       The base name of an TIFF file to which the output should be written.  If
;       this keyword is used then this procedure does not display its output on
;       the screen but rather creates a series of TIFF files. A sequence number and
;       the extension .tif are appended to this base name. The file names are thus
;       of the form 'my_jpeg_name_0001.tif', 'my_jpeg_name_0002.tif', etc.  The system
;       variable !order is used to determine whether the files are written bottom to
;       top or top to bottom.
;       
;   UNSCALED_TIFF
;       Set this flag to write unscaled TIFF files.
;       If this flag is not set then the TIFF files are scaled to 8-bits using the MIN and MAX.
;       If this flag is set then the TIFF files are unscaled, and their data type will be the same
;       as the input VOL array, for example 16-bit signed integers.
;
;   QUALITY:
;       The quality factor for JPEG and MPEG files.
;       See the help for WRITE_JPEG for more information on using this
;       keyword for JPEG files.
;       See the help for MPEG_OPEN for more information on using this
;       keyword for MPEG files.  For MPEG files this keyword is only allowed
;       for IDL 5.4 and higher.
;       Default=90.
;       
;   FPS
;       The Frames Per Second for MP4 files.  This is passed as the VIDEO_FPS keyword to WRITE_VIDEO.
;       The default is 30.
;       
;   BPS
;       The bits per second MP4 files.  This is passed as the BIT_RATE keyword to WRITE_VIDEO.
;       This keyword controls the quality and size of the MP4 file.  Larger values (e.g. 1e6) produce
;       larger files with higher quality, while smaller values (e.g. 1e4) produce smaller files with lower
;       quality.
;       Default=1e4. For typical 16-bit tomography data this produces a file which is about 0.7% of the size of the original
;       with reasonably good quality.  BPS=1e5 produces a file that is about 7% of the original size with excellent quality.
;
;   COLOR
;       A flag which indicates that the JPEG, MPEG, or MP4 output files should be in color,
;       using the current color table, rather than black and white.
;
;   LABEL:
;       A flag which indicates that the current frame number should be drawn
;       as a label on the screen.  This is useful for determining which frames
;       interesting features appear in, for use later with IMAGE_DISPLAY
;       or other routines.
;
;   STATUS_WIDGET:
;       The widget ID of a text widget used to display the status of the
;       preprocessing operation.  If this is a valid widget ID then
;       informational messages will be written to this widget.
;
;   ABORT_WIDGET
;       The widget ID of a widget used to abort the preprocessing operation.
;       If this is a valid widget ID then the "uvalue" of this widget will be
;       checked periodically.  If it is 1 then this routine will clean up and
;       return immediately.
;
; OUTPUTS:
;   This procedure does not return any output to IDL.  It displays images on
;   the screen using the IDL TV procedure, or writes output to disk if JPEG_FILE, MPEG_FILE
;   or MP4_FILE is used.
;
; SIDE EFFECTS:
;   Displays images on the screen using the IDL TV procedure, or writes output
;   to disk if JPEG_FILE, JPEG_FILE, or MP4_FILE is used.
;
; PROCEDURE:
;   This procedure converts each frame to an 8-bit array using the IDL BYTSCL
;   function and then either displays it on the screen using the TV procedure,
;   writes it to an MPEG file, an MP4 file, or to a series of JPEG or TIFF files.
;
; EXAMPLE:
;   ; Display an array as a movie scanning through the first dimension of the
;   ; array, forcing the black level to be 1000, and waiting 0.2 seconds
;   ; between frames
;   MAKE_MOVIE, Volume, index=1, wait=.2, min=1000
;
;   ; Create an MPEG movie of an array scanning through the last dimension
;   ; (default) of the array, forcing the white level to be 10000.
;   MAKE_MOVIE, Volume, max=10000, mpeg_file='movie1.mpeg'
;
;   ; Create a series of JPEG files of an array scanning through the last dimension
;   ; (default) of the array, forcing the white level to be 10000.
;   MAKE_MOVIE, Volume, max=10000, jpeg_file='my_jpegs'
;
;   ; Create a series of JPEG files of disk file scanning through the first dimension
;   ; of the array, forcing the white level to be 5000.  Set a buffer size of
;   ; 200,000,000 voxels (400MB for 16 bit data)
;   MAKE_MOVIE, file='my_volume.dat', max=5000, index=1, jpeg_file='my_volume_X', $
;                     buffer_size=2e8
;
; MODIFICATION HISTORY:
;   Written by:     Mark Rivers, April 26, 1999.  Merged previous routines
;                   TOMO_MOVIE and WRITE_MPEG
;   June 27, 2000  MLR  Changed position of label slightly for legibility
;   Sep. 29, 2000  MLR  Added JPEG_FILE and QUALITY keywords.
;   Feb. 27, 2001  MLR  Added COLOR keyword for color JPEG and MPEG files.
;                       Made QUALITY keyword apply to MPEG files if the IDL version
;                       is 5.4 or higher.
;   Nov. 5, 2001   MLR  Added the FILE and BUFFER_SIZE keywords.
;   Nov. 22, 2001  MLR  Added the ABORT_WIDGET and STATUS_WIDGET keywords
;   Nov. 24, 2001  MLR  Added the WINDOW keyword
;   Apr. 26, 2002  MLR  Worked around bug in MPEG_PUT, movies were upside down relative
;                       to on-screen and JPEG files.
;   Jan. 9,  2004  MLR  Added TIFF_FILE keyword
;   Nov. 30, 2004  MLR  Put MPEG and JPEG output through lookup table if COLOR=0.
;-

if (n_elements(wait) eq 0) then wait=0
if (n_elements(scale) eq 0) then scale=1
if (n_elements(index) eq 0) then index=3
if (n_elements(status_widget) eq 0) then status_widget = -1L
if (n_elements(abort_widget) eq 0) then abort_widget = -1L
if (n_elements(fps) eq 0) then fps = 30
if (n_elements(bps) eq 0) then bps = 1e4
if (n_elements(unscaled_tiff) eq 0) then unscaled_tiff = 0

if (keyword_set(file)) then begin
    file_mode = 1
    if (n_elements(buffer_size) ne 0) then begin
        ; Read parts of file to determine nx, ny, nz
        v = read_tomo_volume(file, zrange=[0,0])
        dims = size(v, /dimensions)
        nx = dims[0]
        ny = dims[1]
        v = read_tomo_volume(file, xrange=[0,0])
        dims = size(v, /dimensions)
        nz = dims[2]
    endif else begin
        ; buffer_size not set, read entire file
        vol = read_tomo_volume(file)
    endelse
endif else begin
    file_mode = 0
endelse

if (n_elements(buffer_size) eq 0) then begin
    nx = n_elements(vol[*,0,0])
    ny = n_elements(vol[0,*,0])
    nz = n_elements(vol[0,0,*])
    buffer_size = long64(nx)*ny*nz
endif

case index of
   1: begin
      ncols=ny
      nrows=nz
      nframes=nx
      buff_frames = buffer_size/(ny*nz)
      if (file_mode) then begin
         vol = read_tomo_volume(file, xrange=[0, (buff_frames < (nx-1))])
      endif
   end
   2: begin
      ncols=nx
      nrows=nz
      nframes=ny
      buff_frames = buffer_size/(nx*nz)
      if (file_mode) then begin
         vol = read_tomo_volume(file, yrange=[0, (buff_frames < (ny-1))])
      endif
   end
   3: begin
      ncols=nx
      nrows=ny
      nframes=nz
      buff_frames = buffer_size/(nx*ny)
      if (file_mode) then begin
         vol = read_tomo_volume(file, zrange=[0, (buff_frames < (nz-1))])
      endif
  end
   else: message, 'INDEX must be in the range 1-3'
endcase

if (n_elements(min) eq 0) then min=min(vol)
if (n_elements(max) eq 0) then max=max(vol)

if (scale gt 1) then begin
    last_col = ncols - 1
    last_row = nrows - 1
    ncols = ncols * fix(scale)
    nrows = nrows * fix(scale)
endif
if (scale lt -1) then begin
    iscale = fix(abs(scale))
    last_col = (ncols/iscale)*iscale - 1
    last_row = (nrows/iscale)*iscale - 1
    ncols = ncols / iscale
    nrows = nrows / iscale
endif

if (n_elements(start) eq 0) then start=0
if (n_elements(stop) eq 0) then stop=nframes-1
stop = stop < (nframes-1)
if (n_elements(step) eq 0) then step=1
if (n_elements(quality) eq 0) then quality=90
if (n_elements(order) eq 0) then order=0
if (n_elements(window) eq 0) then window=0

if (n_elements(mp4_file) ne 0) then begin
    if (!version.release lt '8.2') then begin
        message, 'MP4_FILE only supported in IDL 8.2.3 and later'
    endif else begin
        mp4_mode = 1
        ; WRITE_VIDEO requires color input, so set the color keyword even if it was not set
        color = 1
    endelse
endif else begin
    mp4_mode = 0
endelse

if (n_elements(mpeg_file) ne 0) then begin
  if (!version.release lt '5.4') then begin
    mpegid = mpeg_open([ncols, nrows], file=mpeg_file)
  endif else begin
    mpegid = mpeg_open([ncols, nrows], file=mpeg_file, quality=quality)
  endelse
  mpeg_mode = 1
endif else begin
  mpeg_mode = 0
endelse

if (n_elements(jpeg_file) ne 0) then begin
    jpeg_mode = 1
endif else begin
    jpeg_mode = 0
endelse

if (n_elements(tiff_file) ne 0) then begin
    tiff_mode = 1
endif else begin
    tiff_mode = 0
endelse

if ((jpeg_mode eq 0) and (tiff_mode eq 0) and (mp4_mode eq 0) and (mpeg_mode eq 0)) then begin
   window, window, xsize=(ncols>100), ysize=(nrows>100)
endif

frame_index=start
for frame=start, stop, step do begin
    if ((file_mode) and (frame_index ge buff_frames)) then begin
        vol=0   ; Free up memory
        case index of
            1: vol = read_tomo_volume(file, $
                        xrange=[frame, ((frame+buff_frames) < (nx-1))])
            2: vol = read_tomo_volume(file, $
                        yrange=[frame, ((frame+buff_frames) < (ny-1))])
            3: vol = read_tomo_volume(file, $
                        zrange=[frame, ((frame+buff_frames) < (nz-1))])
        endcase
        frame_index=0
    endif
    case index of
        1: temp=vol[frame_index,*,*]
        2: temp=vol[*,frame_index,*]
        3: temp=vol[*,*,frame_index]
    endcase
    frame_index = frame_index+step
    temp = reform(temp, /overwrite)
    if ((tiff_mode eq 0) or (unscaled_tiff eq 0)) then temp = bytscl(temp, min=min, max=max)
    if (scale ne 1) then temp = rebin(temp[0:last_col, 0:last_row], $
                                      ncols, nrows)
    ; The code for handling color was lifted out of the IDL procedure MPEG_PUT.
    if (jpeg_mode or mpeg_mode or mp4_mode) then begin
        tvlct, red, green, blue, /get
        if (keyword_set(color)) then begin
            ; Apply color tables and create an (ncols, 3 * nrows) array:

            temp = [[red[temp]], [green[temp]], [blue[temp]]]
            ; Reform to (ncols * nrows, 3) and then transpose to interleave color as
            ; first dimension.
            temp = transpose(reform(temp, ncols * nrows, 3, /overwrite))
            ; Now back to (3, ncols, nrows)
            temp = reform(temp, 3, ncols, nrows, /overwrite)
        endif else begin
            ; Not color, but put image through red lookup table for gamma and inversion
            temp = red[temp]
        endelse
    endif
    str = 'Frame = ' + strtrim(frame,2) + '/' + strtrim(stop,2)
    if (mp4_mode) then begin
      print, str
      write_video, mp4_file, temp, handle=handle, video_fps=fps, bit_rate=bps
    endif else if (mpeg_mode) then begin
        print, str
        ; Note, there appears to be a bug in mpeg_put, the order keyword
        ; is the opposite of the documentation, and the behaviour of TV and WRITE_JPEG.
        mpeg_put, mpegid, frame=frame, image=temp, order=1-!order
    endif else if (jpeg_mode) then begin
        num = string(frame+1, format='(i4.4)')
        jfile = jpeg_file + '_' + num + '.jpg'
        str = str + ' (JPEG file=' + jfile + ')'
        print, str
        if (keyword_set(color)) then true=1 else true=0
        write_jpeg, jfile, temp, quality=quality, order=!order, true=true
    endif else if (tiff_mode) then begin
        num = string(frame+1, format='(i4.4)')
        tfile = tiff_file + '_' + num + '.tif'
        str = str + ' (TIFF file=' + tfile + ')'
        print, str
        if (unscaled_tiff) then begin
            tname = size(temp, /tname)
            short = 0
            long = 0
            float = 0
            double = 0
            complex = 0
            dcomplex = 0
            signed = 1
            case tname of
                'BYTE':     signed = 0
                'INT':      short = 1
                'UINT':     begin
                                int = 1
                                signed = 0
                            end
                'LONG':     long = 1
                'ULONG':    begin
                                long = 1
                                signed = 0
                            end
                'FLOAT':    float = 1
                'DOUBLE':   double = 1
                'COMPLEX':  complex = 1
                'DCOMPLEX': dcomplex = 1
                ELSE: message, 'Unsupported TIFF data type = ', tname
            endcase
            write_tiff, tfile, temp, orientation=1-!order, $
                        short=short, long=long, float=float, double=double, complex=complex, dcomplex=dcomplex, signed=signed
        endif else begin
            write_tiff, tfile, temp, orientation=1-!order
        endelse
    endif else begin
        tv, temp
        if (keyword_set(label)) then begin
            ypos = n_elements(temp(0,*)) - 15
            xyouts, 0, ypos, strtrim(frame,2), /device
        endif
        wait, wait
    endelse
    if (widget_info(status_widget, /valid_id)) then $
        widget_control, status_widget, set_value=str
    if (widget_info(abort_widget, /valid_id)) then begin
        event = widget_event(/nowait, abort_widget)
        widget_control, abort_widget, get_uvalue=abort
        if (abort) then begin
            if (widget_info(status_widget, /valid_id)) then $
                widget_control, status_widget, set_value='Movie aborted'
            return
        endif
    endif
endfor

if (mpeg_mode) then begin
    if (widget_info(status_widget, /valid_id)) then $
        widget_control, status_widget, set_value='Closing MPEG file ...'
    mpeg_save, mpegid, file=mpeg_file
    mpeg_close, mpegid
endif

if (mp4_mode) then begin
    if (widget_info(status_widget, /valid_id)) then $
        widget_control, status_widget, set_value='Closing MP4 file ...'
    write_video, file, handle=handle, /close
endif

if (widget_info(status_widget, /valid_id)) then $
    widget_control, status_widget, set_value='Movie complete'

end
