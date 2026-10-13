#include "ColorConvertPluginWrapper.h"

ColorConvertPluginWrapper::ColorConvertPluginWrapper(
    const std::string &port, const std::string &detectorPort)
    : NDPluginColorConvert(port.c_str(), 50, 0, detectorPort.c_str(),
                        0, 0, 0, 0, 0, 0),
      AsynPortClientContainer(port) {}

ColorConvertPluginWrapper::~ColorConvertPluginWrapper() { cleanup(); }
