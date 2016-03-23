/*
 * TimeSeriesPluginWrapper.cpp
 *
 *  Created on: 21 Mar 2016
 *      Author: Ulrik Pedersen
 */

#include "TimeSeriesPluginWrapper.h"

TimeSeriesPluginWrapper::TimeSeriesPluginWrapper(const std::string& port, const std::string& detectorPort)
  :  NDPluginTimeSeries(port.c_str(), 50, 0, detectorPort.c_str(), 0, 1, 0, 0, 0, 0),
     AsynPortClientContainer(port)
{
}

TimeSeriesPluginWrapper::TimeSeriesPluginWrapper(const std::string& port,
                                   int queueSize,
                                   int blocking,
                                   const std::string& detectorPort,
                                   int address,
                                   int maxSignals,
                                   size_t maxMemory,
                                   int priority,
                                   int stackSize)
  :  NDPluginTimeSeries(port.c_str(), queueSize, blocking,
                        detectorPort.c_str(), address, maxSignals,
                        0, maxMemory, priority, stackSize),
     AsynPortClientContainer(port)
{
}

TimeSeriesPluginWrapper::~TimeSeriesPluginWrapper ()
{
  cleanup();
}

