/*
 * AttrPlotPluginWrapper.cpp
 *
 *  Created on: 28 Feb 2017
 *      Author: Blaz Kranjc
 */

#include "AttrPlotPluginWrapper.h"

AttrPlotPluginWrapper::AttrPlotPluginWrapper(const std::string& port,
        int max_attributes, int cache_size, int max_selected,
        const std::string& in_port, int in_addr)
  :  NDPluginAttrPlot(port.c_str(), max_attributes, cache_size, max_selected,
          in_port.c_str(), in_addr, 1000, 0, 0, 0),
     AsynPortClientContainer(port)
{
}

AttrPlotPluginWrapper::~AttrPlotPluginWrapper ()
{
  cleanup();
}

