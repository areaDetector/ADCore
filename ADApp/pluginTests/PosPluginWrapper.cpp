/*
 * PosPluginWrapper.cpp
 *
 *  Created on: 24 Jun 2015
 *      Author: gnx91527
 */

#include "PosPluginWrapper.h"

PosPluginWrapper::PosPluginWrapper(const std::string& port, const std::string& detectorPort)
  :  NDPosPlugin(port.c_str(), 50, 0, detectorPort.c_str(), 0, -1, -1, 0, 0),
     AsynPortClientContainer(port)
{
}

PosPluginWrapper::PosPluginWrapper(const std::string& port,
                                   int queueSize,
                                   int blocking,
                                   const std::string& detectorPort,
                                   int address,
                                   int maxBuffers,
                                   size_t maxMemory,
                                   int priority,
                                   int stackSize)
  :  NDPosPlugin(port.c_str(), queueSize, blocking, detectorPort.c_str(), address, maxBuffers, maxMemory, priority, stackSize),
     AsynPortClientContainer(port)
{
}

PosPluginWrapper::~PosPluginWrapper ()
{
  cleanup();
}

