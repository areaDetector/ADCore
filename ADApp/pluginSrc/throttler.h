#ifndef THROTTLER_H
#define THROTTLER_H

#include <epicsTime.h>

class Throttler {

public:
    Throttler(double limit=0.0);
    void reset(double limit);
    double refill();
    bool tryTake(double tokens);

private:
    double limit_;              // Max tokens per second
    double available_;          // Available tokens
    double refillAmount_;       // How much to refill every msec
    epicsTimeStamp lastRefill_; // When did the last refill happen
};

#endif
