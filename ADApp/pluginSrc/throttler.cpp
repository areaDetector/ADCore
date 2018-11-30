#include "throttler.h"

#include <algorithm>

typedef epicsMutex::guard_t Guard;

Throttler::Throttler(double limit) {
    reset(limit);
}

void Throttler::reset(double limit) {
    Guard G(mutex_);

    available_ = limit_ = limit;
    refillAmount_ = limit / 1000.0;
    epicsTimeGetCurrent(&lastRefill_);
}

double Throttler::refill() {
    Guard G(mutex_);

    epicsTimeStamp now;
    epicsTimeGetCurrent(&now);

    int refillCount = epicsTimeDiffInSeconds(&now, &lastRefill_)*1000;
    if (refillCount) {
        available_ = std::min(limit_, available_ + refillCount*refillAmount_);
        lastRefill_ = now;
    }

    return available_;
}

bool Throttler::tryTake(double tokens) {
    Guard G(mutex_);

    double available = refill();

    if (tokens > available)
        return false;

    available_ -= tokens;
    return true;
}
