/*
 * AsynPortTestWrapper.h
 *
 *  Created on: 10 Mar 2015
 *      Author: gnx91527
 */

#ifndef IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_ASYNPORTTESTWRAPPER_H_
#define IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_ASYNPORTTESTWRAPPER_H_

#include <tr1/memory>
#include <map>

#include <epicsThread.h>
#include <asynPortClient.h>

class AsynPortTestWrapper {
public:
	AsynPortTestWrapper(const std::string& port);
	virtual void write(const std::string& paramName, int value);
	virtual void write(const std::string& paramName, double value);
	virtual void write(const std::string& paramName, const std::string& value, unsigned long int length, unsigned long int *numWritten);
	virtual void read(const std::string& paramName, int *value);
  virtual void read(const std::string& paramName, double *value);
  virtual void read(const std::string& paramName, char *value, int maxlen, size_t *nRead);
	virtual void cleanup();

	virtual ~AsynPortTestWrapper();

protected:
	std::string portName;

private:
	std::map<std::string, std::tr1::shared_ptr<asynInt32Client> > int32Clients;
	std::map<std::string, std::tr1::shared_ptr<asynFloat64Client> > float64Clients;
	std::map<std::string, std::tr1::shared_ptr<asynOctetClient> > octetClients;
};

#endif /* IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_ASYNPORTTESTWRAPPER_H_ */
