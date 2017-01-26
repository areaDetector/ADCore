/*
 * AsynPortClientContainer.h
 *
 *  Created on: 10 Mar 2015
 *      Author: gnx91527
 */

#ifndef IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_AsynPortClientContainer_H_
#define IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_AsynPortClientContainer_H_

#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>

#include <epicsThread.h>
#include <asynPortClient.h>
#include "AsynException.h"

#define MAX_PARAMETER_STRING_LENGTH 1024

typedef std::map<std::string, boost::shared_ptr<asynInt32Client> > int32ClientMap;
typedef std::map<std::string, boost::shared_ptr<asynFloat64Client> > float64ClientMap;
typedef std::map<std::string, boost::shared_ptr<asynOctetClient> > octetClientMap;

class AsynPortClientContainer {
public:
	AsynPortClientContainer(const std::string& port);

	virtual void write(const std::string& paramName, int value, int address=0);
	virtual void write(const std::string& paramName, double value, int address=0);
	virtual unsigned long int write(const std::string& paramName, const std::string& value, int address=0);
	virtual int readInt(const std::string& paramName, int address=0);
    virtual double readDouble(const std::string& paramName, int address=0);
    virtual std::string readString(const std::string& paramName, int address=0);

	virtual void cleanup();

	virtual ~AsynPortClientContainer();

protected:
	std::string portName;

private:
    std::vector<boost::shared_ptr<int32ClientMap> > int32Maps;
    std::vector<boost::shared_ptr<float64ClientMap> > float64Maps;
    std::vector<boost::shared_ptr<octetClientMap> > octetMaps;

    boost::shared_ptr<int32ClientMap> getInt32Map(int address);
    boost::shared_ptr<float64ClientMap> getFloat64Map(int address);
    boost::shared_ptr<octetClientMap> getOctetMap(int address);

    template<class T> boost::shared_ptr<T> processMap(std::vector<boost::shared_ptr<T> > Maps, int address)
    {
        boost::shared_ptr<T> mapPtr;
        // Check vector entry
        if ((int)Maps.size() <= address) {
            // Extend vector
            Maps.resize(address + 1);
        }
        if (Maps[address].get() == NULL){
            // Add map to vector
            mapPtr = boost::shared_ptr<T>(new T());
            Maps[address] = mapPtr;
        } else {
            // Get existing map
            mapPtr = Maps[address];
        }
        return mapPtr;
    }
};

#endif /* IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_AsynPortClientContainer_H_ */
