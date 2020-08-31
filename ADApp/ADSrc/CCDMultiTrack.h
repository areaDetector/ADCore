/**
 * Area Detector class enabling multi-ROI driver for the Andor CCD.
 * This class is used by CCD camera modules that permit multiple regions-of-interest.
 *
 * Multi-ROI is typically used for multi-track spectrocopy application.
 *
 * @author Peter Heesterman
 * @date Nov 2019
 *
 */
#ifndef CCD_MULTI_TRACK_H
#define CCD_MULTI_TRACK_H

#include <asynPortDriver.h>
#include <NDAttributeList.h>
#include <ADCoreAPI.h>

class ADCORE_API CCDMultiTrack
{
    asynPortDriver* mPortDriver;

    int mCCDMultiTrackStart;
    int mCCDMultiTrackEnd;
    int mCCDMultiTrackBin;

    std::vector<int> mTrackStart;
    std::vector<int> mTrackEnd;
    std::vector<int> mTrackBin;

public:
    size_t size() const {
        return mTrackStart.size();
    }
    int CCDMultiTrackStart() const {
        return mCCDMultiTrackStart;
    }
    int CCDMultiTrackEnd() const {
        return mCCDMultiTrackEnd;
    }
    int CCDMultiTrackBin() const {
        return mCCDMultiTrackBin;
    }
    int TrackStart(size_t TrackNum) const {
        return (TrackNum < mTrackStart.size()) ? mTrackStart[TrackNum] : 0;
    };
    int TrackEnd(size_t TrackNum) const {
        return (TrackNum < mTrackEnd.size()) ? mTrackEnd[TrackNum] : TrackStart(TrackNum);
    }
    int TrackHeight(size_t TrackNum) const {
        return TrackEnd(TrackNum) + 1 - TrackStart(TrackNum);
    }
    int TrackBin(size_t TrackNum) const {
        return (TrackNum < mTrackBin.size()) ? mTrackBin[TrackNum] : TrackHeight(TrackNum);
    }
    int DataHeight() const;
    int DataHeight(size_t TrackNum) const {
        return TrackHeight(TrackNum) / TrackBin(TrackNum);
    }

    CCDMultiTrack(asynPortDriver* asynPortDriver);
    asynStatus writeInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements);

    void storeTrackAttributes(NDAttributeList* pAttributeList);

private:
    void writeTrackStart(epicsInt32 *value, size_t nElements);
    void writeTrackEnd(epicsInt32 *value, size_t nElements);
    void writeTrackBin(epicsInt32 *value, size_t nElements);

};

#endif //CCD_MULTI_TRACK_H
