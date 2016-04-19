/*
 * TimeSeriesPluginWrapper.cpp
 *
 *  Created on: 21 Mar 2016
 *      Author: Ulrik Pedersen
 */

#ifndef ADAPP_PLUGINTESTS_FFTPLUGINWRAPPER_H_
#define ADAPP_PLUGINTESTS_FFTPLUGINWRAPPER_H_

#include <NDPluginFFT.h>
#include "AsynPortClientContainer.h"

class FFTPluginWrapper : public NDPluginFFT, public AsynPortClientContainer
{
public:
  FFTPluginWrapper(const std::string& port, const std::string& detectorPort);
  FFTPluginWrapper(const std::string& port,
                   int queueSize,
                   int blocking,
                   const std::string& detectorPort,
                   int address,
                   size_t maxMemory,
                   int priority,
                   int stackSize);
  virtual ~FFTPluginWrapper ();
};

#endif /* ADAPP_PLUGINTESTS_FFTPLUGINWRAPPER_H_ */
