/*
 * ROIPluginWrapper.cpp
 *
 *  Created on: 9 Nov 2016
 *      Author: Mark Rivers
 */

#include "ROIPluginWrapper.h"

ROIPluginWrapper::ROIPluginWrapper(const std::string& port, const std::string& detectorPort)
  :  NDPluginROI(port.c_str(), 50, 0, detectorPort.c_str(), 0, 0, 0, 0, 0),
     AsynPortClientContainer(port)
{
}

ROIPluginWrapper::ROIPluginWrapper(const std::string& port,
                                   int queueSize,
                                   int blocking,
                                   const std::string& detectorPort,
                                   int address,
                                   size_t maxMemory,
                                   int priority,
                                   int stackSize)
  :  NDPluginROI(port.c_str(), queueSize, blocking,
                        detectorPort.c_str(), address,
                        0, maxMemory, priority, stackSize),
     AsynPortClientContainer(port)
{
}

ROIPluginWrapper::~ROIPluginWrapper ()
{
  cleanup();
}

