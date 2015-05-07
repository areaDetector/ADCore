/*
 * SimulatedDetector.cpp
 *
 *  Created on: 10 Mar 2015
 *      Author: gnx91527
 */

#include "SimulatedDetectorWrapper.h"

SimulatedDetectorWrapper::SimulatedDetectorWrapper(const std::string& port) : AsynPortTestWrapper(port)
{
	// Create the simulated detector class with default size and type
	init(port, 1024, 1024, NDUInt8, 0, 0, 0, 0);
}

SimulatedDetectorWrapper::SimulatedDetectorWrapper(const std::string& port,
                                                   int maxSizeX,
                                                   int maxSizeY) : AsynPortTestWrapper(port)
{
	// Create the simulated detector class with default type
	init(port, maxSizeX, maxSizeY, NDUInt8, 0, 0, 0, 0);
}

SimulatedDetectorWrapper::SimulatedDetectorWrapper(const std::string& port,
                                                   int maxSizeX,
                                                   int maxSizeY,
                                                   NDDataType_t dataType) : AsynPortTestWrapper(port)
{
	init(port, maxSizeX, maxSizeY, dataType, 0, 0, 0, 0);
}

SimulatedDetectorWrapper::SimulatedDetectorWrapper(const std::string& port,
                                                   int maxSizeX,
                                                   int maxSizeY,
                                                   NDDataType_t dataType,
                                                   int maxBuffers,
                                                   int maxMemory,
                                                   int priority,
                                                   int stackSize) : AsynPortTestWrapper(port)
{
  init(port, maxSizeX, maxSizeY, dataType, maxBuffers, maxMemory, priority, stackSize);
}

void SimulatedDetectorWrapper::init(const std::string& port,
                                    int maxSizeX,
                                    int maxSizeY,
                                    NDDataType_t dataType,
                                    int maxBuffers,
                                    int maxMemory,
                                    int priority,
                                    int stackSize)
{
	// Create the simulated detector class
	detector = std::tr1::shared_ptr<simDetector>(new simDetector(port.c_str(),
	                                                             maxSizeX,
	                                                             maxSizeY,
	                                                             dataType,
	                                                             maxBuffers,
	                                                             maxMemory,
	                                                             priority,
	                                                             stackSize));
}

void SimulatedDetectorWrapper::acquireSync(int numFrames)
{
	int acquiring;
	if (numFrames == 1){
		this->write(ADImageModeString, ADImageSingle);
	} else {
		this->write(ADImageModeString, ADImageMultiple);
		this->write(ADNumImagesString, numFrames);
	}
	this->write(ADAcquireString, 1);
	do {
		this->read(ADAcquireString, &acquiring);
		epicsThreadSleep(0.1);
	} while (acquiring);
}

SimulatedDetectorWrapper::~SimulatedDetectorWrapper()
{
  cleanup();
	detector.reset();
}

