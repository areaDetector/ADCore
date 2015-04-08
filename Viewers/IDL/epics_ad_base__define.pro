pro epics_ad_base::wait, property, value, transition=transition, delay=delay
;+
; NAME:
;       EPICS_AD_BASE::WAIT
;
; PURPOSE:
;       This procedure waits for a detector property to have a specified value,
;       to make a transition, or both.
;
; CATEGORY:
;       IDL device class library.
;
; CALLING SEQUENCE:
;       D->WAIT(PROPERTY, VALUE, TRANSITION=transition, DELAY=delay)
;
; INPUTS:
;       Property:  The property to wait for.
;
; OPTIONAL INPUTS:
;       Value:  The procedure will wait for the property to have this value.  If this input 
;               is omitted then it will only wait for the transition, if /TRANSITION is specified.
;
; KEYWORD INPUTS:
;       Transition: If this keyword is set then the procedure will wait for a transition
;           (channel access monitor with new value) on the property.  If /TRANSITION and
;           Value are both specified, then it first waits for the transition and then
;           it waits for the value. 
;           This is useful for ensuring, for example, when waiting for 'Acquire' to be 'Done' 
;           that it has changed state, so it is not 'Done' because acquisition has not yet started. 
;
;       Delay:  The time to wait when polling for transitions and waiting for the value to be
;           equal to the desired value.
;
; EXAMPLE:
;       d = obj_new('EPICS_AD_BASE')
;       d->wait, 'Acquire', 'Done', /transition
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers, July 20, 2010
;-

    prop = self->findProperty(property)
    if (n_elements(delay) eq 0) then delay = 0.01
    
    ; If /TRANSITION was specified then wait for a transition on input. 
    if (keyword_set(transition)) then begin
        ; Read the property to clear the monitor flag
        t = caGet(prop.pvName)
        while(1) do begin
            new = caCheckMonitor(prop.pvName)
            if (new eq 1) then break
            wait, delay
        endwhile
    endif

    if (n_elements(value) ne 0) then begin
        stringType = (size(value, /TNAME) eq 'STRING')
        while (1) do begin
            new = self->getProperty(property, string=stringType)
            if (new eq value) then break
            wait, delay
        endwhile
    endif
end

function epics_ad_base::init, prefix
;+
; NAME:
;       EPICS_AD_BASE::INIT
;
; PURPOSE:
;       This is the initialization code which is invoked when a new object of
;       type EPICS_AD_BASE is created.  It cannot be called directly, but only
;       indirectly by the IDL OBJ_NEW() function.
;
; CATEGORY:
;       IDL device class library.
;
; CALLING SEQUENCE:
;       Result = OBJ_NEW('EPICS_AD_BASE')
;
; INPUTS:
;       Prefix:  The prefix string for the EPICS PVs defined in ADBase.template
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
;       d = obj_new('EPICS_AD_BASE')
;       d->setProperty, 'Acquire', 1
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers, December 13, 2008
;-

    status = self->epics_ad_control::init()
    if (status ne 1) then return, status
    self.prefix = prefix

    catch, err
    if (err ne 0) then begin
       print, !ERROR_STATE.MSG
       return, 0
    endif

    self->addProperty, 'Manufacturer_RBV',  'Manufacturer_RBV'
    self->addProperty, 'Model_RBV',         'Model_RBV'
    self->addProperty, 'Gain',              'Gain'
    self->addProperty, 'Gain_RBV',          'Gain_RBV'

    self->addProperty, 'BinX',              'BinX'
    self->addProperty, 'BinX_RBV',          'BinX_RBV'
    self->addProperty, 'BinY',              'BinY'
    self->addProperty, 'BinY_RBV',          'BinY_RBV'

    self->addProperty, 'MinX',              'MinX'
    self->addProperty, 'MinX_RBV',          'MinX_RBV'
    self->addProperty, 'MinY',              'MinY'
    self->addProperty, 'MinY_RBV',          'MinY_RBV'

    self->addProperty, 'SizeX',             'SizeX'
    self->addProperty, 'SizeX_RBV',         'SizeX_RBV'
    self->addProperty, 'SizeY',             'SizeY'
    self->addProperty, 'SizeY_RBV',         'SizeY_RBV'

    self->addProperty, 'MaxSizeX_RBV',      'MaxSizeX_RBV'
    self->addProperty, 'MaxSizeY_RBV',      'MaxSizeY_RBV'

    self->addProperty, 'ReverseX',          'ReverseX'
    self->addProperty, 'ReverseX_RBV',      'ReverseX_RBV'
    self->addProperty, 'ReverseY',          'ReverseY'
    self->addProperty, 'ReverseY_RBV',      'ReverseY_RBV'

    self->addProperty, 'ArraySizeX_RBV',    'ArraySizeX_RBV'
    self->addProperty, 'ArraySizeY_RBV',    'ArraySizeY_RBV'

    self->addProperty, 'ArraySize_RBV',     'ArraySize_RBV'

    self->addProperty, 'DataType',          'DataType'
    self->addProperty, 'DataType_RBV',      'DataType_RBV'

    self->addProperty, 'FrameType',         'FrameType'
    self->addProperty, 'FrameType_RBV',     'FrameType_RBV'

    self->addProperty, 'ImageMode',         'ImageMode'
    self->addProperty, 'ImageMode_RBV',     'ImageMode_RBV'

    self->addProperty, 'TriggerMode',           'TriggerMode'
    self->addProperty, 'TriggerMode_RBV',       'TriggerMode_RBV'

    self->addProperty, 'AcquireTime',           'AcquireTime'
    self->addProperty, 'AcquireTime_RBV',       'AcquireTime_RBV'

    self->addProperty, 'AcquirePeriod',         'AcquirePeriod'
    self->addProperty, 'AcquirePeriod_RBV',     'AcquirePeriod_RBV'

    self->addProperty, 'TimeRemaining_RBV',     'TimeRemaining_RBV'

    self->addProperty, 'NumExposures',          'NumExposures'
    self->addProperty, 'NumExposures_RBV',      'NumExposures_RBV'
    self->addProperty, 'NumExposuresCounter_RBV', 'NumExposuresCounter_RBV'

    self->addProperty, 'NumImages',             'NumImages'
    self->addProperty, 'NumImages_RBV',         'NumImages_RBV'
    self->addProperty, 'NumImagesCounter_RBV',  'NumImagesCounter_RBV'

    self->addProperty, 'ArrayCounter',          'ArrayCounter'
    self->addProperty, 'ArrayCounter_RBV',      'ArrayCounter_RBV'
    self->addProperty, 'ArrayRate_RBV',         'ArrayRate_RBV'

    self->addProperty, 'Acquire',               'Acquire'
    self->addProperty, 'Acquire_RBV',           'Acquire_RBV'

    self->addProperty, 'DetectorState_RBV',     'DetectorState_RBV'

    self->addProperty, 'ArrayCallbacks',        'ArrayCallbacks'
    self->addProperty, 'ArrayCallbacks_RBV',    'ArrayCallbacks_RBV'

    self->addProperty, 'StatusMessage_RBV',     'StatusMessage_RBV', /string
    self->addProperty, 'StringToServer_RBV',    'StringToServer_RBV', /string
    self->addProperty, 'StringFromServer_RBV',  'StringFromServer_RBV', /string

    self->addProperty, 'ReadStatus',            'ReadStatus'

    self->addProperty, 'ShutterMode',           'ShutterMode'
    self->addProperty, 'ShutterMode_RBV',       'ShutterMode_RBV'

    self->addProperty, 'ShutterControl',        'ShutterControl'
    self->addProperty, 'ShutterControl_RBV',    'ShutterControl_RBV'

    self->addProperty, 'ShutterStatus_RBV',     'ShutterStatus_RBV'

    self->addProperty, 'ShutterStatusEPICS_RBV','ShutterStatusEPICS_RBV'

    self->addProperty, 'ShutterOpenDelay',      'ShutterOpenDelay'
    self->addProperty, 'ShutterOpenDelay_RBV',  'ShutterOpenDelay_RBV'

    self->addProperty, 'ShutterCloseDelay',     'ShutterCloseDelay'
    self->addProperty, 'ShutterCloseDelay_RBV', 'ShutterCloseDelay_RBV'

    self->addProperty, 'Temperature',           'Temperature'
    self->addProperty, 'Temperature_RBV',       'Temperature_RBV'

    catch, /cancel

    return, 1
end

pro epics_ad_base__define
;+
; NAME:
;       EPICS_AD_BASE__DEFINE
;
; PURPOSE:
;       This is the definition code which is invoked when a new object of
;       type EPICS_AD_BASE is created.  It cannot be called directly, but only
;       indirectly by the IDL OBJ_NEW() function.  It defines the data
;       structures used for the EPICS_AD_BASE class.
;
; CATEGORY:
;       IDL device class library.
;
; CALLING SEQUENCE:
;       Result = OBJ_NEW('EPICS_AD_BASE')
;
; INPUTS:
;       None
;
; OUTPUTS:
;       None
;
; RESTRICTIONS:
;       This routine cannot be called directly.  It is called indirectly when
;       creating a new object of class EPICS_AD_BASE by the IDL OBJ_NEW()
;       function.
;
; EXAMPLE:
;       d = obj_new('EPICS_AD_BASE')
;       Time = d->getProperty('AcquireTime_RBV')
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers, December 13, 2008
;-
    epics_ad_base = {epics_ad_base, $
                     INHERITS epics_ad_control}
 end
