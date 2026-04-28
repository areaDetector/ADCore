/*
 * AttributePluginWrapper.cpp
 *
 *  Created on: 27 Apr 2026
 *      Author: Jakub Wlodek
 */

#include "AttributePluginWrapper.h"

AttributePluginWrapper::AttributePluginWrapper(const std::string& port,
                                               const std::string& detectorPort,
                                               int maxAttributes)
  :  NDPluginAttribute(port.c_str(), 50, 0, detectorPort.c_str(), 0,
                       maxAttributes, 0, 0, 0, 0),
     AsynPortClientContainer(port)
{
}

AttributePluginWrapper::~AttributePluginWrapper()
{
  cleanup();
}
