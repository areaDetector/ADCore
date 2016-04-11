/*
 * TimeSeriesPluginWrapper.cpp
 *
 *  Created on: 21 Mar 2016
 *      Author: Ulrik Pedersen
 */

#include "FFTPluginWrapper.h"

FFTPluginWrapper::FFTPluginWrapper(const std::string& port, const std::string& detectorPort)
  :  NDPluginFFT(port.c_str(), 50, 0, detectorPort.c_str(), 0, 0, 0, 0, 0),
     AsynPortClientContainer(port)
{
}

FFTPluginWrapper::FFTPluginWrapper(const std::string& port,
                                   int queueSize,
                                   int blocking,
                                   const std::string& detectorPort,
                                   int address,
                                   size_t maxMemory,
                                   int priority,
                                   int stackSize)
  :  NDPluginFFT(port.c_str(), queueSize, blocking,
                        detectorPort.c_str(), address,
                        0, maxMemory, priority, stackSize),
     AsynPortClientContainer(port)
{
}

FFTPluginWrapper::~FFTPluginWrapper ()
{
  cleanup();
}

