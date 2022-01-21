/**
 * Area Detector class enabling multi-ROI driver for the Andor CCD.
 * This class is used by CCD camera modules that permit multiple regions-of-interest.
 *
 * Multi-ROI is typically used for multi-track spectrocopy application.
 *
 * There are 3 use cases:
 *    1 - The user sets only the track start array.
 *        This provides a single height track at thos positions.
 *    2 - The user sets the start and end tracks arrays.
 *        This provides a fully-binned track between the start and end positions.
 *    3 - The user provides start, end and binning values.
 *        This provides (a less than fully binned) track between the start and end positions.
 *
 * @author Peter Heesterman
 * @date Nov 2019
 *
 */

#include "CCDMultiTrack.h"

static const char* CCDMultiTrackStartString = "CCD_MULTI_TRACK_START";
static const char* CCDMultiTrackEndString = "CCD_MULTI_TRACK_END";
static const char* CCDMultiTrackBinString = "CCD_MULTI_TRACK_BIN";

CCDMultiTrack::CCDMultiTrack(asynPortDriver* asynPortDriver)
{
    mPortDriver = asynPortDriver;
    asynPortDriver->createParam(CCDMultiTrackStartString, asynParamInt32Array, &mCCDMultiTrackStart);
    asynPortDriver->createParam(CCDMultiTrackEndString, asynParamInt32Array, &mCCDMultiTrackEnd);
    asynPortDriver->createParam(CCDMultiTrackBinString, asynParamInt32Array, &mCCDMultiTrackBin);
}

void CCDMultiTrack::storeTrackAttributes(NDAttributeList* pAttributeList)
{
    std::string ROIString = "ROI";
    std::string TrackString = "Track ";
    if (pAttributeList)
    {
        for (size_t TrackNum = 0; TrackNum < size(); TrackNum++)
        {
            // Add new attributes listing.
            char Buf[10];
            epicsSnprintf(Buf, 10, "%zd", TrackNum + 1);
            std::string TrackNumString = Buf;
            std::string TrackStartName = ROIString + TrackNumString + "start";
            std::string TrackStartDescription = TrackString + TrackNumString + " start";
            int TrackStart = CCDMultiTrack::TrackStart(TrackNum);
            pAttributeList->add(TrackStartName.c_str(), TrackStartDescription.c_str(), NDAttrInt32, &TrackStart);
            // Set new attributes.
            std::string TrackEndName = ROIString + TrackNumString + "end";
            std::string TrackEndDescription = TrackString + TrackNumString + " end";
            int TrackEnd = CCDMultiTrack::TrackEnd(TrackNum);
            pAttributeList->add(TrackEndName.c_str(), TrackEndDescription.c_str(), NDAttrInt32, &TrackEnd);
            // Set new attributes.
            std::string TrackBinName = ROIString + TrackNumString + "bin";
            std::string TrackBinDescription = TrackString + TrackNumString + " end";
            int TrackBin = CCDMultiTrack::TrackBin(TrackNum);
            pAttributeList->add(TrackBinName.c_str(), TrackBinDescription.c_str(), NDAttrInt32, &TrackBin);
        }
    }
}

void CCDMultiTrack::writeTrackStart(epicsInt32 *value, size_t nElements)
{
    std::vector<int> TrackStart(nElements);
    for (size_t TrackNum = 0; TrackNum < TrackStart.size(); TrackNum++)
    {
        if ((TrackNum > 0) && (value[TrackNum] <= CCDMultiTrack::TrackStart(TrackNum - 1)))
            throw std::string("Tracks starts must be in ascending order");
        /* Copy the new data */
        TrackStart[TrackNum] = value[TrackNum];
    }
    if (mTrackStart != TrackStart)
        mTrackStart = TrackStart;
    std::vector<int> TrackEnd(mTrackStart.size());
    /* If binning is already set, this can define the track end. */
    for (size_t TrackNum = 0; TrackNum < TrackEnd.size(); TrackNum++)
        TrackEnd[TrackNum] = CCDMultiTrack::TrackStart(TrackNum) + TrackHeight(TrackNum) - 1;
    std::vector<int> TrackBin(mTrackStart.size());
    /* If track end is already set, this can define the binning. */
    for (size_t TrackNum = 0; TrackNum < TrackBin.size(); TrackNum++)
        TrackBin[TrackNum] = CCDMultiTrack::TrackBin(TrackNum);
    if (mTrackEnd != TrackEnd)
    {
        mTrackEnd = TrackEnd;
        mPortDriver->doCallbacksInt32Array(&(mTrackEnd[0]), mTrackEnd.size(), CCDMultiTrackEnd(), 0);
    }
    if (mTrackBin != TrackBin)
    {
        mTrackBin = TrackBin;
        mPortDriver->doCallbacksInt32Array(&(mTrackBin[0]), mTrackBin.size(), CCDMultiTrackBin(), 0);
    }
}

void CCDMultiTrack::writeTrackEnd(epicsInt32 *value, size_t nElements)
{
    std::vector<int> TrackEnd(nElements);
    mTrackEnd.resize(nElements);
    for (size_t TrackNum = 0; TrackNum < TrackEnd.size(); TrackNum++)
    {
        if ((TrackNum > 0) && (value[TrackNum] <= CCDMultiTrack::TrackEnd(TrackNum - 1)))
            throw std::string("Tracks ends must be in ascending order");
        /* Copy the new data */
        TrackEnd[TrackNum] = value[TrackNum];
    }
    if (mTrackEnd != TrackEnd)
        mTrackEnd = TrackEnd;
    std::vector<int> TrackBin(mTrackEnd.size());
    /* If track start is already set, this can define the binning */
    for (size_t TrackNum = 0; TrackNum < TrackBin.size(); TrackNum++)
        TrackBin[TrackNum] = TrackHeight(TrackNum);
    if (mTrackBin != TrackBin)
    {
        mTrackBin = TrackBin;
        mPortDriver->doCallbacksInt32Array(&(mTrackBin[0]), mTrackBin.size(), CCDMultiTrackBin(), 0);
    }
}

void CCDMultiTrack::writeTrackBin(epicsInt32 *value, size_t nElements)
{
    std::vector<int> TrackBin(nElements);
    for (size_t TrackNum = 0; TrackNum < TrackBin.size(); TrackNum++)
    {
        if (value[TrackNum] < 1)
            throw std::string("The track binning must be >= 1.");
        if (TrackHeight(TrackNum) % value[TrackNum] != 0)
            throw std::string("Track height must be divisible by binning.");
        /* Copy the new data */
        TrackBin[TrackNum] = value[TrackNum];
    }
    if (mTrackBin != TrackBin)
        mTrackBin = TrackBin;
}

asynStatus CCDMultiTrack::writeInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    if (function == CCDMultiTrackStart())
        writeTrackStart(value, nElements);
    else if (function == CCDMultiTrackEnd())
        writeTrackEnd(value, nElements);
    else if (function == CCDMultiTrackBin())
        writeTrackBin(value, nElements);
    else {
        status = asynError;
    }
    return status;
}

int CCDMultiTrack::DataHeight() const
{
    int DataHeight = 0;
    for (size_t TrackNum = 0; TrackNum < size(); TrackNum++)
        DataHeight += CCDMultiTrack::DataHeight(TrackNum);
    return DataHeight;
}
