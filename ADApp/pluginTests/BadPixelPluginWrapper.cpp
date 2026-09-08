/*
 * BadPixelPluginWrapper.cpp
 *
 *  Created on: 20 Apr 2026
 *      Author: Jakub Wlodek
 */

#include <json.hpp>
using nlohmann::json;

#include "BadPixelPluginWrapper.h"

BadPixelPluginWrapper::BadPixelPluginWrapper(const std::string& port, const std::string& detectorPort)
  :  NDPluginBadPixel(port.c_str(), 50, 0, detectorPort.c_str(), 0, 0, 0, 0, 0, 1),
     AsynPortClientContainer(port)
{
}

BadPixelPluginWrapper::BadPixelPluginWrapper(const std::string& port,
                                   int queueSize,
                                   int blocking,
                                   const std::string& detectorPort,
                                   int address,
                                   size_t maxMemory,
                                   int priority,
                                   int stackSize,
                                   int maxThreads)
  :  NDPluginBadPixel(port.c_str(), queueSize, blocking,
                        detectorPort.c_str(), address,
                        0, maxMemory, priority, stackSize, maxThreads),
     AsynPortClientContainer(port)
{
}

badPixelList_t BadPixelPluginWrapper::testParseBadPixelList(const std::string& jsonStr)
{
  json j = json::parse(jsonStr);
  return parseBadPixelList(j);
}

BadPixelPluginWrapper::~BadPixelPluginWrapper ()
{
  cleanup();
}

