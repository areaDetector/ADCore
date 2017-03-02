/*
 * AttrPlotPluginWrapper.cpp
 *
 *  Created on: 28 Feb 2017
 *      Author: Blaz Kranjc
 */

#ifndef ADAPP_PLUGINTESTS_ATTRPLOTPLUGINWRAPPER_H_
#define ADAPP_PLUGINTESTS_ATTRPLOTPLUGINWRAPPER_H_

#include <NDPluginAttrPlot.h>
#include "AsynPortClientContainer.h"

class AttrPlotPluginWrapper : public NDPluginAttrPlot, public AsynPortClientContainer
{
public:
  AttrPlotPluginWrapper(const std::string& port,
        int max_attributes, int cache_size, int max_selected,
        const std::string& in_port, int in_addr);
  virtual ~AttrPlotPluginWrapper();
};

#endif /* ADAPP_PLUGINTESTS_ATTRPLOTPLUGINWRAPPER_H_ */
