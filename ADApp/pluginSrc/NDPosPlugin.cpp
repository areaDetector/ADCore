/*
 * NDPosPlugin.cpp
 *
 *  Created on: 19 May 2015
 *      Author: gnx91527
 */

#include "NDPosPlugin.h"
#include "NDPosPluginFileReader.h"

#include <epicsExport.h>
#include <string.h>
#include <sstream>
#include <iocsh.h>
#include <sys/stat.h>

static const char *driverName = "NDPosPlugin";

void NDPosPlugin::processCallbacks(NDArray *pArray)
{
  int index = 0;
  int running = 0;
  int mode = 0;
  //static const char *functionName = "NDPosPlugin::processCallbacks";

  // Call the base class method
  NDPluginDriver::processCallbacks(pArray);
  getIntegerParam(NDPos_Running, &running);
  // Only attach the position data to the array if we are running
  if (running == 1){
    getIntegerParam(NDPos_CurrentIndex, &index);
    if (index >= (int)positionArray.size()){
      // We've reached the end of our positions, stop to make sure we don't oveflow
      setIntegerParam(NDPos_Running, 0);
    } else {
      std::map<std::string, int> pos = positionArray[index];
      std::stringstream sspos;
      sspos << "[";
      bool firstTime = true;
      std::map<std::string, int>::iterator iter;
      for (iter = pos.begin(); iter != pos.end(); iter++){
        if (firstTime){
          firstTime = false;
        } else {
          sspos << ",";
        }
        sspos << iter->first << "=" << iter->second;
        NDAttribute *pAtt = new NDAttribute(iter->first.c_str(), "Position of NDArray", NDAttrSourceDriver, driverName, NDAttrInt32, &(iter->second));
        pArray->pAttributeList->add(pAtt);
      }
      sspos << "]";
      setStringParam(NDPos_CurrentPos, sspos.str().c_str());

      // Check the mode
      getIntegerParam(NDPos_Mode, &mode);
      if (mode == MODE_DISCARD){
        // The index will stay the same, and we need to pop the value out of the position array
        positionArray.erase(positionArray.begin());
        setIntegerParam(NDPos_CurrentQty, positionArray.size());
      } else if (mode == MODE_KEEP){
        index++;
        setIntegerParam(NDPos_CurrentIndex, index);
      }
    }
    callParamCallbacks();
  }
  this->unlock();
  doCallbacksGenericPointer(pArray, NDArrayData, 0);
  this->lock();
}

asynStatus NDPosPlugin::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  int function = pasynUser->reason;
  int oldvalue;
  int addr;
  asynStatus status = asynSuccess;
  static const char *functionName = "NDPosPlugin::writeInt32";

  // Set the parameter in the parameter library.
  status = getAddress(pasynUser, &addr);
  if (status == asynSuccess){
    getIntegerParam(function, &oldvalue);

    // By default we set the value in the parameter library. If problems occur we set the old value back.
    setIntegerParam(function, value);

    if (function == NDPos_Load){
      // Call the loadFile function
      status = loadFile();
      if (status == asynError){
        // If a bad value is set then revert it to the original
        setIntegerParam(function, oldvalue);
      }
    } else if (function == NDPos_Mode){
      // Reset the position index to 0 if the mode is changed
      setIntegerParam(NDPos_CurrentIndex, 0);
    } else if (function == NDPos_Restart){
      // Reset the position index to 0
      setIntegerParam(NDPos_CurrentIndex, 0);
      // Reset the last sent position
      setStringParam(NDPos_CurrentPos, "");
    } else if (function == NDPos_Delete){
      // Reset the position index to 0
      setIntegerParam(NDPos_CurrentIndex, 0);
      // Reset the last sent position
      setStringParam(NDPos_CurrentPos, "");
      // Clear out the position array
      positionArray.clear();
      setIntegerParam(NDPos_CurrentQty, positionArray.size());
    } else {
      // If this parameter belongs to a base class call its method
      if (function < FIRST_NDPOS_PARAM){
        status = NDPluginDriver::writeInt32(pasynUser, value);
      }
    }
  }

  // Do callbacks so higher layers see any changes
  status = (asynStatus)callParamCallbacks();

  if (status){
    epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                  "%s: status=%d, function=%d, value=%d",
                  functionName, status, function, value);
  } else {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s: function=%d, value=%d\n",
              functionName, function, value);
  }

  return status;
}

asynStatus NDPosPlugin::writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual)
{
  int addr=0;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  char *fileName = new char[MAX_POS_STRING_LEN];
  fileName[MAX_POS_STRING_LEN - 1] = '\0';
  const char *functionName = "writeOctet";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return(status);
  // Set the parameter in the parameter library.
  status = (asynStatus)setStringParam(addr, function, (char *)value);
  if (status != asynSuccess) return(status);

  if (function == NDPos_Filename){
    // Read the filename parameter
    getStringParam(NDPos_Filename, MAX_POS_STRING_LEN-1, fileName);
    // Now validate the XML
    NDPosPluginFileReader fr;
    if (fr.validateXML(fileName) == asynSuccess){
      setIntegerParam(NDPos_FileValid, 1);
    } else {
      setIntegerParam(NDPos_FileValid, 0);
      status = asynError;
    }
  }

  // Do callbacks so higher layers see any changes
  callParamCallbacks(addr, addr);

  if (status){
    epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                  "%s:%s: status=%d, function=%d, value=%s",
                  driverName, functionName, status, function, value);
  } else {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, value=%s\n",
              driverName, functionName, function, value);
  }
  *nActual = nChars;
  return status;
}

asynStatus NDPosPlugin::loadFile()
{
  asynStatus status = asynSuccess;
  char *fileName = new char[MAX_POS_STRING_LEN];
  fileName[MAX_POS_STRING_LEN - 1] = '\0';
  int fileValid = 1;

  // Read the current filename and validity
  getStringParam(NDPos_Filename, MAX_POS_STRING_LEN, fileName);
  getIntegerParam(NDPos_FileValid, &fileValid);

  // If the file is valid then read in the file
  if (fileValid == 1){
    NDPosPluginFileReader fr;
    fr.loadXML(fileName);
    std::vector<std::map<std::string, int> > positions = fr.readPositions();
    positionArray.insert(positionArray.end(), positions.begin(), positions.end());
    setIntegerParam(NDPos_CurrentQty, positionArray.size());
    callParamCallbacks();
  } else {
    status = asynError;
  }

  return status;
}

NDPosPlugin::NDPosPlugin(const char *portName,
                         int queueSize,
                         int blockingCallbacks,
                         const char *NDArrayPort,
                         int NDArrayAddr,
                         size_t maxMemory,
                         int priority,
                         int stackSize)
  : NDPluginDriver(portName,
                   queueSize,
                   blockingCallbacks,
                   NDArrayPort,
                   NDArrayAddr,
                   1,
                   NUM_NDPOS_PARAMS,
                   2,
                   maxMemory,
                   asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE,
                   1,
                   priority,
                   stackSize)
{
  //static const char *functionName = "NDPluginAttribute::NDPluginAttribute";

  // Create parameters for controlling the plugin
  createParam(str_NDPos_Filename,       asynParamOctet,        &NDPos_Filename);
  createParam(str_NDPos_FileValid,      asynParamInt32,        &NDPos_FileValid);
  createParam(str_NDPos_Load,           asynParamInt32,        &NDPos_Load);
  createParam(str_NDPos_Clear,          asynParamInt32,        &NDPos_Clear);
  createParam(str_NDPos_Running,        asynParamInt32,        &NDPos_Running);
  createParam(str_NDPos_Restart,        asynParamInt32,        &NDPos_Restart);
  createParam(str_NDPos_Delete,         asynParamInt32,        &NDPos_Delete);
  createParam(str_NDPos_Mode,           asynParamInt32,        &NDPos_Mode);
  createParam(str_NDPos_Append,         asynParamInt32,        &NDPos_Append);
  createParam(str_NDPos_CurrentQty,     asynParamInt32,        &NDPos_CurrentQty);
  createParam(str_NDPos_CurrentIndex,   asynParamInt32,        &NDPos_CurrentIndex);
  createParam(str_NDPos_CurrentPos,     asynParamOctet,        &NDPos_CurrentPos);

  // Set the plugin type string
  setStringParam(NDPluginDriverPluginType, "NDPositionPlugin");
  // Set the mode to 0 - Discard after use
  setIntegerParam(NDPos_Mode,              MODE_DISCARD);
  // Set the file valid to 0
  setIntegerParam(NDPos_FileValid,         0);
  // Set the position index to 0
  setIntegerParam(NDPos_CurrentIndex,      0);
  // Set the qty to 0
  setIntegerParam(NDPos_CurrentQty,        0);
  // Set the position string to empty
  setStringParam(NDPos_CurrentPos,         "");
  // Set running to 0
  setIntegerParam(NDPos_Running,           0);


  // Try to connect to the array port
  connectToArrayPort();

}

NDPosPlugin::~NDPosPlugin ()
{
}

// Configuration command
extern "C" int NDPosPluginConfigure(const char *portName,
                                    int queueSize,
                                    int blockingCallbacks,
                                    const char *NDArrayPort,
                                    int NDArrayAddr,
                                    size_t maxMemory,
                                    int priority,
                                    int stackSize)
{
  new NDPosPlugin(portName,
                  queueSize,
                  blockingCallbacks,
                  NDArrayPort,
                  NDArrayAddr,
                  maxMemory,
                  priority,
                  stackSize);
  return(asynSuccess);
}

// EPICS iocsh shell commands
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxMemory",iocshArgInt};
static const iocshArg initArg6 = { "priority",iocshArgInt};
static const iocshArg initArg7 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7};
static const iocshFuncDef initFuncDef = {"NDPosPluginConfigure",8,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
  NDPosPluginConfigure(args[0].sval,
                       args[1].ival,
                       args[2].ival,
                       args[3].sval,
                       args[4].ival,
                       args[5].ival,
                       args[6].ival,
                       args[7].ival);
}

extern "C" void NDPosPluginRegister(void)
{
  iocshRegister(&initFuncDef,initCallFunc);
}

extern "C"
{
  epicsExportRegistrar(NDPosPluginRegister);
}
