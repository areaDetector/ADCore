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

void AsynPortClientContainer::write(const std::string& paramName, int value)
{
	// Check for the client
	if (int32Clients.count(paramName) == 0){
		// We need to create the client as it isn't stored
		int32Clients[paramName] = std::tr1::shared_ptr<asynInt32Client>(new asynInt32Client(portName.c_str(), 0, paramName.c_str()));
	}
	int32Clients[paramName]->write(value);
}

void AsynPortClientContainer::write(const std::string& paramName, double value)
{
	// Check for the client
	if (float64Clients.count(paramName) == 0){
		// We need to create the client as it isn't stored
		float64Clients[paramName] = std::tr1::shared_ptr<asynFloat64Client>(new asynFloat64Client(portName.c_str(), 0, paramName.c_str()));
	}
	float64Clients[paramName]->write(value);
}

void AsynPortClientContainer::write(const std::string& paramName, const std::string& value, unsigned long int length, unsigned long int *numWritten)
{
	// Check for the client
	if (octetClients.count(paramName) == 0){
		// We need to create the client as it isn't stored
		octetClients[paramName] = std::tr1::shared_ptr<asynOctetClient>(new asynOctetClient(portName.c_str(), 0, paramName.c_str()));
	}
	octetClients[paramName]->write(value.c_str(), length, numWritten);
}

int AsynPortClientContainer::readInt(const std::string& paramName)
{
  int value = 0;
  // Check for the client
  if (int32Clients.count(paramName) == 0){
    // We need to create the client as it isn't stored
    int32Clients[paramName] = std::tr1::shared_ptr<asynInt32Client>(new asynInt32Client(portName.c_str(), 0, paramName.c_str()));
  }
  int32Clients[paramName]->read(&value);
  return value;
}

double AsynPortClientContainer::readDouble(const std::string& paramName)
{
  double value = 0.0;
  // Check for the client
  if (float64Clients.count(paramName) == 0){
    // We need to create the client as it isn't stored
    float64Clients[paramName] = std::tr1::shared_ptr<asynFloat64Client>(new asynFloat64Client(portName.c_str(), 0, paramName.c_str()));
  }
  float64Clients[paramName]->read(&value);
  return value;
}

std::string AsynPortClientContainer::readString(const std::string& paramName)
{
  char value[MAX_PARAMETER_STRING_LENGTH];
  int maxlen = MAX_PARAMETER_STRING_LENGTH;
  size_t nRead = 0;
  int reason = 0;
  // Check for the client
  if (octetClients.count(paramName) == 0){
    // We need to create the client as it isn't stored
    octetClients[paramName] = std::tr1::shared_ptr<asynOctetClient>(new asynOctetClient(portName.c_str(), 0, paramName.c_str()));
  }
  octetClients[paramName]->read(value, maxlen, &nRead, &reason);
  std::string svalue(value);
  return svalue;
}

void AsynPortClientContainer::cleanup()
{
  // Empty all of the maps
  int32Clients.clear();
  float64Clients.clear();
  octetClients.clear();
}

AsynPortClientContainer::~AsynPortClientContainer()
{
}

