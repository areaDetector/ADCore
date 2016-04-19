/*
 * TimeSeriesPluginWrapper.cpp
 *
 *  Created on: 21 Mar 2016
 *      Author: Ulrik Pedersen
 */

#ifndef ADAPP_PLUGINTESTS_TIMESERIESPLUGINWRAPPER_H_
#define ADAPP_PLUGINTESTS_TIMESERIESPLUGINWRAPPER_H_

#include <NDPluginTimeSeries.h>
#include "AsynPortClientContainer.h"

class TimeSeriesPluginWrapper : public NDPluginTimeSeries, public AsynPortClientContainer
{
public:
  TimeSeriesPluginWrapper(const std::string& port, const std::string& detectorPort);
  TimeSeriesPluginWrapper(const std::string& port,
                   int queueSize,
                   int blocking,
                   const std::string& detectorPort,
                   int address,
                   int maxSignals,
                   size_t maxMemory,
                   int priority,
                   int stackSize);
  virtual ~TimeSeriesPluginWrapper ();
};

#endif /* ADAPP_PLUGINTESTS_TIMESERIESPLUGINWRAPPER_H_ */
