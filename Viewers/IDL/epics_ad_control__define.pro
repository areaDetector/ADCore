pro epics_ad_control::setProperty, property, value
;+
; NAME:
;       EPICS_AD_CONTROL::setProperty
;
; PURPOSE:
;       This procedure sets one or more properties of the EPICS_AD_CONTROL object.
;
; CATEGORY:
;       IDL device class library.
;
; CALLING SEQUENCE:
;       d->setProperty, Property, Value
;
; INPUTS:
;       Property  The name of the property
;       Value:  The value for the property
;
; OUTPUTS:
;       None
;
; KEYWORD PARAMETERS:
;
; EXAMPLE:
;       d = obj_new('EPICS_AD_BASE')
;       d->setProperty, 'AcquireTime', 1.0
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers, December 13, 2008.
;-
    prop = self->findProperty(property)
    out = value
    if (prop.pvFlag eq self.pvFlagString) then out = byte(value)
    status = caput(prop.pvName, out)
    if (status ne 0) then begin
        message, 'Channel access error for PV ' + prop.pvName
        return
    endif
 end


function epics_ad_control::getProperty, property, string=string
;+
; NAME:
;       EPICS_AD_CONTROL::getProperty
;
; PURPOSE:
;       This function sets one or more properties of the EPICS_AD_CONTROL object.
;
; CATEGORY:
;       IDL device class library.
;
; CALLING SEQUENCE:
;       Value = d->getProperty(Property)
;
; INPUTS:
;       Property  The name of the property
;
; OUTPUTS:
;       Value:  The current value of the property
;
; KEYWORD PARAMETERS:
;       String: Set this flag to 1 to force the value to be read as a string, or
;               0 to force the value to be read as an integer
;
; EXAMPLE:
;       d = obj_new('EPICS_AD_BASE')
;       Value = d->getProperty('AcquireTime')
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers, December 13, 2008.
;-
    prop = self->findProperty(property)
    str = 0
    if (prop.pvFlag eq self.pvFlagEnum) then str=1
    if (n_elements(string) ne 0) then str=string
    status = caget(prop.pvName, value, string=str)
    if (status ne 0) then begin
        message, 'Channel access error for PV ' + prop.pvName
        return, 0
    endif
    if (prop.pvFlag eq self.pvFlagString) then value = string(value)
    return, value
end

function epics_ad_control::findProperty, name

    for i=0, self.lastProperty do begin
        if strcmp(name, (self.properties[i]).name, /fold_case) then return, self.properties[i]
    endfor
    message, 'Unknown property: ' + name
end

pro epics_ad_control::showProperties
;+
; NAME:
;       EPICS_AD_CONTROL::showProperties
;
; PURPOSE:
;       This function prints out the current values of all the properties of
;       the EPICS_AD_CONTROL object.
;
; CATEGORY:
;       IDL device class library.
;
; CALLING SEQUENCE:
;       d->showProperties
;
; INPUTS:
;       None
;
; OUTPUTS:
;       None
;
; KEYWORD PARAMETERS:
;       None

; EXAMPLE:
;       d = obj_new('EPICS_AD_BASE')
;       d->showProperties
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers, December 13, 2008.
;-
    print, format='(a, t30, a, t70, a, t75, a)', 'Property name', 'PV name', 'Flag', 'Value'
    print
    for i=0, self.lastProperty do begin
        prop = self.properties[i]
        value = self->getProperty(prop.name)
        print, format='(a, t30, a, t70, i1, t75, a)', prop.name, prop.pvName, prop.pvFlag, strtrim(value,2)
    endfor
end

pro epics_ad_control::addProperty, name, pvName, string=string
;+
; NAME:
;       EPICS_AD_CONTROL::addProperty
;
; PURPOSE:
;       This procedure adds a new property to the EPICS_AD_CONTROL object.
;       It is normally called only from the derived classes, not directly by
;       the user.
;
; CATEGORY:
;       IDL device class library.
;
; CALLING SEQUENCE:
;       d->addProperty, PropertyName, PVName
;
; INPUTS:
;       PropertyName: The name of this property
;       PVName: The EPICS PV name for this property
;
; OUTPUTS:
;       None
;
; KEYWORD PARAMETERS:
;       None

; EXAMPLE:
;       d = obj_new('EPICS_AD_BASE')
;       d->showProperties
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers, December 13, 2008.
;-
    prop = {epics_ad_control_property}
    prop.name = name
    prop.pvName = self.prefix + pvName
    status = casetmonitor(prop.pvName)
    if (status ne 0) then message, 'Error, cannot connect to PV ' + prop.pvName
    status = cagetcountandtype(prop.pvName, count, type)
    if (status ne 0) then message, 'Error, cannot get count and type for PV ' + prop.pvName
    if (type[0] eq 3) then prop.pvFlag = self.pvFlagEnum
    if (keyword_set(string)) then prop.pvFlag = self.pvFlagString
    self.lastProperty++
    if (self.lastProperty ge n_elements(self.properties)) then begin
        message, 'Too many properties'
        return
    endif
    self.properties[self.lastProperty] = prop
end

function epics_ad_control::init
;+
; NAME:
;       EPICS_AD_CONTROL::INIT
;
; PURPOSE:
;       This is the initialization code which is invoked when a new object of
;       type EPICS_AD_CONTROL is created.  It cannot be called directly, but only
;       indirectly by the IDL OBJ_NEW() function.
;
; CATEGORY:
;       IDL device class library.
;
; CALLING SEQUENCE:
;       Result = OBJ_NEW('EPICS_AD_CONTROL')
;
; INPUTS:
;       None
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
    self.lastProperty = -1
    self.pvFlagNormal = 0
    self.pvFlagEnum   = 1
    self.pvFlagString = 2
    return, 1
end

pro epics_ad_control__define
;+
; NAME:
;       EPICS_AD_CONTROL__DEFINE
;
; PURPOSE:
;       This is the definition code which is invoked when a new object of
;       type EPICS_AD_CONTROL is created.  It cannot be called directly, but only
;       indirectly by the IDL OBJ_NEW() function.  It defines the data
;       structures used for the EPICS_AD_CONTROL class.
;
; CATEGORY:
;       IDL device class library.
;
; CALLING SEQUENCE:
;       Result = OBJ_NEW('EPICS_AD_CONTROL')
;
; INPUTS:
;       None
;
; OUTPUTS:
;       None
;
; RESTRICTIONS:
;       This routine cannot be called directly.  It is called indirectly when
;       creating a new object of class EPICS_AD_CONTROL by the IDL OBJ_NEW()
;       function.
;
; EXAMPLE:
;       d = obj_new('EPICS_AD_CONTROL')
;       Time = d->getProperty('AcquireTime_RBV')
;
; MODIFICATION HISTORY:
;       Written by:     Mark Rivers, December 13, 2008
;-
    prop = {epics_ad_control_property, $
        name: "", $
        pvName: "", $
        pvFlag: 0L $
    }
    MAX_PROPERTIES=100
    properties = replicate(prop, MAX_PROPERTIES)
    epics_ad_control = {epics_ad_control, $
        properties: properties, $
        pvFlagNormal:   0L, $
        pvFlagEnum:     0L, $
        pvFlagString:   0L, $
        prefix:         "", $
        lastProperty:   0L  $
    }
end
