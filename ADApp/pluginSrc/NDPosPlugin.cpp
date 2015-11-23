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

/** Callback function that is called by the NDArray driver with new NDArray data.
  * If the plugin is running then it attaches position data to the NDArray as NDAttributes
  * and then passes the array on.  If the plugin is not running then NDArrays are not
  * passed through to the next plugin(s) in the chain.
  * \param[in] pArray  The NDArray from the callback.
  */ 
void NDPosPlugin::processCallbacks(NDArray *pArray)
{
  int index = 0;
  int running = NDPOS_IDLE;
  int skip = 0;
  int mode = 0;
  int duplicates = 0;
  int dropped = 0;
  int expectedID = 0;
  int IDDifference = 0;
  epicsInt32 IDValue = 0;
  char IDName[MAX_STRING_SIZE];
  static const char *functionName = "NDPosPlugin::processCallbacks";

  // Call the base class method
  NDPluginDriver::processCallbacks(pArray);
  getIntegerParam(NDPos_Running, &running);
  // Only attach the position data to the array if we are running
  if (running == NDPOS_RUNNING){
    getIntegerParam(NDPos_CurrentIndex, &index);
    if (index >= (int)positionArray.size()){
      // We've reached the end of our positions, stop to make sure we don't overflow
      setIntegerParam(NDPos_Running, NDPOS_IDLE);
      running = NDPOS_IDLE;
    } else {
      // Read the ID parameter from the NDArray.  If it cannot be found then abort
      getStringParam(NDPos_IDName, MAX_STRING_SIZE, IDName);
      // Check for IDName, if it is empty the we use the unique ID of the array
      if (strcmp(IDName, "") == 0){
        IDValue = pArray->uniqueId;
      } else {
        NDAttribute *IDAtt = pArray->pAttributeList->find(IDName);
        if (IDAtt){
          epicsInt32 IDValue;
          if (IDAtt->getValue(NDAttrInt32, &IDValue, sizeof(epicsInt32)) == ND_ERROR){
            // Error, unable to get the value from the ID attribute
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                      "%s::%s ERROR: could not retrieve expected ID from attribute [%s]\n",
                      driverName, functionName, IDName);
            setIntegerParam(NDPos_Running, NDPOS_IDLE);
            running = NDPOS_IDLE;
          }
        } else {
          // Error, unable to find the named ID attribute
          asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s::%s ERROR: could not find attribute [%s]\n",
                    driverName, functionName, IDName);
          setIntegerParam(NDPos_Running, NDPOS_IDLE);
          running = NDPOS_IDLE;
        }
      }
      if (running == NDPOS_RUNNING){
        // Check the ID is the same as the expected index
        getIntegerParam(NDPos_IDDifference, &IDDifference);
        getIntegerParam(NDPos_ExpectedID, &expectedID);
        if (expectedID < IDValue){
          asynPrint(this->pasynUserSelf, ASYN_TRACE_WARNING,
                    "%s::%s WARNING: possible frame drop detected: expected ID [%d] received ID [%d]\n",
                    driverName, functionName, expectedID, IDValue);
          // If expected is less than ID throw away positions and record dropped events
          getIntegerParam(NDPos_MissingFrames, &dropped);
          getIntegerParam(NDPos_Mode, &mode);
          if (mode == MODE_DISCARD){
            while ((expectedID < IDValue) && (positionArray.size() > 0)){
              // The index will stay the same, and we need to pop the value out of the position array
              positionArray.erase(positionArray.begin());
              expectedID += IDDifference;
              dropped++;
            }
            // If the size has dropped to zero then we've run out of positions, abort
            if (positionArray.size() == 0){
              setIntegerParam(NDPos_Running, NDPOS_IDLE);
              running = NDPOS_IDLE;
            }
            setIntegerParam(NDPos_CurrentQty, positionArray.size());
          } else if (mode == MODE_KEEP){
            while (expectedID < IDValue && (index < (int)positionArray.size())){
              index++;
              expectedID += IDDifference;
              dropped++;
            }
            // If the index has reached the size of the array then we've run out of positions, abort
            if (index == (int)positionArray.size()){
              setIntegerParam(NDPos_Running, NDPOS_IDLE);
              running = NDPOS_IDLE;
            }
            setIntegerParam(NDPos_CurrentIndex, index);
          }
          setIntegerParam(NDPos_ExpectedID, expectedID);
          setIntegerParam(NDPos_MissingFrames, dropped);
        } else if (expectedID > IDValue){
          asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s::%s ERROR: dropping frame! possible duplicate detected: expected ID [%d] received ID [%d]\n",
                    driverName, functionName, expectedID, IDValue);
          // If expected is greater than ID then ignore the frame and record duplicate event
          getIntegerParam(NDPos_DuplicateFrames, &duplicates);
          duplicates++;
          setIntegerParam(NDPos_DuplicateFrames, duplicates);
          skip = 1;
        }
      }

      // Only perform the actual setting of positions if we aren't skipping
      if (skip == 0 && running == NDPOS_RUNNING){
        // We always keep the last array so read() can use it.
        // Release previous one. Reserve new one below during the copy.
        if (this->pArrays[0]){
          this->pArrays[0]->release();
          this->pArrays[0] = NULL;
        }
        // We must make a copy of the array as we are going to alter it
        this->pArrays[0] = this->pNDArrayPool->copy(pArray, this->pArrays[0], 1);
        if (this->pArrays[0]){
          std::map<std::string, double> pos = positionArray[index];
          std::stringstream sspos;
          sspos << "[";
          bool firstTime = true;
          std::map<std::string, double>::iterator iter;
          for (iter = pos.begin(); iter != pos.end(); iter++){
            if (firstTime){
              firstTime = false;
            } else {
              sspos << ",";
            }
            sspos << iter->first << "=" << iter->second;
            // Create the NDAttribute with the position data
            NDAttribute *pAtt = new NDAttribute(iter->first.c_str(), "Position of NDArray", NDAttrSourceDriver, driverName, NDAttrFloat64, &(iter->second));
            // Add the NDAttribute to the NDArray
            this->pArrays[0]->pAttributeList->add(pAtt);
          }
          sspos << "]";
          setStringParam(NDPos_CurrentPos, sspos.str().c_str());

        } else {
          // We were unable to allocate the required buffer (memory or qty exceeded).
          // This results in us dropping a frame, note it and print an error
          asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s::%s ERROR: dropped frame! Could not allocate the required buffer\n",
                    driverName, functionName);
          // Note the frame drop
          getIntegerParam(NDPluginDriverDroppedArrays, &dropped);
          dropped++;
          setIntegerParam(NDPluginDriverDroppedArrays, dropped);
          skip = 1;
        }

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
        // Increment the expectedID by the difference
        expectedID += IDDifference;
        setIntegerParam(NDPos_ExpectedID, expectedID);
      }
    }
    // If the size has dropped to zero then we've run out of positions, abort
    if (positionArray.size() == 0){
      setIntegerParam(NDPos_Running, NDPOS_IDLE);
    }
    callParamCallbacks();
    if (skip == 0 && running == NDPOS_RUNNING){
      this->unlock();
      doCallbacksGenericPointer(this->pArrays[0], NDArrayData, 0);
      this->lock();
    }
  }
}

/** Sets an int32 parameter.
  * \param[in] pasynUser asynUser structure that contains the function code in pasynUser->reason. 
  * \param[in] value The value for this parameter 
  *
  * Takes action if the function code requires it.
  */
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

    if (function == NDPos_Running){
      // Reset the expected ID to the starting value
      int expected = 0;
      getIntegerParam(NDPos_IDStart, &expected);
      setIntegerParam(NDPos_ExpectedID, expected);
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

/** Called when asyn clients call pasynOctet->write().
  * This function performs actions for some parameters, including AttributesFile.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Address of the string to write.
  * \param[in] nChars Number of characters to write.
  * \param[out] nActual Number of characters actually written.
  */
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
    // If the status of validation is OK then load the file
    if (status == asynSuccess){
      // Call the loadFile function
      status = loadFile();
    }
  } else if (function < FIRST_NDPOS_PARAM){
    // If this parameter belongs to a base class call its method
    status = NDPluginDriver::writeOctet(pasynUser, value, nChars, nActual);
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

/** Loads an XML position definition file.
  * This function reads the filename and valid parameters and if the file is considered valid
  * then the positions are loaded and appended to the position set.
  */
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
    std::vector<std::map<std::string, double> > positions = fr.readPositions();
    positionArray.insert(positionArray.end(), positions.begin(), positions.end());
    setIntegerParam(NDPos_CurrentQty, positionArray.size());
    callParamCallbacks();
  } else {
    status = asynError;
  }

  return status;
}

/** Constructor for the NDPosPlugin class.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when 
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxBuffers The maximum number of buffers that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPosPlugin::NDPosPlugin(const char *portName,
                         int queueSize,
                         int blockingCallbacks,
                         const char *NDArrayPort,
                         int NDArrayAddr,
                         int maxBuffers,
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
                   maxBuffers,
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
  createParam(str_NDPos_Filename,        asynParamOctet,        &NDPos_Filename);
  createParam(str_NDPos_FileValid,       asynParamInt32,        &NDPos_FileValid);
  createParam(str_NDPos_Clear,           asynParamInt32,        &NDPos_Clear);
  createParam(str_NDPos_Running,         asynParamInt32,        &NDPos_Running);
  createParam(str_NDPos_Restart,         asynParamInt32,        &NDPos_Restart);
  createParam(str_NDPos_Delete,          asynParamInt32,        &NDPos_Delete);
  createParam(str_NDPos_Mode,            asynParamInt32,        &NDPos_Mode);
  createParam(str_NDPos_Append,          asynParamInt32,        &NDPos_Append);
  createParam(str_NDPos_CurrentQty,      asynParamInt32,        &NDPos_CurrentQty);
  createParam(str_NDPos_CurrentIndex,    asynParamInt32,        &NDPos_CurrentIndex);
  createParam(str_NDPos_CurrentPos,      asynParamOctet,        &NDPos_CurrentPos);
  createParam(str_NDPos_MissingFrames,   asynParamInt32,        &NDPos_MissingFrames);
  createParam(str_NDPos_DuplicateFrames, asynParamInt32,        &NDPos_DuplicateFrames);
  createParam(str_NDPos_ExpectedID,      asynParamInt32,        &NDPos_ExpectedID);
  createParam(str_NDPos_IDName,          asynParamOctet,        &NDPos_IDName);
  createParam(str_NDPos_IDDifference,    asynParamInt32,        &NDPos_IDDifference);
  createParam(str_NDPos_IDStart,         asynParamInt32,        &NDPos_IDStart);

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
  setIntegerParam(NDPos_Running,           NDPOS_IDLE);
  // Set the ID Name to the default UniqueID
  setStringParam(NDPos_IDName,             "");
  // Set the ID Difference to a default of 1
  setIntegerParam(NDPos_IDDifference,      1);
  // Set the ID Start value to a default of 1
  setIntegerParam(NDPos_IDStart,           1);
  // Set the Expected next ID value to 1
  setIntegerParam(NDPos_ExpectedID,        1);
  // Set the duplicate frames to 0
  setIntegerParam(NDPos_MissingFrames,     0);
  // Set the missing frames to 0
  setIntegerParam(NDPos_DuplicateFrames,   0);

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
                                    int maxBuffers,
                                    size_t maxMemory,
                                    int priority,
                                    int stackSize)
{
  new NDPosPlugin(portName,
                  queueSize,
                  blockingCallbacks,
                  NDArrayPort,
                  NDArrayAddr,
                  maxBuffers,
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
static const iocshArg initArg5 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg6 = { "maxMemory",iocshArgInt};
static const iocshArg initArg7 = { "priority",iocshArgInt};
static const iocshArg initArg8 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8};
static const iocshFuncDef initFuncDef = {"NDPosPluginConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
  NDPosPluginConfigure(args[0].sval,
                       args[1].ival,
                       args[2].ival,
                       args[3].sval,
                       args[4].ival,
                       args[5].ival,
                       args[6].ival,
                       args[7].ival,
                       args[8].ival);
}

extern "C" void NDPosPluginRegister(void)
{
  iocshRegister(&initFuncDef,initCallFunc);
}

extern "C"
{
  epicsExportRegistrar(NDPosPluginRegister);
}
