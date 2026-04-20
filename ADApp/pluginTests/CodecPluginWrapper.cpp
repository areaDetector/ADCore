/*
 * CodecPluginWrapper.cpp
 *
 *  Created on: 20 Apr 2026
 *      Author: Jakub Wlodek
 */

#include "CodecPluginWrapper.h"

CodecPluginWrapper::CodecPluginWrapper(const std::string& port, const std::string& detectorPort)
  :  NDPluginCodec(port.c_str(), 50, 0, detectorPort.c_str(), 0, 0, 0, 0, 0, 1),
     AsynPortClientContainer(port)
{
}

CodecPluginWrapper::CodecPluginWrapper(const std::string& port,
                                   int queueSize,
                                   int blocking,
                                   const std::string& detectorPort,
                                   int address,
                                   size_t maxMemory,
                                   int priority,
                                   int stackSize,
                                   int maxThreads)
  :  NDPluginCodec(port.c_str(), queueSize, blocking,
                        detectorPort.c_str(), address,
                        0, maxMemory, priority, stackSize, maxThreads),
     AsynPortClientContainer(port)
{
}

CodecPluginWrapper::~CodecPluginWrapper ()
{
  cleanup();
}

