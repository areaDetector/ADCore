/*
 * plugintest.h
 *
 *  Created on: 8 Apr 2015
 *      Author: Ulrik Kofoed Pedersen, Diamond Light Source
 */

#ifndef ADAPP_PLUGINTESTS_TESTINGUTILITIES_H_
#define ADAPP_PLUGINTESTS_TESTINGUTILITIES_H_

#include <deque>
#include <string>

void uniqueAsynPortName(std::string& name);

// Mock NDPlugin; simply stores all received NDArrays and provides them to a client on request.
class TestingPlugin : public NDPluginDriver {
public:
  TestingPlugin (const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
  ~TestingPlugin();
  void processCallbacks(NDArray *pArray);
  std::deque<NDArray *> *arrays();
private:
  std::deque<NDArray *> *arrays_;
};


#endif /* ADAPP_PLUGINTESTS_TESTINGUTILITIES_H_ */
