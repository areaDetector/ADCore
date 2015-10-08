/*
 * NDPosPluginFileReader.cpp
 *
 *  Created on: 21 May 2015
 *      Author: gnx91527
 */

#include "NDPosPluginFileReader.h"
#include <sstream>

const std::string NDPosPluginFileReader::ELEMENT_NAME       = "name";
const std::string NDPosPluginFileReader::ELEMENT_DIMENSIONS = "dimensions";
const std::string NDPosPluginFileReader::ELEMENT_DIMENSION  = "dimension";
const std::string NDPosPluginFileReader::ELEMENT_POSITIONS  = "positions";
const std::string NDPosPluginFileReader::ELEMENT_POSITION   = "position";

const std::string NDPosPluginFileReader::DIMENSION_NAME     = "name";

NDPosPluginFileReader::NDPosPluginFileReader()
  : xmlreader(NULL)
{
}

NDPosPluginFileReader::~NDPosPluginFileReader()
{
}

asynStatus NDPosPluginFileReader::validateXML(const std::string& filename)
{
  asynStatus status = asynSuccess;
  int ret = 0;
  // Check for either a valid XML file or a valid XML string
  // if the file name contains <pos_layout> then load it as an xml string from memory
  if (filename.find("<pos_layout>") != std::string::npos){
    xmlreader = xmlReaderForMemory(filename.c_str(), (int)filename.length(), NULL, NULL, 0);
  } else {
    xmlreader = xmlReaderForFile(filename.c_str(), NULL, 0);
  }

  if (xmlreader == NULL){
    setErrorMsg("Error creating XML parser, check file");
    status = asynError;
  }

  if (status == asynSuccess){
    while ((ret = xmlTextReaderRead(xmlreader)) == 1){
      // Do nothing, we're just trying to validate
    }
    xmlFreeTextReader(xmlreader);
    xmlreader = NULL;
    if (ret != 0){
      setErrorMsg("XML parsing failed, check file format");
      status = asynError;
    }
  }

  return status;
}

asynStatus NDPosPluginFileReader::loadXML(const std::string& filename)
{
  asynStatus status = asynSuccess;
  int ret = 0;
  // if the file name contains <pos_layout> then load it as an xml string from memory
  if (filename.find("<pos_layout>") != std::string::npos){
    xmlreader = xmlReaderForMemory(filename.c_str(), (int)filename.length(), NULL, NULL, 0);
  } else {
    xmlreader = xmlReaderForFile(filename.c_str(), NULL, 0);
  }

  if (xmlreader == NULL){
    setErrorMsg("Error creating XML parser, check file");
    status = asynError;
  }

  if (status == asynSuccess){
    while ((ret = xmlTextReaderRead(xmlreader)) == 1){
      this->processNode();
    }
    xmlFreeTextReader(xmlreader);
    xmlreader = NULL;
    if (ret != 0){
      setErrorMsg("XML parsing failed, check file format");
      status = asynError;
    }
  }

  return status;
}

std::vector<std::string> NDPosPluginFileReader::readDimensions()
{
  return dimensions;
}

std::vector<std::map<std::string, double> > NDPosPluginFileReader::readPositions()
{
  return positions;
}

asynStatus NDPosPluginFileReader::clearPositions()
{
  positions.clear();
  dimensions.clear();
  return asynSuccess;
}

asynStatus NDPosPluginFileReader::processNode()
{
  asynStatus status = asynSuccess;
  xmlReaderTypes type = (xmlReaderTypes)xmlTextReaderNodeType(xmlreader);
  const xmlChar* xmlname = NULL;

  xmlname = xmlTextReaderConstName(xmlreader);
  if (xmlname != NULL){

    std::string name((const char*)xmlname);
    switch(type)
    {
      // Elements can be either 'dimensions', 'dimension', 'positions' or 'position'
      case XML_READER_TYPE_ELEMENT:
        if (name == NDPosPluginFileReader::ELEMENT_DIMENSIONS){
          // Currently do nothing
        } else if (name == NDPosPluginFileReader::ELEMENT_DIMENSION){
          status = addDimension();
        } else if (name == NDPosPluginFileReader::ELEMENT_POSITIONS){
          // Currently do nothing
        } else if (name == NDPosPluginFileReader::ELEMENT_POSITION){
          status = addPosition();
        }
        if (status != asynSuccess){
          // Perhaps display a warning here for bad XML
          setErrorMsg("Possible bad XML format, check file");
        }
        break;

      default:
        break;
    }
  }
  return status;
}

asynStatus NDPosPluginFileReader::addDimension()
{
  asynStatus status = asynSuccess;
  xmlChar *dim_name = NULL;

  // First check the basics
  if (!xmlTextReaderHasAttributes(this->xmlreader)){
    status = asynError;
  }

  // Check for the dimension name specified in the element
  if (status == asynSuccess){
    dim_name = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar *)NDPosPluginFileReader::DIMENSION_NAME.c_str());
    if (dim_name == NULL){
      status = asynError;
    }
  }

  // Add the dimension to the vector of dimensions
  if (status == asynSuccess){
    std::string str_dim_name;
    str_dim_name = (char*)dim_name;
    //printf("Adding dimension: %s\n", str_dim_name.c_str());
    // Add the dimension to the vector of dimension names
    dimensions.push_back(str_dim_name);
  }
  return status;
}

asynStatus NDPosPluginFileReader::addPosition()
{
  asynStatus status = asynSuccess;
  xmlChar *pos_val = NULL;
  std::string pos_str;
  std::map<std::string, double> pos;

  // First check the basics
  if (!xmlTextReaderHasAttributes(this->xmlreader)){
    status = asynError;
  }

  double index;

  // Loop over the dimensions looking for each
  for (std::vector<std::string>::iterator iter = dimensions.begin(); iter != dimensions.end(); iter++){
    // Check for the dimension name specified in the element
    if (status == asynSuccess){
      pos_val = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar *)(*iter).c_str());
      if (pos_val == NULL){
        status = asynError;
      } else {
        // Convert the string value into an integer index
        std::stringstream sindex((char *)pos_val);
        sindex >> index;
        if (!sindex){
          status = asynError;
        } else {
          // Insert the dimension index into the position map
          pos[*iter] = index;
        }
      }
    }
  }

  if (status == asynSuccess){
    //printf("Adding position: %s\n", pos_str.c_str());
    positions.push_back(pos);
  }
  return status;
}

std::string NDPosPluginFileReader::getErrorMsg()
{
  return errorMessage;
}

void NDPosPluginFileReader::setErrorMsg(const std::string& msg)
{
  errorMessage = msg;
}
