#include "throttler.h"

#include <algorithm>

Throttler::Throttler(double limit)
{
    reset(limit);
}

void Throttler::reset(double limit)
{
    available_ = limit_ = limit;
    refillAmount_ = limit / 1000.0;
    epicsTimeGetCurrent(&lastRefill_);
}

double Throttler::refill()
{
    epicsTimeStamp now;
    epicsTimeGetCurrent(&now);

    int refillCount = epicsTimeDiffInSeconds(&now, &lastRefill_)*1000;

    if (refillCount) {
        available_ = std::min(limit_, available_ + refillCount*refillAmount_);
        lastRefill_ = now;
    }

    return available_;
}

bool Throttler::tryTake(double tokens)
{
    if (tokens > refill())
        return false;

    available_ -= tokens;
    return true;
}
