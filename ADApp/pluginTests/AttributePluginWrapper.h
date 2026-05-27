/*
 * AttributePluginWrapper.h
 *
 *  Created on: 27 Apr 2026
 *      Author: Jakub Wlodek
 */

#ifndef ADAPP_PLUGINTESTS_ATTRIBUTEPLUGINWRAPPER_H_
#define ADAPP_PLUGINTESTS_ATTRIBUTEPLUGINWRAPPER_H_

#include <NDPluginAttribute.h>
#include "AsynPortClientContainer.h"

class AttributePluginWrapper : public NDPluginAttribute, public AsynPortClientContainer
{
public:
  AttributePluginWrapper(const std::string& port, const std::string& detectorPort,
                         int maxAttributes);
  virtual ~AttributePluginWrapper();
};

#endif /* ADAPP_PLUGINTESTS_ATTRIBUTEPLUGINWRAPPER_H_ */
