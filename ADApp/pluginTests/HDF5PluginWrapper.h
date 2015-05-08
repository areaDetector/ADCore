/*
 * HDF5TestWrapper.h
 *
 *  Created on: 10 Mar 2015
 *      Author: gnx91527
 */

#ifndef IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_HDF5TESTWRAPPER_H_
#define IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_HDF5TESTWRAPPER_H_

#include <NDFileHDF5.h>
#include "AsynPortClientContainer.h"

class HDF5PluginWrapper : public NDFileHDF5, public AsynPortClientContainer
{
public:
  HDF5PluginWrapper(const std::string& port, const std::string& detectorPort);
  HDF5PluginWrapper(const std::string& port,
                    int queueSize,
                    int blocking,
                    const std::string& detectorPort,
                    int address,
                    int priority,
                    int stackSize);
	virtual ~HDF5PluginWrapper();

};

#endif /* IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_HDF5TESTWRAPPER_H_ */
