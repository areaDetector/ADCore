/*
 * HDF5TestWrapper.cpp
 *
 *  Created on: 10 Mar 2015
 *      Author: gnx91527
 */

#include "HDF5PluginWrapper.h"

HDF5PluginWrapper::HDF5PluginWrapper(const std::string& port, const std::string& detectorPort) : AsynPortTestWrapper(port)
{
	// Init the HDF5 class
	init(port, 50, 0, detectorPort, 0, 0, 0);
}

HDF5PluginWrapper::HDF5PluginWrapper(const std::string& port,
                                     int queueSize,
                                     int blocking,
                                     const std::string& detectorPort,
                                     int address,
                                     int priority,
                                     int stackSize) : AsynPortTestWrapper(port)
{
  // Init the HDF5 class
  init(port, queueSize, blocking, detectorPort, address, priority, stackSize);
}

void HDF5PluginWrapper::init(const std::string& port,
                             int queueSize,
                             int blocking,
                             const std::string& detectorPort,
                             int address,
                             int priority,
                             int stackSize)
{
	// Create the simulated detector class
	hdf5Writer = std::tr1::shared_ptr<NDFileHDF5>(new NDFileHDF5(port.c_str(),
	                                                             queueSize,
	                                                             blocking,
	                                                             detectorPort.c_str(),
	                                                             address,
	                                                             priority,
	                                                             stackSize));
}

void HDF5PluginWrapper::processCallbacks(NDArray *array)
{
  hdf5Writer->processCallbacks(array);
}

void HDF5PluginWrapper::lock()
{
  hdf5Writer->lock();
}

void HDF5PluginWrapper::unlock()
{
  hdf5Writer->unlock();
}

HDF5PluginWrapper::~HDF5PluginWrapper()
{
  cleanup();
	hdf5Writer.reset();
}

