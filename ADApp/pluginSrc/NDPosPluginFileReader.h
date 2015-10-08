/*
 * NDPosPluginFileReader.h
 *
 *  Created on: 21 May 2015
 *      Author: gnx91527
 */

#ifndef POSPLUGINAPP_SRC_NDPOSPLUGINFILEREADER_H_
#define POSPLUGINAPP_SRC_NDPOSPLUGINFILEREADER_H_

#include "asynDriver.h"
#include <libxml/xmlreader.h>
#include <string>
#include <vector>
#include <map>

class NDPosPluginFileReader
{
public:
  static const std::string ELEMENT_NAME;
  static const std::string ELEMENT_DIMENSIONS;
  static const std::string ELEMENT_DIMENSION;
  static const std::string ELEMENT_POSITIONS;
  static const std::string ELEMENT_POSITION;

  static const std::string DIMENSION_NAME;

  NDPosPluginFileReader();
  virtual ~NDPosPluginFileReader();
  asynStatus validateXML(const std::string& filename);
  asynStatus loadXML(const std::string& filename);
  std::vector<std::string> readDimensions();
  std::vector<std::map<std::string, double> > readPositions();
  asynStatus clearPositions();
  asynStatus processNode();
  asynStatus addDimension();
  asynStatus addPosition();
  std::string getErrorMsg();

protected:
  void setErrorMsg(const std::string& msg);

private:
  xmlTextReaderPtr xmlreader;
  std::vector<std::string> dimensions;
  std::vector<std::map<std::string, double> > positions;
  std::string errorMessage;
};

#endif /* POSPLUGINAPP_SRC_NDPOSPLUGINFILEREADER_H_ */
