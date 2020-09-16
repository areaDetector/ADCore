/*
 * NDPosPlugin.h
 *
 *  Created on: 19 May 2015
 *      Author: gnx91527
 *
 *  Use this plugin to attach positional data to NDArrays
 *  as they pass through.  The positional data is appended
 *  as a 1D integer valued attribute [x] or [x,y] etc.
 *
 *  The following parameters are used to interact with this plugin:
 *
 *  NDPos_Filename           - Filename to load positional data from
 *  NDPos_FileValid          - Is the currently selected filename a valid location
 *  NDPos_Load               - Load the filename specified above
 *  NDPos_Clear              - Clear the current positional data store
 *  NDPos_NameIndex1         - Name of attribute for first index to add to the NDArray
 *  NDPos_NameIndex2         - Name of attribute for second index to add to the NDArray
 *  NDPos_NameIndex3         - Name of attribute for third index to add to the NDArray
 *  NDPos_Running            - Used to turn on/off the position attaching
 *  NDPos_Restart            - Restart appending from the beginning of the store
 *  NDPos_Mode               - [0 - Discard] positions after use (for rolling buffer)
 *                           - [1 - Keep] positions in store
 *  NDPos_Append             - Add a single position to the end of the store
 *  NDPos_CurrentQty         - Number of loaded positions in the store
 *  NDPos_CurrentIndex       - Current index of position in store (0 if Discard mode)
 *  NDPos_CurrentPos         - Value of the next position to attach to the NDArray
 */

#ifndef NDPosPluginAPP_SRC_NDPOSPLUGIN_H_
#define NDPosPluginAPP_SRC_NDPOSPLUGIN_H_

#include <string>
#include <list>
#include <map>

#include "NDPluginDriver.h"

#define str_NDPos_Filename        "NDPos_Filename"
#define str_NDPos_FileValid       "NDPos_FileValid"
#define str_NDPos_Clear           "NDPos_Clear"
#define str_NDPos_Running         "NDPos_Running"
#define str_NDPos_Restart         "NDPos_Restart"
#define str_NDPos_Delete          "NDPos_Delete"
#define str_NDPos_Mode            "NDPos_Mode"
#define str_NDPos_Append          "NDPos_Append"
#define str_NDPos_CurrentQty      "NDPos_CurrentQty"
#define str_NDPos_CurrentIndex    "NDPos_CurrentIndex"
#define str_NDPos_CurrentPos      "NDPos_CurrentPos"
#define str_NDPos_MissingFrames   "NDPos_MissingFrames"
#define str_NDPos_DuplicateFrames "NDPos_DuplicateFrames"
#define str_NDPos_ExpectedID      "NDPos_ExpectedID"
#define str_NDPos_IDName          "NDPos_IDName"
#define str_NDPos_IDDifference    "NDPos_IDDifference"
#define str_NDPos_IDStart         "NDPos_IDStart"

#define MODE_DISCARD 0
#define MODE_KEEP    1

#define NDPOS_IDLE    0
#define NDPOS_RUNNING 1

class NDPLUGIN_API NDPosPlugin : public NDPluginDriver
{

public:
  NDPosPlugin(const char *portName,      // The name of the asyn port driver to be created.
              int queueSize,             // The number of NDArrays that the input queue for this plugin can hold.
              int blockingCallbacks,     // Initial setting for the NDPluginDriverBlockingCallbacks flag.
              const char *NDArrayPort,   // Name of asyn port driver for initial source of NDArray callbacks.
              int NDArrayAddr,           // asyn port driver address for initial source of NDArray callbacks.
              int maxBuffers,            // The maximum number of buffers that this driver can allocate.
              size_t maxMemory,          // The maximum amount of memory that this driver can allocate.
              int priority,              // The thread priority for the asyn port driver thread.
              int stackSize);            // The stack size for the asyn port driver thread.
  virtual ~NDPosPlugin();
  // These methods override the virtual methods in the base class
  void processCallbacks(NDArray *pArray);
  asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual);

protected:
  // plugin parameters
  int NDPos_Filename;
  #define FIRST_NDPOS_PARAM NDPos_Filename
  int NDPos_FileValid;
  int NDPos_Clear;
  int NDPos_Running;
  int NDPos_Restart;
  int NDPos_Delete;
  int NDPos_Mode;
  int NDPos_Append;
  int NDPos_CurrentQty;
  int NDPos_CurrentIndex;
  int NDPos_CurrentPos;
  int NDPos_DuplicateFrames;
  int NDPos_MissingFrames;
  int NDPos_ExpectedID;
  int NDPos_IDName;
  int NDPos_IDDifference;
  int NDPos_IDStart;

private:
  // Plugin member variables
  std::list<std::map<std::string, double> > positionArray;
};

#endif /* NDPosPluginAPP_SRC_NDPOSPLUGIN_H_ */
