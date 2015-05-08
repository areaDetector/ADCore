/*
 * HDF5TestWrapper.cpp
 *
 *  Created on: 10 Mar 2015
 *      Author: gnx91527
 */

#include "HDF5PluginWrapper.h"

HDF5PluginWrapper::HDF5PluginWrapper(const std::string& port, const std::string& detectorPort)
  :  NDFileHDF5(port.c_str(), 50, 0, detectorPort.c_str(), 0, 0, 0),
     AsynPortClientContainer(port)
{
}

HDF5PluginWrapper::HDF5PluginWrapper(const std::string& port,
                                     int queueSize,
                                     int blocking,
                                     const std::string& detectorPort,
                                     int address,
                                     int priority,
                                     int stackSize)
  : NDFileHDF5(port.c_str(), queueSize, blocking, detectorPort.c_str(), address, priority, stackSize),
    AsynPortClientContainer(port)
{
}

HDF5PluginWrapper::~HDF5PluginWrapper()
{
  cleanup();
}

