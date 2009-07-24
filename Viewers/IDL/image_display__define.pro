pro image_display::write_output_file, format=format,out=out,noprompt=noprompt

    oform = 'jpeg'
    if (keyword_set(format) ne 0 ) then oform = format
    oform = strlowcase(oform)

    if (self.image_window.order EQ 0) then begin
        self->dc_to_ic, 0, 0, first_col, first_row, /clip
        self->dc_to_ic, self.image_window.x_size-1, self.image_window.y_size-1, $
                        last_col, last_row, /clip
    endif else begin
      self->dc_to_ic, 0, self.image_window.y_size-1, first_col, first_row, /clip
      self->dc_to_ic, self.image_window.x_size-1, 0, last_col, last_row, /clip
    endelse
    if (self.image_window.order EQ 0) then begin
        self->ic_to_dc, first_col, first_row, x_offset, y_offset
        x_offset = (x_offset - self.image_window.x_zoom/2.) > 0
        y_offset = (y_offset - self.image_window.y_zoom/2.) > 0
    endif else begin
        self->ic_to_dc, first_col, last_row, x_offset, y_offset
        x_offset = (x_offset - self.image_window.x_zoom/2.) > 0
        y_offset = (y_offset - self.image_window.y_zoom/2.) > 0
    endelse

    nc = last_col - first_col + 1
    nr = last_row - first_row + 1
    if self.image_window.zoom_mode eq 0 then sample=1 else sample=0
    wset, self.image_window.winid

    buff = rebin((*self.image_data.display_buff)(first_col:last_col, first_row:last_row), $
                  nc*self.image_window.x_zoom, nr*self.image_window.y_zoom, sample=sample)

    ncout = nc*self.image_window.x_zoom
    nrout = nr*self.image_window.y_zoom
    a     = fltarr(ncout,nrout)
    a     = float(buff)
    tr    = bytarr(ncout, nrout)
    tg    = bytarr(ncout, nrout)
    tb    = bytarr(ncout, nrout)
    ta    = bytarr(ncout, nrout, 3)
    tvlct, r, g, b, /get
    for i=0,ncout-1 do for j=0,nrout-1 do tr[i, j] = r[a[i,j]]
    for i=0,ncout-1 do for j=0,nrout-1 do tg[i, j] = g[a[i,j]]
    for i=0,ncout-1 do for j=0,nrout-1 do tb[i, j] = b[a[i,j]]
    ta[*,*,0]=tr
    ta[*,*,1]=tg
    ta[*,*,2]=tb

    outfile=''
    df     = self.image_data.handle
    ; MN 13 Aug 2001
    ; remove '/' characters from image name
    ino    = strpos(df,'/')
    while (ino ge 0) do begin
        strput, df, '_', ino
        ino = strpos(df,'/')
    endwhile

    ; force output choice of jpg,png,bmp,tif:
    if (oform eq 'jpeg') then oform='jpg'
    if (oform eq 'gif')  then oform='png'
    if (oform eq 'tiff') then oform='tif'
    ffilter =     '*.' + oform
    def_out = df + '.' + oform

    do_prompt = 1
    if (keyword_set(out)) then begin
        do_prompt = 0
        def_out = out
     endif
    if (keyword_set(noprompt)) then do_prompt = 0
    if do_prompt ne 0 then begin
       outfile = dialog_pickfile(filter=ffilter, file=def_outt, /write, $
                                 title='Select Output Image File')
    endif else begin
       outfile = def_out
    endelse

    if (outfile ne '') then begin
       case oform of
          'jpg':  write_jpeg, outfile, ta, true=3, order=self.image_window.order
          'gif':  write_png,  outfile, buff, r, g, b, order=self.image_window.order
          'png':  write_png,  outfile, buff, r, g, b, order=self.image_window.order
          'bmp':  write_bmp,  outfile, ta
          'tif':  write_tiff, outfile, buff, red=r, green=g, blue=b, orientation=self.image_window.order
          else:   outfile=''
       endcase
       print, 'wrote ', outfile
    endif  else begin
       print, 'no output written'
    endelse

    return
end
;
;------------------
pro image_display::init_plot_windows, leave_mouse=leave_mouse

    if (keyword_set(leave_mouse)) then begin
        cursor, x_mouse, y_mouse, /nowait, /device
    endif else begin
        x_mouse=self.image_window.x_size/2
        y_mouse=self.image_window.y_size/2
        tvcrs, x_mouse, y_mouse
    endelse
    wset, self.image_window.winid
    self->dc_to_ic, x_mouse, y_mouse, xc, yc, /clip
    self->dc_to_ic, 0, 0, x_min, y_min, /clip
    self->dc_to_ic, self.image_window.x_size-1, self.image_window.y_size-1, $
                x_max, y_max, /clip
    if (self.image_window.order eq 1) then begin
        temp = y_max
        y_max = y_min
        y_min = temp
    endif

    x_axis = *self.image_data.x_dist
    y_axis = *self.image_data.y_dist
    black_level = self.image_window.black_level
    white_level = self.image_window.white_level

    diff = white_level - black_level
    crange = [(black_level - 0.1*diff), (white_level + 0.1*diff)]
    diff = x_axis(x_max) - x_axis(x_min)
    xrange = [(x_axis(x_min) - 0.05*diff), (x_axis(x_max) + 0.05*diff)]
    diff = y_axis(y_max) - y_axis(y_min)
    yrange = [(y_axis(y_min) - 0.05*diff), (y_axis(y_max) + 0.05*diff)]
    if (self.image_window.order eq 1) then yrange=reverse(yrange)
    wset, self.row_plot_window.winid
    plot, x_axis(x_min:x_max), (*self.image_data.raw_data)(x_min:x_max, yc), $
            /nodata, $
            xrange = xrange, yrange=crange, xstyle=1, $
            position=[.15, .15, .95, .95], $
            charsize=.65
    self.row_plot_window.x = !x
    self.row_plot_window.y = !y
    self.row_plot_window.p = !p
    wset, self.col_plot_window.winid
    plot, (*self.image_data.raw_data)(xc, y_min:y_max), $
            y_axis(y_min:y_max), /nodata, $
            xrange=crange, yrange=yrange, ystyle=1, $
            position=[.20, .10, .95, .97], $
            charsize=.65
    self.col_plot_window.x = !x
    self.col_plot_window.y = !y
    self.col_plot_window.p = !p
    self.image_window.x_min = x_min
    self.image_window.x_max = x_max
    self.image_window.y_min = y_min
    self.image_window.y_max = y_max
    self.image_window.x_prev = xc
    self.image_window.y_prev = yc
    self->plot_profiles, x_mouse, y_mouse
end


pro image_display::plot_profiles, x_mouse, y_mouse
    xoldc = self.image_window.x_prev
    yoldc = self.image_window.y_prev
    wset, self.image_window.winid
    self->dc_to_ic, x_mouse, y_mouse, xc, yc, /clip
    self.XYtable[0].pixel = xc
    self.XYtable[1].pixel = yc
    self.XYtable[2].pixel = 0
    self.XYtable[0].user = (*self.image_data.x_dist)[xc]
    self.XYtable[1].user = (*self.image_data.y_dist)[yc]
    self.XYtable[2].user = (*self.image_data.raw_data)[xc, yc]
    self.XYtable[0].screen = x_mouse
    self.XYtable[1].screen = y_mouse
    self.XYtable[2].screen = (*self.image_data.display_buff)[xc, yc]
    widget_control, self.widgets.XYtable, set_value=self.XYtable
    wset, self.row_plot_window.winid
    !x = self.row_plot_window.x
    !y = self.row_plot_window.y
    !p = self.row_plot_window.p
    x_axis = *self.image_data.x_dist
    y_axis = *self.image_data.y_dist
    x_min = self.image_window.x_min
    x_max = self.image_window.x_max
    y_min = self.image_window.y_min
    y_max = self.image_window.y_max
    black_level = self.image_window.black_level
    white_level = self.image_window.white_level
    oplot, x_axis(x_min:x_max), $
        (*self.image_data.raw_data)(x_min:x_max, yoldc), $
        psym=10, color=self.colors.erase
    plots, [x_axis(xoldc), x_axis(xoldc)], [black_level, white_level], $
        color=self.colors.erase
    oplot, x_axis(x_min:x_max), (*self.image_data.raw_data)(x_min:x_max, yc), $
        psym=10, color=self.colors.plot
    plots, [x_axis(xc), x_axis(xc)], [black_level, white_level], $
        color=self.colors.line
    wset, self.col_plot_window.winid
    !x = self.col_plot_window.x
    !y = self.col_plot_window.y
    !p = self.col_plot_window.p
    oplot, (*self.image_data.raw_data)(xoldc, y_min:y_max), $
            y_axis(y_min:y_max), $
            psym=10, color=self.colors.erase
    plots, [black_level, white_level], [y_axis(yoldc), y_axis(yoldc)], $
            color=self.colors.erase
    oplot, (*self.image_data.raw_data)(xc, y_min:y_max), y_axis(y_min:y_max), $
            psym=10, color=self.colors.plot
    plots, [black_level, white_level], [y_axis(yc), y_axis(yc)], $
            color=self.colors.line
    self.image_window.x_prev = xc
    self.image_window.y_prev = yc
end


pro image_display::display_image, noerase=noerase, leave_mouse=leave_mouse

    if n_elements(noerase) eq 0 then noerase=0

    if (self.image_window.order EQ 0) then begin
        self->dc_to_ic, 0, 0, first_col, first_row, /clip
        self->dc_to_ic, self.image_window.x_size-1, $
            self.image_window.y_size-1, $
            last_col, last_row, /clip
    endif else begin
        self->dc_to_ic, 0, self.image_window.y_size-1, $
            first_col, first_row, /clip
        self->dc_to_ic, self.image_window.x_size-1, 0, $
            last_col, last_row, /clip
    endelse
    if (self.image_window.order EQ 0) then begin
        self->ic_to_dc, first_col, first_row, x_offset, y_offset
        x_offset = (x_offset - self.image_window.x_zoom/2.) > 0
        y_offset = (y_offset - self.image_window.y_zoom/2.) > 0
    endif else begin
        self->ic_to_dc, first_col, last_row, x_offset, y_offset
        x_offset = (x_offset - self.image_window.x_zoom/2.) > 0
        y_offset = (y_offset - self.image_window.y_zoom/2.) > 0
    endelse

    nc = last_col - first_col + 1
    nr = last_row - first_row + 1
    if self.image_window.zoom_mode eq 0 then sample=1 else sample=0
    wset, self.image_window.winid
    if noerase eq 0 then erase
    if (self.image_window.x_zoom eq 1) and $
       (self.image_window.y_zoom eq 1) then begin
        tv, (*self.image_data.display_buff)(first_col:last_col, $
                first_row:last_row), x_offset, y_offset, order=self.image_window.order
    endif else begin
        buff = rebin((*self.image_data.display_buff)(first_col:last_col, $
                        first_row:last_row), nc*self.image_window.x_zoom, $
                        nr*self.image_window.y_zoom, sample=sample)
        tv, buff, x_offset, y_offset, order=self.image_window.order
    endelse
    self->init_plot_windows, leave_mouse=leave_mouse
end


pro image_display::set_image_data, image
    isize = size(image)
    if (isize[0] ne 2) then image = reform(image)
    self.image_data.x_size = n_elements(image(*,0))
    self.image_data.y_size = n_elements(image(0,*))
    ptr_free, self.image_data.raw_data
    self.image_data.raw_data = ptr_new(image)

    xdist = findgen(self.image_data.x_size)
    ydist = findgen(self.image_data.y_size)

    ptr_free, self.image_data.x_dist
    ptr_free, self.image_data.y_dist
    self.image_data.x_dist = ptr_new(xdist)
    self.image_data.y_dist = ptr_new(ydist)


return 
end

pro image_display::scale_image, image, min=min, max=max, zoom=zoom, $
            center=center, noerase=noerase, interpolate=interpolate, $
            replicate=replicate, xdist=xdist, ydist=ydist, $
            title=title, subtitle=subtitle, leave_mouse=leave_mouse, $
            order=order, retain=retain
;+
; NAME:
;       IMAGE_DISPLAY::SCALE_IMAGE
;
; PURPOSE:
;       This routine displays a new image in an existing IMAGE_DISPLAY object.
;
; CATEGORY:
;       Imaging
;
; CALLING SEQUENCE:
;       image_data->SCALE_IMAGE, Data
;
; INPUTS:
;       Data:   A 2-D array to be displayed
;
; KEYWORD PARAMETERS:
;       XDIST:  An array containing the user units ("distance") of the
;               X axis pixels.  Dimensions must be same as xsize of Data.
;
;       YDIST:  An array containing the user units ("distance") of the
;               Y axis pixels.  Dimensions must be same as ysize of Data.
;
;       MIN:    The minimum display intensity.  Default=min(Data).
;
;       MAX:    The maximum display intensity.  Default=max(Data).
;
;       ZOOM:   A scaler or 2-element (X,Y) array of integer zoom factors.
;               Default = 1 in each direction.  ZOOM=2 will zoom 2X in both
;               directions, ZOOM=[1,2] will zoom 1X in X, 2X in Y.
;
;       CENTER: The location where the center of the image should be located
;               in the display window.
;               The default is the center of the display window.
;               CENTER=[200,300] will center the image at X=200, Y=300
;
;       NOERASE: Set this flag to not erase the window before displaying the
;               image.  Allows multiple images to share a window.
;
;       INTERPOLATE:  Zoom the image by interpolation rather than replication.
;
;       REPLICATE: Zoom the image by replication rather than interpolation.
;
;       TITLE:  The title to give the display window.
;
;       SUBTITLE:  The subtitle to give the display window.
;
;       ORDER:  The order in which to display the image.
;               0=bottom to top
;               1=top to bottom
;               Default = Existing order
;
;       LEAVE_MOUSE:  Set this keyword to not move the mouse to the center of the
;               new image display.
;
;       RETAIN: Set this keyword to not reset the zoom and intensity scaling when
;               when the new data are displayed.  This requires that the dimensions
;               of the image be the same as the image currently displayed.
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers (3-DEC-1998)
;       20-Nov-2007  MLR  Added ORDER and RETAIN keywords.
;
; EXAMPLE:
;       IDL> a = DIST(512)
;       IDL> obj = OBJ_NEW('IMAGE_DISPLAY', a)
;       IDL> obj->SCALE_IMAGE, a+100, /RETAIN
;
;-


    size = size(image)
    if (n_elements(order) ne 0) then begin
        if (order ne self.image_window.order) then begin
            self.image_window.order = order
            widget_control, self.widgets.order, set_droplist_select=self.image_window.order
        endif
    endif

    if (size[0] ne 2) then image = reform(image)
    self.image_data.x_size = n_elements(image(*,0))
    self.image_data.y_size = n_elements(image(0,*))
    ptr_free, self.image_data.raw_data
    self.image_data.raw_data = ptr_new(image)

    if (keyword_set(retain)) then begin
        xdist = *self.image_data.x_dist
        ydist = *self.image_data.y_dist
    endif
    if (n_elements(xdist) eq 0) then xdist = findgen(self.image_data.x_size)
    if (n_elements(ydist) eq 0) then ydist = findgen(self.image_data.y_size)
    if (n_elements(title) ne 0) then begin
       if (n_elements(subtitle) ne 0) then title = title+':'+subtitle
       widget_control, self.widgets.base, tlb_set_title=title
    endif
    if (n_elements(xdist) ne self.image_data.x_size) or $
       (n_elements(ydist) ne self.image_data.y_size) then $
        t = dialog_message('Size of xdist and ydist must match size of image')
    ptr_free, self.image_data.x_dist
    ptr_free, self.image_data.y_dist
    self.image_data.x_dist = ptr_new(xdist)
    self.image_data.y_dist = ptr_new(ydist)

    case n_elements(center) of
        0: begin
            if (not keyword_set(retain)) then begin
                self.image_window.x_cent_i = self.image_data.x_size/2
                self.image_window.y_cent_i = self.image_data.y_size/2
            endif
        end
        1: begin
            self.image_window.x_cent_i = center
            self.image_window.y_cent_i = center
        end
        2: begin
            self.image_window.x_cent_i = center(0)
            self.image_window.y_cent_i = center(1)
       end
    endcase

    case n_elements(zoom) of
        0: begin
            if (keyword_set(retain)) then begin
                x_zoom = self.image_window.x_zoom
                y_zoom = self.image_window.y_zoom
            endif else begin
                x_zoom = self.image_window.x_size / self.image_data.x_size
                y_zoom = self.image_window.y_size / self.image_data.y_size
                if (x_zoom gt y_zoom) then x_zoom = y_zoom
                if (y_zoom gt x_zoom) then y_zoom = x_zoom
            endelse
        end
        1: begin
            x_zoom = zoom
            y_zoom = zoom
        end
        2: begin
            x_zoom = zoom(0)
            y_zoom = zoom(1)
        end
    endcase
    self.image_window.x_zoom = x_zoom > 1
    self.image_window.y_zoom = y_zoom > 1

    if (n_elements(min) ne 0) then begin
        self.image_window.black_level = min
    endif else begin
        if (not keyword_set(retain)) then begin
            self.image_window.black_level = min(image)
        endif
    endelse

    if (n_elements(max) ne 0) then begin
        self.image_window.white_level = max
    endif else begin
        if (not keyword_set(retain)) then begin
            self.image_window.white_level = max(image)
        endif
    endelse

    if n_elements(noerase) eq 0 then noerase = 0
    if n_elements(interpolate) ne 0 then self.image_window.zoom_mode=1
    if n_elements(replicate) ne 0 then self.image_window.zoom_mode=0

    ptr_free, self.image_data.display_buff
    self.image_data.display_buff = ptr_new(bytscl(*self.image_data.raw_data, $
                                       min=self.image_window.black_level, $
                                       max=self.image_window.white_level, $
                                       top=!D.TABLE_SIZE-1))

    self.MinMaxtable[0].actual=min(image)
    self.MinMaxtable[1].actual=max(image)
    self.MinMaxtable[0].display=self.image_window.black_level
    self.MinMaxtable[1].display=self.image_window.white_level
    widget_control, self.widgets.MinMaxtable, set_value=self.MinMaxtable
    self->display_image, noerase=noerase, leave_mouse=leave_mouse
end


pro image_display::dc_to_ic, xd, yd, xi, yi, clip=clip

    if (self.image_window.order EQ 1) then y_dir = -1 else y_dir = 1
    x_dir = 1
    xi = (xd - self.image_window.x_size/2) / $
                (x_dir * self.image_window.x_zoom) + $
    self.image_window.x_cent_i
    yi = (yd - self.image_window.y_size/2) / $
                (y_dir * self.image_window.y_zoom) + $
    self.image_window.y_cent_i
    if (keyword_set(clip)) then begin
        xi = (xi > 0) < (self.image_data.x_size-1)
        yi = (yi > 0) < (self.image_data.y_size-1)
    endif
end



pro image_display::ic_to_dc, xi, yi, xd, yd, clip=clip

    if (self.image_window.order EQ 1) then y_dir = -1 else y_dir = 1
    x_dir = 1
    xd = self.image_window.x_size/2 + $
            x_dir*(xi - self.image_window.x_cent_i) * self.image_window.x_zoom
    yd = self.image_window.y_size/2 + $
            y_dir*(yi - self.image_window.y_cent_i) * self.image_window.y_zoom
    if (keyword_set(clip)) then begin
        xd = (xd > 0) < (self.image_window.x_size-1)
        yd = (yd > 0) < (self.image_window.y_size-1)
    endif
end



pro image_display_event, event

    widget_control, event.top, get_uvalue=image_display
    image_display->event, event
end

pro image_display::event, event
    if (tag_names(event, /structure_name) eq 'WIDGET_KILL_REQUEST') then begin
        widget_control, event.top, /destroy
        obj_destroy, self
        return
    endif
    case event.id of
        self.widgets.image: begin
            case event.type of
                0: begin    ; Button press event
                    case event.press of
                        1: begin  ; Left mouse button
                            self->dc_to_ic, event.x, event.y, xi, yi, /clip
                            self.image_window.x_cent_i = xi
                            self.image_window.y_cent_i = yi
                            if (self.image_window.x_zoom gt 1) and $
                                (self.image_window.y_zoom gt 1) then begin
                                self.image_window.x_zoom = $
                                    self.image_window.x_zoom/2
                                self.image_window.y_zoom = $
                                    self.image_window.y_zoom/2
                            endif else begin
                                self.image_window.x_cent_i = $
                                    self.image_data.x_size/2
                             self.image_window.y_cent_i = $
                                    self.image_data.y_size/2
                            endelse
                        end
                        2: begin
                            ; Middle mouse button does nothing now
                        end
                        4: begin
                            self->dc_to_ic, event.x, event.y, xi, yi, /clip
                            self.image_window.x_cent_i = xi
                            self.image_window.y_cent_i = yi
                            self.image_window.x_zoom = $
                                self.image_window.x_zoom*2
                            self.image_window.y_zoom = $
                                self.image_window.y_zoom*2
                        end
                        else: begin
                            print, 'Unexpected mouse event = ', event.press
                        end
                    endcase
                    self->display_image
                end
                2: begin   ; Motion event
                    x_mouse=event.x
                    y_mouse=event.y
                    self->plot_profiles, x_mouse, y_mouse
                end
                else:
            endcase
        end

        self.widgets.MinMaxtable: begin
            widget_control, self.widgets.MinMaxtable, get_value=value
            if (event.Y eq 1) then begin
                self.image_window.black_level=value[0].display
                self.image_window.white_level=value[1].display
                ptr_free, self.image_data.display_buff
                self.image_data.display_buff = $
                        ptr_new(bytscl(*self.image_data.raw_data, $
                                min=self.image_window.black_level, $
                                max=self.image_window.white_level, $
                                top=!D.TABLE_SIZE-1))
            endif
            self->display_image
        end

        self.widgets.order: begin
            self.image_window.order = widget_info(event.id, $
                                                      /droplist_select)
            self->display_image
        end

        self.widgets.zoom_mode: begin
            self.image_window.zoom_mode = widget_info(event.id, $
                                                      /droplist_select)
            self->display_image
        end

        self.widgets.autoscale: begin
            self.image_window.black_level=min(*self.image_data.raw_data)
            self.image_window.white_level=max(*self.image_data.raw_data)
            self.MinMaxtable[0].display=self.image_window.black_level
            self.MinMaxtable[1].display=self.image_window.white_level
            widget_control, self.widgets.MinMaxtable, set_value=self.MinMaxtable
            ptr_free, self.image_data.display_buff
            self.image_data.display_buff = $
                        ptr_new(bytscl(*self.image_data.raw_data, $
                                min=self.image_window.black_level, $
                                max=self.image_window.white_level, $
                                top=!D.TABLE_SIZE-1))
            self->display_image
        end

        self.widgets.new_color_table: begin
            xloadct
        end

        self.widgets.apply_color_table: begin
            self->display_image
        end

        self.widgets.out_form: begin
            self.image_data.out_form = event.index
        end
        self.widgets.out_file: begin
            self->write_output_file, $
                    format=self.image_data.oformats[self.image_data.out_form]
        end


        else:  t = dialog_message('Unknown event')
    endcase
end


function image_display::init, data, xsize=xsize, ysize=ysize, $
                                    xdist=xdist, ydist=ydist, min=min, max=max, $
                                    title=title, subtitle=subtitle, order=order
;+
; NAME:
;       IMAGE_DISPLAY::INIT
;
; PURPOSE:
;       This function initializes an object of class IMAGE_DISPLAY.  It is
;       not called directly, but is called indirectly when a new object of
;       class IMAGE_DISPLAY is created via OBJ_NEW('IMAGE_DISPLAY')
;
;       The IMAGE_DISPLAY object is a GUI display which provides interactive
;       pan, zoom and scroll, live update of row and column profiles, etc.
;
; CATEGORY:
;       Imaging
;
; CALLING SEQUENCE:
;       obj = OBJ_NEW('IMAGE_DISPLAY', Data)
;
; INPUTS:
;       Data:   A 2-D array to be displayed
;
; KEYWORD PARAMETERS:
;       XSIZE:  The number of pixels horizontally in the image window.
;               Default is the greater of 400 pixels or the xsize of Data.
;
;       YSIZE:  The number of pixels vertically in the image window.
;               Default is the greater of 400 pixels or the ysize of Data.
;
;       XDIST:  An array containing the user units ("distance") of the
;               X axis pixels.  Dimensions must be same as xsize of Data.
;
;       YDIST:  An array containing the user units ("distance") of the
;               Y axis pixels.  Dimensions must be same as ysize of Data.
;
;       MIN:    The minimum display intensity.  Default=min(Data).
;
;       MAX:    The maximum display intensity.  Default=max(Data).
;
;       TITLE:  The title to give the display window.
;
;       SUBTITLE:  The subtitle to give the display window.
;
;       ORDER:  The order in which to display the image.
;               0=bottom to top
;               1=top to bottom
;               Default = !ORDER system variable when this function is called.
;
; OUTPUTS:
;       This function returns 1 to indicate that the object was successfully
;       created.
;
; EXAMPLE:
;       IDL> a = DIST(512)
;       IDL> obj = OBJ_NEW('IMAGE_DISPLAY', a)
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers (3-DEC-1998)
;       3-APR-1999  MLR  Fixed so that it will reform input array if it is
;                        greater than 2-D but with some dimensions=1,
;                        e.g. Data = intarr(100,1,100), which happens
;                        frequently when extracting slices from 3-D data
;       23-SEP-1999 MN   Added code to write JPEG file
;       12-APR-2001 MLR  Added cleanup code so that the image_display object and all
;                        pointers are free when the user closes the window.
;       11-JUN-2001 MN   Added dropdown menu for output types (JPEG, BMP, PNG), and
;                        added 'title' and 'subtitle' keywords which are
;                        displayed in the window title
;       17-JAN-2001 MLR  Delete line that called "a2f".  Line did nothing and a2f
;                        is not needed.
;       27-APR-2002 MLR  Added order keywords to write_jpeg and write_png so images
;                        are correct side up.
;                        Fixed bug in plotting vertical column profile if self.image_window.order=1, thanks
;                        to Peter Vontobel of Swiss Light Source.
;
;       4-JUN-2002  MLR  Added "title" keyword to image_display::display_image
;                        and "leave_mouse" keyword to other routines.  These
;                        changes allow tomo_display:: to scroll though 2-D
;                        slices in a 3-D volume quickly and easily.
;
;       27-APR-2006  MLR Added "tiff" output option
;
;       21-NOV-2007  MLR Added order keyword and order widget, no longer use !order except at startup
;                        Added RETAIN keyword to SCALE_IMAGE, to preserve zoom, center and intensity
;                        scaling.
;                        Added autoscale widget for automatically scaling display range.
;-

    size = size(data)
    if (size[0] ne 2) then data = reform(data)
    size = size(data)
    if (size[0] ne 2) then t = dialog_message('Data must be 2-D')
    if (n_elements(xsize) eq 0) then xsize=size[1] > 400
    if (n_elements(ysize) eq 0) then ysize=size[2] > 400
    self.image_window.x_size = xsize
    self.image_window.y_size = ysize
    if (n_elements(order) eq 0) then order=!order
    self.image_window.order = order

    self.row_plot_window.y_size=200
    self.col_plot_window.x_size=200


    maintitle= 'IDL Image Display'
    stmp     = 'image'
    if (keyword_set(title))    then begin
        maintitle=title
        stmp     =title
    endif
    if (keyword_set(subtitle)) then  begin
        maintitle= maintitle+ ': '+subtitle
        stmp     = stmp     + '_' +subtitle
    endif
    self.image_data.handle=  strcompress(stmp, /remove_all)

    top = !d.table_size-1
    ; Don't use decomposed color on 24 bit display, since we want to use
    ; lookup tables.
    if (!d.n_colors gt 256) then device, decomposed=0
    self.colors.erase = 0
    self.colors.plot = top
    self.colors.line = top/2

    self.widgets.base= widget_base(column=1, /tlb_kill_request_events,title=maintitle)

    row = widget_base(self.widgets.base, row=1, /align_center)
    self.widgets.XYtable = widget_table(row, xsize=3, ysize=3, $
                                    value=self.XYtable, scroll=0, $
                                    /column_major, $
                                    column_label=['X', 'Y', 'Data'], $
                                    row_label=['Pixel', 'User', 'Screen'])
    self.widgets.MinMaxtable = widget_table(row, xsize=2, ysize=2, $
                                    value=self.MinMaxtable, scroll=0, $
                                    /column_major, $
                                    column_label=['Min', 'Max'], $
                                    row_label=['Actual', 'Display'], /edit)
    col = widget_base(row, column=1, /align_center)
    self.widgets.autoscale = widget_button(col, value='Autoscale')
    row = widget_base(self.widgets.base, row=1)
    base = widget_base(row, xsize=self.col_plot_window.x_size, $
                        ysize=self.row_plot_window.y_size, column=1)
    col = widget_base(base, column=1, /align_center)
    row1 = widget_base(col, row=1, /frame, /align_center)
    col1 = widget_base(row1, column=1, /align_center)
    t = widget_label(col1, value='Display order')
    self.widgets.order = widget_droplist(col1, $
                                value=['Bottom to top', 'Top to bottom'])
    col1 = widget_base(row1, col=1, /align_center)
    t = widget_label(col1, value='Zoom Mode')
    self.widgets.zoom_mode = widget_droplist(col1, $
                                value=['Replicate', 'Interpolate'])
    row1 = widget_base(col, row=1, /frame, /align_center)
    self.widgets.new_color_table = widget_button(row1, value='New color table')
    self.widgets.apply_color_table = widget_button(row1, $
                                value='Apply color table')
    self.image_data.oformats = ['JPEG', 'BMP', 'PNG', 'TIF']
    self.image_data.out_form = 0

    col1 = widget_base(base, column=1, /align_center,/frame)
    row1 = widget_base(col1, row=1)
    t  = widget_label(row1, value='Output Format')
    self.widgets.out_form = widget_droplist(row1, value=self.image_data.oformats)

    widget_control, self.widgets.out_form,  set_droplist_select=self.image_data.out_form
    self.widgets.out_file = widget_button(col1, value='Write Image File')
    self.widgets.row_plot = widget_draw(row, xsize=xsize, $
                                         ysize=self.row_plot_window.y_size)
    row = widget_base(self.widgets.base, row=1)
    self.widgets.col_plot = widget_draw(row, ysize=ysize, $
                                        xsize=self.col_plot_window.x_size)
    self.widgets.image = widget_draw(row, xsize=xsize, ysize=ysize, $
                                     /motion_events, /button_events)

    widget_control, self.widgets.base, /realize

    widget_control, self.widgets.row_plot, get_value=window
    self.row_plot_window.winid = window
    widget_control, self.widgets.col_plot, get_value=window
    self.col_plot_window.winid = window
    widget_control, self.widgets.image, get_value=window
    self.image_window.winid = window

    self->scale_image, data, xdist=xdist, ydist=ydist, min=min, max=max
    widget_control, self.widgets.base, set_uvalue=self

    xmanager, 'image_display', self.widgets.base, /no_block
    return, 1
end

pro image_display::cleanup
    ptr_free, self.image_data.raw_data
    ptr_free, self.image_data.display_buff
    ptr_free, self.image_data.x_dist
    ptr_free, self.image_data.y_dist
end

pro image_display__define, data, xsize=xsize, ysize=ysize

    widgets={ image_display_widgets, $
        base: 0L, $
        image: 0L, $
        row_plot: 0L, $
        col_plot: 0L, $
        XYtable: 0L, $
        MinMaxtable: 0L, $
        order:     0L, $
        autoscale: 0L, $
        zoom_mode: 0L, $
        new_color_table: 0L, $
        apply_color_table: 0L, $
        out_form: 0L, $
        out_file: 0L, $
        exit: 0L $
    }

    colors = {image_display_colors, $
        erase: 0L, $
        plot:  0L, $
        line:  0L $
    }

    image_window = {image_display_image_window, $
        winid:  0L, $
        x_size: 0L, $
        y_size: 0L, $
        black_level: 0.0, $
        white_level: 0.0, $
        colors: colors, $
        x_zoom: 0L, $
        y_zoom: 0L, $
        zoom_mode: 0L, $
        order:    0L, $
        x_cent_i: 0L, $
        y_cent_i: 0L, $
        x_min: 0L, $
        x_max: 0L, $
        y_min: 0L, $
        y_max: 0L, $
        x_prev: 0L,$
        y_prev: 0L $
    }

    col_plot_window = {image_display_plot, $
        winid:  0L, $
        x_size: 0L, $
        y_size: 0L, $
        x:      !X, $
        y:      !Y, $
        p:      !P $
    }

    row_plot_window = {image_display_plot}

    image_data = {image_display_image_data, $
        raw_data: ptr_new(), $
        display_buff: ptr_new(), $
        oformats: strarr(4), $
        handle: '', $
        out_form: 0L, $
        x_size: 0L, $
        y_size: 0L, $
        x_dist: ptr_new(), $
        y_dist: ptr_new() $
    }

    XYtable = {image_display_XYtable, pixel: 0L, user: 0.0, screen: 0L}
    XYtable = replicate(XYtable, 3)
    MinMaxtable = {image_display_MinMaxtable, actual: 0.0, display: 0.0}
    MinMaxtable = replicate(MinMaxtable, 2)


    image_display = {image_display, $
        widgets: widgets, $
        image_window: image_window, $
        row_plot_window: row_plot_window, $
        col_plot_window: col_plot_window, $
        image_data: image_data, $
        colors: colors, $
        XYtable:XYtable, $
        MinMaxtable:MinMaxtable $
    }
end


