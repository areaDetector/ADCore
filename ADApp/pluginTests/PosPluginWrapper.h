/*
 * PosPluginWrapper.h
 *
 *  Created on: 24 Jun 2015
 *      Author: gnx91527
 */

#ifndef ADAPP_PLUGINTESTS_POSPLUGINWRAPPER_H_
#define ADAPP_PLUGINTESTS_POSPLUGINWRAPPER_H_

#include <NDPosPlugin.h>
#include "AsynPortClientContainer.h"

class PosPluginWrapper : public NDPosPlugin, public AsynPortClientContainer
{
public:
  PosPluginWrapper(const std::string& port, const std::string& detectorPort);
  PosPluginWrapper(const std::string& port,
                   int queueSize,
                   int blocking,
                   const std::string& detectorPort,
                   int address,
                   int maxBuffers,
                   size_t maxMemory,
                   int priority,
                   int stackSize);
  virtual ~PosPluginWrapper ();
};

#endif /* ADAPP_PLUGINTESTS_POSPLUGINWRAPPER_H_ */
