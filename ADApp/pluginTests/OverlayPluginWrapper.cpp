/*
 * OverlayPluginWrapper.cpp
 *
 *  Created on: 9 Nov 2016
 *      Author: Mark Rivers
 */

#include "OverlayPluginWrapper.h"

OverlayPluginWrapper::OverlayPluginWrapper(const std::string& port, const std::string& detectorPort, int maxOverlays)
  :  NDPluginOverlay(port.c_str(), int addr, 50, 0, detectorPort.c_str(), 0, maxOverlays, 0, 0, 0, 0),
     AsynPortClientContainer(port, addr)
{
}

OverlayPluginWrapper::OverlayPluginWrapper(const std::string& port,
                                   int addr,
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
     AsynPortClientContainer(port, addr)
{
}

OverlayPluginWrapper::~OverlayPluginWrapper ()
{
  cleanup();
}

