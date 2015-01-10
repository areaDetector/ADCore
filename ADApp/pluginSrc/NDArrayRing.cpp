/*
 * NDArrayRing.cpp
 *
 * Circular ring of NDArray pointers
 * Author: Alan Greer
 *
 * Created June 21, 2013
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include "NDArrayRing.h"

NDArrayRing::NDArrayRing(int noOfBuffers)
{
  noOfBuffers_ = noOfBuffers;
  buffers_ = NULL;
  writeIndex_ = -1;
  readIndex_ = -1;
  wrapped_ = 0;

  buffers_ = new NDArray *[noOfBuffers_];
  for (int index = 0; index < noOfBuffers_; index++){
    buffers_[index] = NULL;
  }
}

NDArrayRing::~NDArrayRing()
{
  clear();
}

int NDArrayRing::size()
{
  if (wrapped_ == 1){
    return noOfBuffers_;
  }
  return writeIndex_ + 1;
}

NDArray *NDArrayRing::addToEnd(NDArray *pArray)
{
  NDArray *retVal = NULL;

  if (noOfBuffers_ > 0) {
      writeIndex_ = (writeIndex_ + 1) % noOfBuffers_;
      if (wrapped_ == 1){
	  retVal = buffers_[writeIndex_];
      }
      buffers_[writeIndex_] = pArray;
      if (writeIndex_+1 == noOfBuffers_){
	  // We have now wrapped
	  wrapped_ = 1;
      }
  } else {
      // Buffer is not being used, so return the passed array to be released immediately.
      retVal = pArray;
  }
  return retVal;
}

// Return the oldest frame in the buffer
NDArray *NDArrayRing::readFromStart()
{
  if (wrapped_) {
    readIndex_ = ((writeIndex_+1) % noOfBuffers_);
  } else {
    readIndex_ = 0;
  }

  //printf("readFromStart - Readindex %d\n", readIndex_);
  return buffers_[readIndex_];
}

NDArray *NDArrayRing::readNext()
{
  readIndex_ = (readIndex_ + 1) % noOfBuffers_;
  //printf("readNext - Readindex %d\n", readIndex_);
  return buffers_[readIndex_];
}

bool NDArrayRing::hasNext()
{
  // Here readIndex is index of last frame read out
  if (readIndex_ == writeIndex_){
    return false;
  }
  return true;
}

void NDArrayRing::clear()
{
  writeIndex_ = -1;
  wrapped_ = 0;
  if (buffers_){
    for (int index = 0; index < noOfBuffers_; index++){
      if (buffers_[index]){
        buffers_[index]->release();
      }
    }
    delete[] buffers_;
  }
}


