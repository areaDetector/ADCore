/*
 * OverlayPluginWrapper.h
 *
 *  Created on: 9 Nov 2016
 *      Author: Mark Rivers
 */

#ifndef ADAPP_PLUGINTESTS_OVERLAYPLUGINWRAPPER_H_
#define ADAPP_PLUGINTESTS_OVERLAYPLUGINWRAPPER_H_

#include <NDPluginOverlay.h>
#include "AsynPortClientContainer.h"

class OverlayPluginWrapper : public NDPluginOverlay, public AsynPortClientContainer
{
public:
  OverlayPluginWrapper(const std::string& port, int addr, const std::string& detectorPort, int maxOverlays);
  OverlayPluginWrapper(const std::string& port,
                   int addr,
                   int queueSize,
                   int blocking,
                   const std::string& detectorPort,
                   int address,
                   int maxOverlays,
                   size_t maxMemory,
                   int priority,
                   int stackSize);
  virtual ~OverlayPluginWrapper ();
};

#endif /* ADAPP_PLUGINTESTS_OVERLAYPLUGINWRAPPER_H_ */
