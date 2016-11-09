/*
 * ROIPluginWrapper.h
 *
 *  Created on: 9 Nov 2016
 *      Author: Mark Rivers
 */

#ifndef ADAPP_PLUGINTESTS_ROIPLUGINWRAPPER_H_
#define ADAPP_PLUGINTESTS_ROIPLUGINWRAPPER_H_

#include <NDPluginROI.h>
#include "AsynPortClientContainer.h"

class ROIPluginWrapper : public NDPluginROI, public AsynPortClientContainer
{
public:
  ROIPluginWrapper(const std::string& port, const std::string& detectorPort);
  ROIPluginWrapper(const std::string& port,
                   int queueSize,
                   int blocking,
                   const std::string& detectorPort,
                   int address,
                   size_t maxMemory,
                   int priority,
                   int stackSize);
  virtual ~ROIPluginWrapper ();
};

#endif /* ADAPP_PLUGINTESTS_ROIPLUGINWRAPPER_H_ */
