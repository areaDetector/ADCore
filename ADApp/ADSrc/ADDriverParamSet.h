#ifndef ADDriverParamSet_H
#define ADDriverParamSet_H

#include "asynNDArrayDriverParamSet.h"


#define ADGainString "GAIN"

    /* Parameters that control the detector binning */
#define ADBinXString "BIN_X"
#define ADBinYString "BIN_Y"
#define ADMinXString "MIN_X"
#define ADMinYString "MIN_Y"
#define ADSizeXString "SIZE_X"
#define ADSizeYString "SIZE_Y"
#define ADMaxSizeXString "MAX_SIZE_X"
#define ADMaxSizeYString "MAX_SIZE_Y"
#define ADReverseXString "REVERSE_X"
#define ADReverseYString "REVERSE_Y"
#define ADFrameTypeString "FRAME_TYPE"
#define ADImageModeString "IMAGE_MODE"
#define ADTriggerModeString "TRIGGER_MODE"
#define ADNumExposuresString "NEXPOSURES"
#define ADNumImagesString "NIMAGES"
#define ADAcquireTimeString "ACQ_TIME"
#define ADAcquirePeriodString "ACQ_PERIOD"
#define ADStatusString "STATUS"
#define ADShutterControlString "SHUTTER_CONTROL"
#define ADShutterControlEPICSString "SHUTTER_CONTROL_EPICS"
#define ADShutterStatusString "SHUTTER_STATUS"
#define ADShutterModeString "SHUTTER_MODE"
#define ADShutterOpenDelayString "SHUTTER_OPEN_DELAY"
#define ADShutterCloseDelayString "SHUTTER_CLOSE_DELAY"
#define ADTemperatureString "TEMPERATURE"
#define ADTemperatureActualString "TEMPERATURE_ACTUAL"
#define ADNumImagesCounterString "NIMAGES_COUNTER"
#define ADNumExposuresCounterString "NEXPOSURES_COUNTER"
#define ADTimeRemainingString "TIME_REMAINING"
#define ADReadStatusString "READ_STATUS"
#define ADStatusMessageString "STATUS_MESSAGE"
#define ADStringToServerString "STRING_TO_SERVER"
#define ADStringFromServerString "STRING_FROM_SERVER"

/** ADDriver param set */
class ADDriverParamSet : public virtual asynNDArrayDriverParamSet {
public:
    ADDriverParamSet() {
        std::cout << "ADDriverParamSet" << std::endl;
        this->add(ADGainString, asynParamFloat64, &ADGain);
        this->add(ADBinXString, asynParamInt32, &ADBinX);
        this->add(ADBinYString, asynParamInt32, &ADBinY);
        this->add(ADMinXString, asynParamInt32, &ADMinX);
        this->add(ADMinYString, asynParamInt32, &ADMinY);
        this->add(ADSizeXString, asynParamInt32, &ADSizeX);
        this->add(ADSizeYString, asynParamInt32, &ADSizeY);
        this->add(ADMaxSizeXString, asynParamInt32, &ADMaxSizeX);
        this->add(ADMaxSizeYString, asynParamInt32, &ADMaxSizeY);
        this->add(ADReverseXString, asynParamInt32, &ADReverseX);
        this->add(ADReverseYString, asynParamInt32, &ADReverseY);
        this->add(ADFrameTypeString, asynParamInt32, &ADFrameType);
        this->add(ADImageModeString, asynParamInt32, &ADImageMode);
        this->add(ADNumExposuresString, asynParamInt32, &ADNumExposures);
        this->add(ADNumExposuresCounterString, asynParamInt32, &ADNumExposuresCounter);
        this->add(ADNumImagesString, asynParamInt32, &ADNumImages);
        this->add(ADNumImagesCounterString, asynParamInt32, &ADNumImagesCounter);
        this->add(ADAcquireTimeString, asynParamFloat64, &ADAcquireTime);
        this->add(ADAcquirePeriodString, asynParamFloat64, &ADAcquirePeriod);
        this->add(ADTimeRemainingString, asynParamFloat64, &ADTimeRemaining);
        this->add(ADStatusString, asynParamInt32, &ADStatus);
        this->add(ADTriggerModeString, asynParamInt32, &ADTriggerMode);
        this->add(ADShutterControlString, asynParamInt32, &ADShutterControl);
        this->add(ADShutterControlEPICSString, asynParamInt32, &ADShutterControlEPICS);
        this->add(ADShutterStatusString, asynParamInt32, &ADShutterStatus);
        this->add(ADShutterModeString, asynParamInt32, &ADShutterMode);
        this->add(ADShutterOpenDelayString, asynParamFloat64, &ADShutterOpenDelay);
        this->add(ADShutterCloseDelayString, asynParamFloat64, &ADShutterCloseDelay);
        this->add(ADTemperatureString, asynParamFloat64, &ADTemperature);
        this->add(ADTemperatureActualString, asynParamFloat64, &ADTemperatureActual);
        this->add(ADReadStatusString, asynParamInt32, &ADReadStatus);
        this->add(ADStatusMessageString, asynParamOctet, &ADStatusMessage);
        this->add(ADStringToServerString, asynParamOctet, &ADStringToServer);
        this->add(ADStringFromServerString, asynParamOctet, &ADStringFromServer);
    }

    int ADGain;
    #define FIRST_AD_PARAM_INDEX ADGain
    int ADBinX;
    int ADBinY;
    int ADMinX;
    int ADMinY;
    int ADSizeX;
    int ADSizeY;
    int ADMaxSizeX;
    int ADMaxSizeY;
    int ADReverseX;
    int ADReverseY;
    int ADFrameType;
    int ADImageMode;
    int ADNumExposures;
    int ADNumExposuresCounter;
    int ADNumImages;
    int ADNumImagesCounter;
    int ADAcquireTime;
    int ADAcquirePeriod;
    int ADTimeRemaining;
    int ADStatus;
    int ADTriggerMode;
    int ADShutterControl;
    int ADShutterControlEPICS;
    int ADShutterStatus;
    int ADShutterMode;
    int ADShutterOpenDelay;
    int ADShutterCloseDelay;
    int ADTemperature;
    int ADTemperatureActual;
    int ADReadStatus;
    int ADStatusMessage;
    int ADStringToServer;
    int ADStringFromServer;
};

#endif  // ADDriverParamSet_H
