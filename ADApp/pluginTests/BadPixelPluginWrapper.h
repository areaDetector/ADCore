/*
 * BadPixelPluginWrapper.h
 *
 *  Created on: 06 Jul 2026
 *      Author: Jakub Wlodek
 */

#ifndef ADAPP_PLUGINTESTS_BADPIXELPLUGINWRAPPER_H_
#define ADAPP_PLUGINTESTS_BADPIXELPLUGINWRAPPER_H_

#include <NDPluginBadPixel.h>
#include "AsynPortClientContainer.h"

class BadPixelPluginWrapper : public NDPluginBadPixel, public AsynPortClientContainer
{
public:
  BadPixelPluginWrapper(const std::string& port, const std::string& detectorPort);
  BadPixelPluginWrapper(const std::string& port,
                   int queueSize,
                   int blocking,
                   const std::string& detectorPort,
                   int address,
                   size_t maxMemory,
                   int priority,
                   int stackSize,
                   int maxThreads);
  virtual ~BadPixelPluginWrapper ();

  // Expose private members for testing
  badPixelList_t& getBadPixelList() { return badPixelList; }
  badPixelList_t testParseBadPixelList(const std::string& jsonStr);
  asynStatus testHandleBadPixelFileUpdate(const char* fileName) { return handleBadPixelFileUpdate(fileName); }
};

#endif /* ADAPP_PLUGINTESTS_BADPIXELPLUGINWRAPPER_H_ */
