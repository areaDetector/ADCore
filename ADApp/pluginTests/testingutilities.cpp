/*
 * testingutilities.cpp
 *
 *  Created on: 8 Apr 2015
 *      Author: Ulrik Kofoed Pedersen, Diamond Light Source
 */
#include <sstream>
#include <iostream>
#include <NDPluginDriver.h>
#include "testingutilities.h"


void fillNDArrays(const std::vector<size_t>& dimensions,
                        NDDataType_t dataType,
                        std::vector<NDArray*>& arrays)
{
  NDArrayInfo_t arrinfo;
  std::vector<NDArray*>::iterator ait;
  unsigned int array_num = 0;
  for (ait=arrays.begin(); ait != arrays.end(); ++ait)
  {
    *ait = new NDArray();
    NDArray* parr = *ait;
    parr->dataType = dataType;
    parr->ndims = dimensions.size();
    parr->pNDArrayPool = NULL;
    parr->getInfo(&arrinfo);
    parr->dataSize = arrinfo.bytesPerElement;
    unsigned int i=0;
    for (std::vector<size_t>::const_iterator it = dimensions.begin(); it != dimensions.end(); ++it)
    {
      parr->dataSize *= *it;
      parr->dims[i].size = *it;
      i++;
    }

    parr->pData = calloc(parr->dataSize, sizeof(char));
    memset(parr->pData, array_num, parr->dataSize);

    parr->uniqueId = array_num;
  }
}


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

void TestingPluginCallback(void *drvPvt, asynUser *pasynUser, void *ptr)
{
  TestingPlugin* self = (TestingPlugin*)drvPvt;
  self->callback((NDArray*)ptr);
}

TestingPlugin::TestingPlugin (const char *portName, int addr)
/* Invoke the base class constructor */
: asynGenericPointerClient(portName, addr, NDArrayDataString)
{
  this->registerInterruptUser(TestingPluginCallback);
}

TestingPlugin::~TestingPlugin()
{
  while(arrays.front()) {
    arrays.front()->release();
    arrays.pop_front();
  }
}

void TestingPlugin::callback(NDArray *pArray)
{
  arrays.push_back(pArray);
}



