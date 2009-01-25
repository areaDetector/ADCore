
function epics_ad_file::init, prefix
;+
; NAME:
;       EPICS_AD_FILE::INIT
;
; PURPOSE:
;       This is the initialization code which is invoked when a new object of
;       type EPICS_AD_FILE is created.  It cannot be called directly, but only
;       indirectly by the IDL OBJ_NEW() function. 
;
; CATEGORY:
;       IDL device class library.
;
; CALLING SEQUENCE:
;       Result = OBJ_NEW('EPICS_AD_FILE')
;
; INPUTS:
;       Prefix:  The prefix string for the EPICS PVs defined in ADFile.template
;
; OUTPUTS:
;       This function returns success (1) if it was able to connect to all of the
;       EPICS PVs for this object.  It returns failure (0) if is was unable to
;       connect to any EPICS PV.
;
; RESTRICTIONS:
;       This function only works on systems that have the EPICS IDL Channel Access
;       software installed.  This requires the shareable object ezcaIDL.dll or
;       ezcaIDL.so.  The EZCA_IDL_SHARE environment variable must point to this
;       shareable object file.  It must be a version of this library built with
;       EPICS 3.14 to transfer arrays larger than 16,000 bytes. 
;       The EPICS_CA_MAX_ARRAY_BYTES environment variable must be set to a value
;       at least as large as the biggest array that will be used.
;
; EXAMPLE:
;       f = obj_new('EPICS_AD_FILE')
;       f->setProperty, 'FilePath', '/home/epics/data/'
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers, December 16, 2008
;-

    status = self->epics_ad_control::init()
    if (status ne 1) then return, status
    self.prefix = prefix
    
    catch, err
    if (err ne 0) then begin
       print, !ERROR_STATE.MSG
       return, 0
    endif

    self->addProperty, 'FilePath',          'FilePath'
    self->addProperty, 'FilePath_RBV',      'FilePath_RBV'
    self->addProperty, 'FileName',          'FileName'
    self->addProperty, 'FileName_RBV',      'FileName_RBV'
    self->addProperty, 'FileNumber',        'FileNumber'
    self->addProperty, 'FileNumber_RBV',    'FileNumber_RBV'
    self->addProperty, 'FileTemplate',      'FileTemplate'
    self->addProperty, 'FileTemplate_RBV',  'FileTemplate_RBV'

    catch, /cancel

    return, 1
end

pro epics_ad_file__define
;+
; NAME:
;       EPICS_AD_FILE__DEFINE
;
; PURPOSE:
;       This is the definition code which is invoked when a new object of
;       type EPICS_AD_FILE is created.  It cannot be called directly, but only
;       indirectly by the IDL OBJ_NEW() function.  It defines the data
;       structures used for the EPICS_AD_FILE class.
;
; CATEGORY:
;       IDL device class library.
;
; CALLING SEQUENCE:
;       Result = OBJ_NEW('EPICS_AD_FILE')
;
; INPUTS:
;       None
;
; OUTPUTS:
;       None
;
; RESTRICTIONS:
;       This routine cannot be called directly.  It is called indirectly when
;       creating a new object of class EPICS_AD_FILE by the IDL OBJ_NEW()
;       function.
;
; EXAMPLE:
;       f = obj_new('EPICS_AD_FILE')
;       Template = f->getProperty('FileTemplate')
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers, December 16, 2008
;-
    epics_ad_file = {epics_ad_file, $
                     INHERITS epics_ad_control}
 end
