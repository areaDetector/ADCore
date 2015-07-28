/*
 * AsynPortClientContainer.h
 *
 *  Created on: 10 Mar 2015
 *      Author: gnx91527
 */

#ifndef IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_AsynPortClientContainer_H_
#define IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_AsynPortClientContainer_H_

#include <tr1/memory>
#include <map>

#include <epicsThread.h>
#include <asynPortClient.h>
#include "AsynException.h"

#define MAX_PARAMETER_STRING_LENGTH 1024

class AsynPortClientContainer {
public:
	AsynPortClientContainer(const std::string& port);
	virtual void write(const std::string& paramName, int value);
	virtual void write(const std::string& paramName, double value);
	virtual unsigned long int write(const std::string& paramName, const std::string& value);
	virtual int readInt(const std::string& paramName);
  virtual double readDouble(const std::string& paramName);
  virtual std::string readString(const std::string& paramName);
	virtual void cleanup();

	virtual ~AsynPortClientContainer();

protected:
	std::string portName;

private:
	std::map<std::string, std::tr1::shared_ptr<asynInt32Client> > int32Clients;
	std::map<std::string, std::tr1::shared_ptr<asynFloat64Client> > float64Clients;
	std::map<std::string, std::tr1::shared_ptr<asynOctetClient> > octetClients;
};

#endif /* IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_AsynPortClientContainer_H_ */
