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

#include <AsynPortTestWrapper.h>
#include <asynPortClient.h>
#include <simDetector.h>

class SimulatedDetectorWrapper : public AsynPortTestWrapper
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
	void init(const std::string& port,
	          int maxSizeX,
	          int maxSizeY,
	          NDDataType_t dataType,
	          int maxBuffers,
	          int maxMemory,
	          int priority,
	          int stackSize);
	void acquireSync(int numFrames);

	virtual ~SimulatedDetectorWrapper();

private:
	std::tr1::shared_ptr<simDetector> detector;
};

#endif /* IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_SIMULATEDDETECTOR_H_ */
