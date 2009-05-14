
function epics_ndplugin_base::init, prefix
;+
; NAME:
;       EPICS_NDPLUGIN_BASE::INIT
;
; PURPOSE:
;       This is the initialization code which is invoked when a new object of
;       type EPICS_NDPLUGIN_BASE is created.  It cannot be called directly, but only
;       indirectly by the IDL OBJ_NEW() function.
;
; CATEGORY:
;       IDL device class library.
;
; CALLING SEQUENCE:
;       Result = OBJ_NEW('EPICS_NDPLUGIN_BASE')
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
;       p = obj_new('EPICS_NDPLUGIN_BASE')
;       p->setProperty, 'MinCallbackTime', 1.0
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

    self->addProperty, 'NDArrayPort',                  'NDArrayPort'
    self->addProperty, 'NDArrayPort_RBV',              'NDArrayPort_RBV'
    self->addProperty, 'NDArrayAddress',               'NDArrayAddress'
    self->addProperty, 'NDArrayAddress_RBV',           'NDArrayAddress_RBV'
    self->addProperty, 'EnableCallbacks',              'EnableCallbacks'
    self->addProperty, 'EnableCallbacks_RBV',          'EnableCallbacks_RBV'
    self->addProperty, 'MinCallbackTime',              'MinCallbackTime'
    self->addProperty, 'MinCallbackTime_RBV',          'MinCallbackTime_RBV'
    self->addProperty, 'BlockingCallbacks',            'BlockingCallbacks'
    self->addProperty, 'BlockingCallbacks_RBV',        'BlockingCallbacks_RBV'
    self->addProperty, 'ArrayCounter',                 'ArrayCounter'
    self->addProperty, 'ArrayCounter_RBV',             'ArrayCounter_RBV'
    self->addProperty, 'ArrayRate_RBV',                'ArrayRate_RBV'
    self->addProperty, 'DroppedArrays',                'DroppedArrays'
    self->addProperty, 'DroppedArrays_RBV',            'DroppedArrays_RBV'
    self->addProperty, 'NDimensions_RBV',              'NDimensions_RBV'
    self->addProperty, 'Dimensions_RBV',               'Dimensions_RBV'
    self->addProperty, 'ArraySize0_RBV',               'ArraySize0_RBV'
    self->addProperty, 'ArraySize1_RBV',               'ArraySize1_RBV'
    self->addProperty, 'ArraySize2_RBV',               'ArraySize2_RBV'
    self->addProperty, 'DataType_RBV',                 'DataType_RBV'
    self->addProperty, 'ColorMode_RBV',                'ColorMode_RBV'
    self->addProperty, 'UniqueId_RBV',                 'UniqueId_RBV'
    self->addProperty, 'TimeStamp_RBV',                'TimeStamp_RBV'

    catch, /cancel

    return, 1
end

pro epics_ndplugin_base__define
;+
; NAME:
;       EPICS_NDPLUGIN_BASE__DEFINE
;
; PURPOSE:
;       This is the definition code which is invoked when a new object of
;       type EPICS_NDPLUGIN_BASE is created.  It cannot be called directly, but only
;       indirectly by the IDL OBJ_NEW() function.  It defines the data
;       structures used for the EPICS_NDPLUGIN_BASE class.
;
; CATEGORY:
;       IDL device class library.
;
; CALLING SEQUENCE:
;       Result = OBJ_NEW('EPICS_NDPLUGIN_BASE')
;
; INPUTS:
;       None
;
; OUTPUTS:
;       None
;
; RESTRICTIONS:
;       This routine cannot be called directly.  It is called indirectly when
;       creating a new object of class EPICS_NDPLUGIN_BASE by the IDL OBJ_NEW()
;       function.
;
; EXAMPLE:
;       p = obj_new('EPICS_NDPLUGIN_BASE')
;       Enable = p->getProperty('EnableCallbacks')
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers, December 16, 2008
;-
    epics_ndplugin_base = {epics_ndplugin_base, $
                     INHERITS epics_ad_control}
 end
