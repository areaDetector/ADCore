/*
 * OverlayPluginWrapper.cpp
 *
 *  Created on: 26 Nov 2016
 *      Author: Mark Rivers
 */

#include "OverlayPluginWrapper.h"

OverlayPluginWrapper::OverlayPluginWrapper(const std::string& port, const std::string& detectorPort, int maxOverlays)
  :  NDPluginOverlay(port.c_str(), 50, 0, detectorPort.c_str(), 0, maxOverlays, 0, 0, 0, 0),
     AsynPortClientContainer(port)
{
}

OverlayPluginWrapper::OverlayPluginWrapper(const std::string& port,
                                   int queueSize,
                                   int blocking,
                                   const std::string& detectorPort,
                                   int address,
                                   int maxOverlays,
                                   size_t maxMemory,
                                   int priority,
                                   int stackSize)
  :  NDPluginOverlay(port.c_str(), queueSize, blocking,
                        detectorPort.c_str(), address, maxOverlays,
                        0, maxMemory, priority, stackSize),
     AsynPortClientContainer(port)
{
}

OverlayPluginWrapper::~OverlayPluginWrapper ()
{
  cleanup();
}

