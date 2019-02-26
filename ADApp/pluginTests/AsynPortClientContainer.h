/*
 * AsynPortClientContainer.h
 *
 *  Created on: 10 Mar 2015
 *      Author: gnx91527
 */

#ifndef IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_AsynPortClientContainer_H_
#define IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_AsynPortClientContainer_H_

#include <memory>
#include <map>
#include <vector>

#include <epicsThread.h>
#include <asynPortClient.h>
#include "AsynException.h"

typedef std::map<std::string, std::shared_ptr<asynInt32Client> > int32ClientMap;
typedef std::map<std::string, std::shared_ptr<asynFloat64Client> > float64ClientMap;
typedef std::map<std::string, std::shared_ptr<asynOctetClient> > octetClientMap;

class AsynPortClientContainer {
public:
    static const int max_string_parameter_len = 1024;

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
    std::vector<std::shared_ptr<int32ClientMap> > int32Maps;
    std::vector<std::shared_ptr<float64ClientMap> > float64Maps;
    std::vector<std::shared_ptr<octetClientMap> > octetMaps;

    std::shared_ptr<int32ClientMap> getInt32Map(int address);
    std::shared_ptr<float64ClientMap> getFloat64Map(int address);
    std::shared_ptr<octetClientMap> getOctetMap(int address);

    template<class T> std::shared_ptr<T> processMap(std::vector<std::shared_ptr<T> >& Maps, int address)
    {
        std::shared_ptr<T> mapPtr;
        // Check vector entry
        if ((int)Maps.size() <= address) {
            // Extend vector
            Maps.resize(address + 1);
        }
        if (Maps[address].get() == NULL){
            // Add map to vector
            mapPtr = std::shared_ptr<T>(new T());
            Maps[address] = mapPtr;
        } else {
            // Get existing map
            mapPtr = Maps[address];
        }
        return mapPtr;
    }
};

#endif /* IOCS_SIMDETECTORNOIOC_SIMDETECTORNOIOCAPP_SRC_AsynPortClientContainer_H_ */
