/*
 * AsynException.h
 *
 *  Created on: 24 Jun 2015
 *      Author: gnx91527
 */

#ifndef ADAPP_PLUGINTESTS_ASYNEXCEPTION_H_
#define ADAPP_PLUGINTESTS_ASYNEXCEPTION_H_

#include <exception>
#include <string>

class AsynException
{
public:
  AsynException();
  virtual ~AsynException() throw ();
};

#endif /* ADAPP_PLUGINTESTS_ASYNEXCEPTION_H_ */
