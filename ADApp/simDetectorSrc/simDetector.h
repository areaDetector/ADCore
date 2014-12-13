#include <epicsEvent.h>
#include "ADDriver.h"

/** Simulation detector driver; demonstrates most of the features that areaDetector drivers can support. */
class epicsShareClass simDetector : public ADDriver {
public:
    simDetector(const char *portName, int maxSizeX, int maxSizeY, NDDataType_t dataType,
                int maxBuffers, size_t maxMemory,
                int priority, int stackSize);

    /* These are the methods that we override from ADDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual void setShutter(int open);
    virtual void report(FILE *fp, int details);
    void simTask(); /**< Should be private, but gets called from C, so must be public */

protected:
    int SimGainX;
    #define FIRST_SIM_DETECTOR_PARAM SimGainX
    int SimGainY;
    int SimGainRed;
    int SimGainGreen;
    int SimGainBlue;
    int SimNoise;
    int SimResetImage;
    int SimMode;
    int SimPeakStartX;
    int SimPeakStartY;
    int SimPeakWidthX;
    int SimPeakWidthY;
    int SimPeakNumX;
    int SimPeakNumY;
    int SimPeakStepX;
    int SimPeakStepY;
    int SimPeakHeightVariation;

    #define LAST_SIM_DETECTOR_PARAM SimPeakHeightVariation

private:
    /* These are the methods that are new to this class */
    template <typename epicsType> int computeArray(int sizeX, int sizeY);
    template <typename epicsType> int computeLinearRampArray(int sizeX, int sizeY);
    template <typename epicsType> int computePeaksArray(int sizeX, int sizeY);
    int computeImage();

    /* Our data */
    epicsEventId startEventId;
    epicsEventId stopEventId;
    NDArray *pRaw;
};

typedef enum {
    SimModeLinearRamp,
    SimModePeaks,
}SimModes_t;

#define SimGainXString          "SIM_GAIN_X"
#define SimGainYString          "SIM_GAIN_Y"
#define SimGainRedString        "SIM_GAIN_RED"
#define SimGainGreenString      "SIM_GAIN_GREEN"
#define SimGainBlueString       "SIM_GAIN_BLUE"
#define SimNoiseString          "SIM_NOISE"
#define SimResetImageString     "RESET_IMAGE"
#define SimModeString           "SIM_MODE"
#define SimPeakStartXString     "SIM_PEAK_START_X"
#define SimPeakStartYString     "SIM_PEAK_START_Y"
#define SimPeakWidthXString     "SIM_PEAK_WIDTH_X"
#define SimPeakWidthYString     "SIM_PEAK_WIDTH_Y"
#define SimPeakNumXString       "SIM_PEAK_NUM_X"
#define SimPeakNumYString       "SIM_PEAK_NUM_Y"
#define SimPeakStepXString      "SIM_PEAK_STEP_X"
#define SimPeakStepYString      "SIM_PEAK_STEP_Y"
#define SimPeakHeightVariationString  "SIM_PEAK_HEIGHT_VARIATION"


#define NUM_SIM_DETECTOR_PARAMS ((int)(&LAST_SIM_DETECTOR_PARAM - &FIRST_SIM_DETECTOR_PARAM + 1))

