/*
 * SimulatedDetector.cpp
 *
 *  Created on: 10 Mar 2015
 *      Author: gnx91527
 */

#include "SimulatedDetectorWrapper.h"

SimulatedDetectorWrapper::SimulatedDetectorWrapper(const std::string& port)
  : simDetector(port.c_str(), 1024, 1024, NDUInt8, 0, 0, 0, 0),
    AsynPortClientContainer(port)
{
}

SimulatedDetectorWrapper::SimulatedDetectorWrapper(const std::string& port,
                                                   int maxSizeX,
                                                   int maxSizeY)
  : simDetector(port.c_str(), maxSizeX, maxSizeY, NDUInt8, 0, 0, 0, 0),
    AsynPortClientContainer(port)
{
}

SimulatedDetectorWrapper::SimulatedDetectorWrapper(const std::string& port,
                                                   int maxSizeX,
                                                   int maxSizeY,
                                                   NDDataType_t dataType)
  : simDetector(port.c_str(), maxSizeX, maxSizeY, dataType, 0, 0, 0, 0),
    AsynPortClientContainer(port)
{
}

SimulatedDetectorWrapper::SimulatedDetectorWrapper(const std::string& port,
                                                   int maxSizeX,
                                                   int maxSizeY,
                                                   NDDataType_t dataType,
                                                   int maxBuffers,
                                                   int maxMemory,
                                                   int priority,
                                                   int stackSize)
  : simDetector(port.c_str(), maxSizeX, maxSizeY, dataType, maxBuffers, maxMemory, priority, stackSize),
    AsynPortClientContainer(port)
{
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
		acquiring = this->readInt(ADAcquireString);
		epicsThreadSleep(0.1);
	} while (acquiring);
}

SimulatedDetectorWrapper::~SimulatedDetectorWrapper()
{
  cleanup();
}

