/*
 * AsynPortClientContainer.cpp
 *
 *  Created on: 10 Mar 2015
 *      Author: gnx91527
 */

#include "AsynPortClientContainer.h"

AsynPortClientContainer::AsynPortClientContainer(const std::string& port)
{
	portName = port;
}

boost::shared_ptr<int32ClientMap> AsynPortClientContainer::getInt32Map(int address)
{
    return processMap<int32ClientMap>(int32Maps, address);
}

boost::shared_ptr<float64ClientMap> AsynPortClientContainer::getFloat64Map(int address)
{
    return processMap<float64ClientMap>(float64Maps, address);
}

boost::shared_ptr<octetClientMap> AsynPortClientContainer::getOctetMap(int address)
{
    return processMap<octetClientMap>(octetMaps, address);
}

void AsynPortClientContainer::write(const std::string& paramName, int value, int address)
{
    boost::shared_ptr<int32ClientMap> mapPtr = getInt32Map(address);
	// Check for the client
	if (mapPtr->count(paramName) == 0){
		// We need to create the client as it isn't stored
        (*mapPtr)[paramName] = boost::shared_ptr<asynInt32Client>(new asynInt32Client(portName.c_str(), address, paramName.c_str()));
	}
	if ((*mapPtr)[paramName]->write(value) != asynSuccess){
	  throw AsynException();
	}
}

void AsynPortClientContainer::write(const std::string& paramName, double value, int address)
{
    boost::shared_ptr<float64ClientMap> mapPtr = getFloat64Map(address);
	// Check for the client
    if (mapPtr->count(paramName) == 0){
		// We need to create the client as it isn't stored
        (*mapPtr)[paramName] = boost::shared_ptr<asynFloat64Client>(new asynFloat64Client(portName.c_str(), address, paramName.c_str()));
	}
	if ((*mapPtr)[paramName]->write(value) != asynSuccess){
	  throw AsynException();
	}
}

unsigned long int AsynPortClientContainer::write(const std::string& paramName, const std::string& value, int address)
{
    boost::shared_ptr<octetClientMap> mapPtr = getOctetMap(address);
  size_t length = 0;
  size_t numWritten = 0;
	// Check for the client
	if (mapPtr->count(paramName) == 0){
		// We need to create the client as it isn't stored
        (*mapPtr)[paramName] = boost::shared_ptr<asynOctetClient>(new asynOctetClient(portName.c_str(), address, paramName.c_str()));
	}
	length = value.size();
	if ((*mapPtr)[paramName]->write(value.c_str(), length, &numWritten) != asynSuccess){
    throw AsynException();
	}
	return numWritten;
}

int AsynPortClientContainer::readInt(const std::string& paramName, int address)
{
  int value = 0;
  boost::shared_ptr<int32ClientMap> mapPtr = getInt32Map(address);
  // Check for the client
  if (mapPtr->count(paramName) == 0){
    // We need to create the client as it isn't stored
      (*mapPtr)[paramName] = boost::shared_ptr<asynInt32Client>(new asynInt32Client(portName.c_str(), address, paramName.c_str()));
  }
  if ((*mapPtr)[paramName]->read(&value) != asynSuccess){
    throw AsynException();
  }
  return value;
}

double AsynPortClientContainer::readDouble(const std::string& paramName, int address)
{
  double value = 0.0;
  boost::shared_ptr<float64ClientMap> mapPtr = getFloat64Map(address);
  // Check for the client
  if (mapPtr->count(paramName) == 0){
    // We need to create the client as it isn't stored
      (*mapPtr)[paramName] = boost::shared_ptr<asynFloat64Client>(new asynFloat64Client(portName.c_str(), address, paramName.c_str()));
  }
  if ((*mapPtr)[paramName]->read(&value) != asynSuccess){
    throw AsynException();
  }
  return value;
}

std::string AsynPortClientContainer::readString(const std::string& paramName, int address)
{
  char value[MAX_PARAMETER_STRING_LENGTH];
  int maxlen = MAX_PARAMETER_STRING_LENGTH;
  size_t nRead = 0;
  int reason = 0;
  boost::shared_ptr<octetClientMap> mapPtr = getOctetMap(address);
  // Check for the client
  if (mapPtr->count(paramName) == 0){
    // We need to create the client as it isn't stored
      (*mapPtr)[paramName] = boost::shared_ptr<asynOctetClient>(new asynOctetClient(portName.c_str(), address, paramName.c_str()));
  }
  if ((*mapPtr)[paramName]->read(value, maxlen, &nRead, &reason) != asynSuccess){
    throw AsynException();
  }
  std::string svalue(value);
  return svalue;
}

void AsynPortClientContainer::cleanup()
{
  // Empty all of the maps
  int32Maps.clear();
  float64Maps.clear();
  octetMaps.clear();
}

AsynPortClientContainer::~AsynPortClientContainer()
{
}

