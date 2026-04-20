/*
 * CodecPluginWrapper.cpp
 *
 *  Created on: 20 Apr 2026
 *      Author: Jakub Wlodek
 */

#ifndef ADAPP_PLUGINTESTS_CODECPLUGINWRAPPER_H_
#define ADAPP_PLUGINTESTS_CODECPLUGINWRAPPER_H_

#include <NDPluginCodec.h>
#include "AsynPortClientContainer.h"

class CodecPluginWrapper : public NDPluginCodec, public AsynPortClientContainer
{
public:
  CodecPluginWrapper(const std::string& port, const std::string& detectorPort);
  CodecPluginWrapper(const std::string& port,
                   int queueSize,
                   int blocking,
                   const std::string& detectorPort,
                   int address,
                   size_t maxMemory,
                   int priority,
                   int stackSize,
                   int maxThreads);
  virtual ~CodecPluginWrapper ();
};

#endif /* ADAPP_PLUGINTESTS_CODECPLUGINWRAPPER_H_ */
