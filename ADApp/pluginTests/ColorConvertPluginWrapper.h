#ifndef ADAPP_PLUGINTESTS_COLORCONVERTPLUGINWRAPPER_H_
#define ADAPP_PLUGINTESTS_COLORCONVERTPLUGINWRAPPER_H_

#include <NDPluginColorConvert.h>
#include "AsynPortClientContainer.h"

class ColorConvertPluginWrapper : public NDPluginColorConvert, public AsynPortClientContainer
{
public:
    ColorConvertPluginWrapper(const std::string& port, const std::string& detectorPort);
    virtual ~ColorConvertPluginWrapper();
};

#endif /* ADAPP_PLUGINTESTS_COLORCONVERTPLUGINWRAPPER_H_ */
