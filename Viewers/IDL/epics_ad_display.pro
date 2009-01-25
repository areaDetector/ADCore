pro epics_ad_display, image_pv

;+
; NAME:
;       EPICS_AD_DISPLAY
;
; PURPOSE:
;       This procedure is a general purpose routine to display EPICS arrays as
;       images.
;       It uses either the built-in IDL routine TV for simple image display, or our routine IMAGE_DISPLAY to perform interactive pan, 
;       zoom and scroll, live update of row and column profiles, etc.
;
; CATEGORY:
;       Imaging
;
; CALLING SEQUENCE:
;       EPICS_AD_DISPLAY, ImagePV
;
; INPUTS:
;       ImagePV:  The base name of an areaDetector NDStdArrays object that contains the image data.  This name is the
;                 $(P)$(R) values passed to NDStdArrays.template when the EPICS database is loaded.  This database
;                 contains not only the image data itself, but also PVs that contain the image dimensions, etc.
;
; PROCEDURE:
;       This procedure simply create a new object of class EPICS_AD_DISPLAY.
;       See the documentation for EPICS_AD_DISPLAY for more information.
;
; PREBUILT VERSION:
;       In addition to the source code version of these files, the file epics_ad_display.sav
;       is included in the distribution tar file.  This file can be run for free 
;       (no IDL license needed) with the IDL Virtual Machine.
;
; IMPORTANT NOTES:
;       The environment variable EZCA_IDL_SHARE must be set to point to the complete path
;       to the shareable library ezcaIDL.so (Linux, Unix, and Mac) or ezcaIDL.dll (Windows).
;       Note that the ezcaIDL shareable library or DLL must be built with EPICS R3.14 or later
;       in order to use arrays larger than 16000 bytes.
;
;       The environment variable EPICS_CA_MAX_ARRAY_BYTES must be set to at least the number
;       of bytes in the the image data array.
;
; EXAMPLE:
;       IDL> epics_ad_display, 'GSE-PILATUS1:Image1:'
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers (21-Jul-2007)
;-

    t = obj_new('epics_ad_display', image_pv)

end

