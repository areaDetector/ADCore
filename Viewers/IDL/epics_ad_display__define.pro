pro epics_ad_display::connect_detector
    if (obj_valid(self.detector)) then obj_destroy, self.detector
    self.detector = obj_new('epics_nd_std_arrays', self.base_pv)
    if (obj_valid(self.detector)) then begin
        self.connected = 1
        widget_control, self.widgets.message, set_value=systime(0) + ': Connected to detector: ' + self.base_pv
        widget_control, self.widgets.timer, timer=self.display_interval
    endif else begin
        self.connected = 0;
        widget_control, self.widgets.message, set_value=systime(0) + ': Unable to connect to detector: ' + self.base_pv
    endelse
end

pro epics_ad_display::get_data, size_changed
    data = self.detector->getArray()
    ; Get the new image sizes
    size_changed = 0
    colorMode = self.detector->getProperty('ColorMode_RBV')
    dims = size(data, /dimensions)
    ; If any of the dimensions is 1 then reform the array
    t = where(dims eq 1, count)
    if (count ne 0) then data = reform(data)
    dims = size(data, /dimensions)
    nx = dims[0]
    ny = dims[1]
    ndims = n_elements(dims)
    if (n_elements(dims) gt 2) then nz = dims[2] else nz=1
    ; These are correct for all except RGB1 and RGB2
    self.hSize = nx
    self.vSize = ny
    ptr_free, self.pData
    self.pData = ptr_new(data, /no_copy)
    self.true_color = 0
    ; The colorMode PV can be out of sync with data, be careful
    if (ndims eq 3) then begin
        case colorMode of
            'RGB1': begin
                self.hSize = ny
                self.vSize = nz
                self.true_color = 1
            end
            'RGB2': begin
                self.hSize = nx
                self.vSize = nz
                self.true_color = 2
            end
            'RGB3': begin
                self.true_color = 3
            end
        endcase
    endif
    if (nx ne self.nx) then begin
        self.nx = nx
        size_changed = 1
        widget_control, self.widgets.nx, set_value=nx
    endif
    if (ny ne self.ny) then begin
        self.ny = ny
        size_changed = 1
        widget_control, self.widgets.ny, set_value=ny
    endif
    if (nz ne self.nz) then begin
        self.nz = nz
        size_changed = 1
        widget_control, self.widgets.nz, set_value=nz
    endif
    if (self.nx eq 0) or (self.ny eq 0) then return
end

pro epics_ad_display_event, event
    widget_control, event.top, get_uvalue=epics_ad_display
    epics_ad_display->event, event
end


pro epics_ad_display::event, event
    if (tag_names(event, /structure_name) eq 'WIDGET_KILL_REQUEST') then begin
        widget_control, event.top, /destroy
        obj_destroy, self
        return
    endif

    case event.id of

        self.widgets.exit: begin
            widget_control, event.top, /destroy
            obj_destroy, self
            return
        end

        self.widgets.base_pv: begin
            widget_control, self.widgets.base_pv, get_value=t
            if (strlen(t) gt 0) then begin
                self.base_pv = t
                self->connect_detector
            endif
        end

        self.widgets.display_mode: begin
            self.display_mode = event.index
            self->display_image, *self.pData, true=self.true_color, retain=1
        end

        self.widgets.display_enable: begin
            self.display_enable = event.index
        end

        self.widgets.autoscale: begin
            self.autoscale = event.index
            self->display_image, *self.pData, true=self.true_color, retain=1
        end

        self.widgets.flip_y: begin
            self.flip_y = event.index
            self->display_image, *self.pData, true=self.true_color, retain=1
        end

        self.widgets.display_min: begin
            self.display_min = event.value
            self->display_image, *self.pData, true=self.true_color, retain=1
        end

        self.widgets.display_max: begin
            self.display_max = event.value
            self->display_image, *self.pData, true=self.true_color, retain=1
        end

        self.widgets.timer: begin
            catch, err
            if (err ne 0) then begin
                ; We got an error.  This is almost certainly because the detector
                ; is not available.
                print, !error_state.msg
                ; Re-enable timer with connect interval so it tries to reconnect, but not too
                ; frequently
                widget_control, self.widgets.timer, timer=self.connect_interval
                return
            endif
            ; Timer event to check for new data and display it.
            ; If we are not connected try to connect
            if (not self.connected) then self->connect_detector
            ; If that did not work, try again next time
            if (not self.connected) then begin
                widget_control, self.widgets.timer, timer=self.connect_interval
                return
            endif
            ; If we get to here then we are connected
            ; Reset the timer for the display interval
            widget_control, self.widgets.timer, timer=self.display_interval
            if (not self.display_enable) then break
            ; for the widget timer event between frame
            for i=0, self.frames_per_loop do begin
                new_data = self.detector->newArray()
                if (new_data eq 0) then break
                ; There is new data, get it
                self->get_data, size_changed
                retain = size_changed eq 0
                self->display_image, *self.pData, true=self.true_color, retain=retain
                self.image_counter = self.image_counter + 1
            endfor
            time = systime(1)
            dt = time - self.last_update
            if (dt gt 2.0) then begin
                rate = self.image_counter / dt
                widget_control, self.widgets.frame_rate, set_value=string(rate, format='(f8.3)')
                if (self.image_counter gt 0) then begin
                    widget_control, self.widgets.message, set_value='Got new image at ' + systime(0)
                endif
                self.last_update = time
                self.image_counter = 0
            endif
            ; Need to set the timer again because the time might have elapsed in display?
            widget_control, self.widgets.timer, timer=self.display_interval

        end

        else:  t = dialog_message('Unknown event')
    endcase
    return
end

pro epics_ad_display::display_image, dataIn, true=true, retain=retain
    ; If we are using iTools or tv then make data byte type
    if (self.display_mode ne 1) then begin
        if (self.autoscale) then begin
            black=min(dataIn, max=white)
            self.display_min = black
            self.display_max = white
            widget_control, self.widgets.display_min, set_value=float(black)
            widget_control, self.widgets.display_max, set_value=float(white)
        endif else begin
            black=self.display_min
            white=self.display_max
        endelse
        ; We can optimize by not doing any scaling if this is already a byte image
        ; and the display range is 0 to 255
        if (size(dataIn, /type) ne 1) or (self.display_min ne 0) or (self.display_max ne 255) then begin
            data = bytscl(dataIn, min=black, max=white)
        endif
    endif
    ; Make a copy of data if not done just above
    if (n_elements(data) eq 0) then data = dataIn
    ; Flip the data vertically if desired.  Use rotate for mono data (it's faster) and
    ; order for color because rotate does not work
    order = 0
    if (self.flip_y) then begin
        ndims = size(data, /n_dimensions)
        if (ndims eq 2) then data = rotate(data, 7) else order=1
    endif
    case self.display_mode of
    0: begin
           catch, err
           if ((err ne 0) or (retain eq 0)) then begin
               if (err ne 0) then print, !error_state.msg
               device, window_state=state
               if (state[self.tv_window] ne 0) then begin
                   device, get_window_position=position
                   if (!version.os_family eq "Windows") then position[1] = position[1] - !d.y_size
               endif else begin
                   position=[500,0]
               endelse
               window, self.tv_window, xpos=position[0], ypos=position[1], $
                       xsize=self.hSize, ysize=self.vSize, title=self.base_pv
               wshow, self.tv_window
           endif
           wset, self.tv_window
           tv, data, true=true, order=order
           catch, /cancel
       end
    1: begin
           ; image_display cannot display color yet
           if (true ne 0) then begin
                t = dialog_message('Cannot use color with image_display')
                self.display_mode = 0
                return
           endif
           if ((not obj_valid(self.image_display)) or (retain eq 0)) then begin
               self.image_display = obj_new('image_display', data, order=1)
           endif else begin
               self.image_display->scale_image, data, /leave_mouse, /noerase, retain=retain, $
                                                min=black, max=white
           endelse
       end
    2: begin
           if ((not obj_valid(self.iimage_obj)) or (retain eq 0)) then begin
               iimage, data
               idTool = itgetcurrent(TOOL=oTool)
               idVisImage = oTool->FindIdentifiers('*/IMAGE', /VISUALIZATIONS)
               oImage = oTool->GetByIdentifier(idVisImage[0])
               oParams = oImage->GetParameterSet()
               ; I guessed the name of this parameter from the iTools Visualization Browser
               oImageParam = oParams->Get(NAME='IMAGE PLANES')
               self.iimage_obj = oImageParam
           endif else begin
               t = self.iimage_obj->SetData(data)
           endelse
       end
    endcase
end


function epics_ad_display::init, base_pv

;+
; NAME:
;       EPICS_AD_DISPLAY::INIT
;
; PURPOSE:
;       The EPICS_IMAGE_DISPLAY class provides a general purpose routine to display
;       EPICS arrays as images. It uses either TV for simple but fast display,
;       or <A HREF=#IMAGE_DISPLAY>IMAGE_DISPLAY</A> to perform interactive pan,
;       zoom and scroll, live update of row and column profiles, etc.
;
;       This function initializes an object of class EPICS_AD_DISPLAY.  It is
;       not called directly, but is called indirectly when a new object of
;       class EPICS_IMAGE_DISPLAY is created via OBJ_NEW('EPICS_IMAGE_DISPLAY')
;
;       The EPICS_AD_DISPLAY object creates simple GUI display which provides
;       widgets to control the EPICS PV to display, the number of rows and columns in
;       the images, inverting in the Y direction, enable/disable, and a status display.
;
;       EPICS_AD_DISPLAY waits new data for the EPICS array, and then displays the
;       new image.
;
; CATEGORY:
;       Imaging
;
; CALLING SEQUENCE:
;       obj = OBJ_NEW('EPICS_AD_DISPLAY', ImagePV)
;
; INPUTS:
;       ImagePV:  The base name of an areaDetector NDStdArrays object that contains the image data.  This name is the
;                 $(P)$(R) values passed to NDStdArrays.template when the EPICS database is loaded.  This database
;                 contains not only the image data itself, but also PVs that contain the image dimensions, etc.
;
;       In order to display images that contain more than 16000 bytes (which most images do!)
;       it is necessary that both the EPICS server (e.g. IOC) and the EPICS client
;       (i.e. ezcaIDL.so or ezcaIDL.dll) be built with EPICS 3.14 or later.
;       Furthermore, one must set the environment variable EPICS_CA_MAX_ARRAY_BYTES to be
;       at least the size of the image array data.  This must be done on both the client and
;       server machines.
;       For example, with the Pilatus detector we typically set EPICS_CA_MAX_ARRAY_BYTES=500000.
;
; OUTPUTS:
;       This function returns 1 to indicate that the object was successfully
;       created.
;
; REQUIREMENTS:
;       This function requires <A HREF=#IMAGE_DISPLAY>IMAGE_DISPLAY__DEFINE.PRO</A>,
;       GET_FONT_NAME.PRO, and <A HREF=ezcaIDLGuide.html>ezcaIDL</A>.
;       EZCA_IDL requires the shareable library (ezcaIDL.so or ezcaIDL.dll) that
;       contains the functions used to communicate with EPICS.
;
; EXAMPLE:
;       IDL> obj = OBJ_NEW('EPICS_IMAGE_DISPLAY', 'GSE-PILATUS1:ImageData', nx=487, ny=195)
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers (1-March-2008).
;-
    if (n_elements(base_pv) eq 0) then base_pv=''
    self.base_pv = base_pv
    self.connected = 0
    self.display_min = 0
    self.display_max = 255
    self.display_interval = 0.01
    self.connect_interval = 2.0
    self.display_enable = 1
    self.flip_y = 0
    self.display_mode = 0
    ; Maximum number of frames to display per widget timer loop.
    ; Too large and IDL will become unresponsive to widget events
    self.frames_per_loop = 10
    self.autoscale = 1

    self.fonts.normal = get_font_name(/helvetica)
    self.fonts.heading1 = get_font_name(/large, /bold)
    self.fonts.heading2 = get_font_name(/bold)

    cainit  ; Need to call this in case it was not called in startup script, i.e. Virtual Machine
    casettimeout, .01
    casetretrycount, 100
    self.widgets.base= widget_base(column=1, /tlb_kill_request_events, $
                                   title='EPICS Image Display', $
                                   mbar=mbar, tab_mode=1)

    ; File menu
    file = widget_button(mbar, /menu, value = 'File')
    self.widgets.exit = widget_button(file, $
                                            value = 'Exit')
    row = widget_base(self.widgets.base, /row)
    self.widgets.base_pv = cw_field(row, /column, title='Base PV', xsize=30, /return_events)
    widget_control, self.widgets.base_pv, set_value=self.base_pv

    self.widgets.nx = cw_field(row, /column, /integer, title='NX', xsize=8, /noedit)
    self.widgets.ny = cw_field(row, /column, /integer, title='NY', xsize=8, /noedit)
    self.widgets.nz = cw_field(row, /column, /integer, title='NZ', xsize=8, /noedit)

    col = widget_base(row, /column)
    t = widget_label(col, value='Flip Y')
    self.widgets.flip_y = widget_droplist(col, value=['No', 'Yes'])
    widget_control, self.widgets.flip_y, set_droplist_select = self.flip_y

    col = widget_base(row, /column)
    t = widget_label(col, value='Display mode')
    self.widgets.display_mode = widget_droplist(col, value=['TVSCL', 'image_display', 'iimage'])
    widget_control, self.widgets.display_mode, set_droplist_select=self.display_mode

    col = widget_base(row, /column)
    t = widget_label(col, value='Autoscale')
    self.widgets.autoscale = widget_droplist(col, value=['No', 'Yes'])
    widget_control, self.widgets.autoscale, set_droplist_select = self.autoscale

    self.widgets.display_min = cw_field(row, /column, /float, title='Min.', xsize=8, value=self.display_min, /return_events)
    self.widgets.display_max = cw_field(row, /column, /float, title='Max.', xsize=8, value=self.display_max, /return_events)

    col = widget_base(row, /column)
    t = widget_label(col, value='Display enable')
    self.widgets.display_enable = widget_droplist(col, value=['No', 'Yes'])
    widget_control, self.widgets.display_enable, set_droplist_select=self.display_enable

    self.widgets.frame_rate = cw_field(row, /column, title='Frames/sec', xsize=8, /noedit)

    row = widget_base(self.widgets.base, /row)
    self.widgets.message = cw_field(row, /row, title='Message', xsize=100)

    widget_control, self.widgets.base, set_uvalue=self
    widget_control, self.widgets.base, /realize

    ; Timer widgets
    self.widgets.timer = self.widgets.base
    widget_control, self.widgets.timer, timer=self.display_interval

    self.tv_window = 10

    xmanager, 'epics_ad_display', self.widgets.base, /no_block
    if (strlen(self.base_pv) gt 0) then self->connect_detector
    return, 1
end

pro epics_ad_display::cleanup

end

pro epics_ad_display__define

    widgets={ epics_ad_display_widgets, $
        base:          0L, $
        filename:      0L, $
        base_pv:       0L, $
        nx:            0L, $
        ny:            0L, $
        nz:            0L, $
        display_mode:  0L, $
        display_enable: 0L, $
        display_min:   0L, $
        display_max:   0L, $
        autoscale:     0L, $
        flip_y:        0L, $
        frame_rate:    0L, $
        message:       0L, $
        exit:          0L, $
        timer:         0L $
    }

    fonts = {epics_ad_display_fonts, $
        normal: '', $
        heading1: '', $
        heading2: '' $
    }

    epics_ad_display = {epics_ad_display, $
        widgets:        widgets, $
        fonts:          fonts, $
        image_display:  obj_new(), $
        iimage_obj:     obj_new(), $
        detector:       obj_new(), $
        pData:          ptr_new(), $
        true_color:     0L, $
        base_pv:        "", $
        tv_window:      0L, $
        connected:      0L, $
        nx:             0L, $
        ny:             0L, $
        nz:             0L, $
        hSize:          0L, $
        vSize:          0L, $
        display_mode:   0L, $
        display_enable: 0L, $
        autoscale:      0L, $
        display_min:    0., $
        display_max:    0., $
        flip_y:         0L, $
        image_counter:  0L, $
        last_update:    0.0D, $
        frames_per_loop: 0L, $
        display_interval: 0.0, $
        connect_interval: 0.0 $
    }
end
