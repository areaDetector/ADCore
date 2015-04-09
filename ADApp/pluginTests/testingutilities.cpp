/*
 * testingutilities.cpp
 *
 *  Created on: 8 Apr 2015
 *      Author: Ulrik Kofoed Pedersen, Diamond Light Source
 */
#include <sstream>
#include <NDPluginDriver.h>
#include "testingutilities.h"

/** Append a unique code at the end of the string name
 * To be used to generate unique asyn port names. Currently only
 * implemented with a basic static counter.
 */
void uniqueAsynPortName(std::string& name)
{
  static unsigned long counter = 0;
  std::stringstream ss;

  ss << name << "_" << counter;
  name = ss.str();
  counter++;
}

TestingPlugin::TestingPlugin (const char *portName, int queueSize, int blockingCallbacks,
                              const char *NDArrayPort, int NDArrayAddr,
                              int maxBuffers, size_t maxMemory,
                              int priority, int stackSize)
/* Invoke the base class constructor */
: NDPluginDriver(portName, queueSize, blockingCallbacks,
                 NDArrayPort, NDArrayAddr, 1, 0, maxBuffers, maxMemory,
                 asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                 asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                 0, 1, priority, stackSize)
{
  arrays_ = new std::deque<NDArray *>();

  connectToArrayPort();
}

TestingPlugin::~TestingPlugin()
{
  while(arrays_->front()) {
    arrays_->front()->release();
    arrays_->pop_front();
  }
  delete arrays_;
}

std::deque<NDArray *> *TestingPlugin::arrays()
{
  return arrays_;
}

void TestingPlugin::processCallbacks(NDArray *pArray)
{
  NDPluginDriver::processCallbacks(pArray);
  NDArray *pArrayCpy = this->pNDArrayPool->copy(pArray, NULL, 1);
  if (pArrayCpy) {
    arrays_->push_back(pArrayCpy);
  }
}



