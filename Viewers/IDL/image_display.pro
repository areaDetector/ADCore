pro image_display, data, object, _EXTRA=extra
;+
; NAME:
;       IMAGE_DISPLAY
;
; PURPOSE:
;       This procedure is a general purpose image display routine.
;       It allows interactive pan, zoom and scroll, live update of row and
;       column profiles, etc.
;
; CATEGORY:
;       Imaging
;
; CALLING SEQUENCE:
;       IMAGE_DISPLAY, Data, Object_ref
;
; INPUTS:
;       Data:   A 2-D array to be displayed
;
; KEYWORD PARAMETERS:
;       This procedure simply passes all keywords to IMAGE_DISPLAY::INIT by
;       the keyword inheritence mechanism (_EXTRA).  See the documentation
;       for IMAGE_DISPLAY::INIT for more information.
;
; OPTIONAL OUTPUTS:
;       Object_ref:  
;           The object reference for the resulting IMAGE_DISPLAY object.  This
;           is useful for controlling the IMAGE_DISPLAY object from other
;           routines.
;
; PROCEDURE:
;       This procedure simply create a new object of class IMAGE_DISPLAY.
;       See the documentation for the IMAGE_DISPLAY class library for more information.
;
; EXAMPLE:
;       IDL> a = DIST(512)
;       IDL> image_display, a
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers (3-DEC-1998)
;-

    object = obj_new('image_display', data, _EXTRA=extra)
end
