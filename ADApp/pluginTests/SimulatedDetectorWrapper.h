/*
 * SimulatedDetector.h
 *
 *  Created on: 10 Mar 2015
 *      Author: gnx91527
 */

#ifndef IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_SIMULATEDDETECTOR_H_
#define IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_SIMULATEDDETECTOR_H_

#include <tr1/memory>
#include <map>

#include <asynPortClient.h>
#include <simDetector.h>
#include "AsynPortClientContainer.h"

class SimulatedDetectorWrapper : public simDetector, public AsynPortClientContainer
{
public:
	SimulatedDetectorWrapper(const std::string& port);
	SimulatedDetectorWrapper(const std::string& port,
	                         int maxSizeX,
	                         int maxSizeY);
  SimulatedDetectorWrapper(const std::string& port,
                           int maxSizeX,
                           int maxSizeY,
                           NDDataType_t dataType);
  SimulatedDetectorWrapper(const std::string& port,
                           int maxSizeX,
                           int maxSizeY,
                           NDDataType_t dataType,
                           int maxBuffers,
                           int maxMemory,
                           int priority,
                           int stackSize);
	void acquireSync(int numFrames);

	virtual ~SimulatedDetectorWrapper();

};

#endif /* IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_SIMULATEDDETECTOR_H_ */
