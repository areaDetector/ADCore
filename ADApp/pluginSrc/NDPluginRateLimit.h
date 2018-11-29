#ifndef NDPluginRateLimit_H
#define NDPluginRateLimit_H

#include <epicsTypes.h>

#include "NDPluginDriver.h"

#define NDRateLimitModeString         "MODE"             /* (NDRateLimitMode_t r/w) Mode: off / array rate (arrays/s) / byte rate (bytes/s) */
#define NDRateLimitLimitString        "LIMIT"            /* (int r/w) The limit to be applied */
#define NDRateLimitRefillTimeString   "REFILL_TIME"      /* (int r/o) Time (ms) between refills */
#define NDRateLimitRefillAmountString "REFILL_AMOUNT"    /* (int r/o) Amount to be refilled on every update, derived from Limit and RefillTime*/
#define NDRateLimitNumTokensString    "NUM_TOKENS"       /* (int r/o) Number of tokens currently in the bucket */
#define NDRateLimitDroppedString      "DROPPED"          /* (int r/o) Number of arrays dropped so far */


/* Rate limits the arrays passing through this plugin, according to its mode:
 *
 *   Off: no rate limiting
 *   ArrayRate: limits the number of arrays per second
 *   ByteRate: limits the number of bytes per second
 *             (note: this is calculated from the contents of pArray->pData ONLY)
 */

typedef enum {
    NDRATELIMIT_OFF,
    NDRATELIMIT_ARRAYRATE,
    NDRATELIMIT_BYTERATE,

    NDRATELIMIT_MODE_MAX
}NDRateLimitMode_t;

class epicsShareClass NDPluginRateLimit : public NDPluginDriver {
public:
    NDPluginRateLimit(const char *portName, int queueSize, int blockingCallbacks,
                  const char *NDArrayPort, int NDArrayAddr,
                  int maxBuffers, size_t maxMemory,
                  int priority, int stackSize, int maxThreads);

    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

protected:
    int NDRateLimitMode;
    #define FIRST_NDCODEC_PARAM NDRateLimitMode
    int NDRateLimitLimit;
    int NDRateLimitRefillTime;
    int NDRateLimitRefillAmount;
    int NDRateLimitNumTokens;
    int NDRateLimitDropped;

private:
    epicsTimeStamp lastUpdate_;

    void reset();
    int refill(void);
    bool tryTake(int tokens);
};

#endif
