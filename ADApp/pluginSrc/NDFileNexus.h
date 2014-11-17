/*
 * NDFileTIFF.h
 * Writes NDArrays to NeXus (HDF) files.
 * John Hammonds
 * April 17, 2009
 */

#ifndef DRV_NDFileNexus_H
#define DRV_NDFileNexus_H

#include "NDPluginFile.h"
#include <napi.h>
#include <tinyxml.h>

/* This version number is an attribute in the Nexus file to allow readers
 * to handle changes in the file contents */
#define NDNexusFileVersion 1.0


#define NDFileNexusTemplatePathString "TEMPLATE_FILE_PATH"
#define NDFileNexusTemplateFileString "TEMPLATE_FILE_NAME"
#define NDFileNexusTemplateValidString "TEMPLATE_FILE_VALID"
#define NUM_ND_FILE_NEXUS_PARAMS (sizeof(NDFileNexusParamString)/sizeof(NDFileNexusParamString[0]))

/** Writes NDArrays in the NeXus file format.
  * Uses an XML template file to configure the contents of the NeXus file.
  *
  * This version is currently limited to writing a single NDArray to each NeXus file.
  * Future releases will be capable of storing multiple NDArrays in each NeXus file.
  */
class epicsShareClass NDFileNexus : public NDPluginFile {
public:
    NDFileNexus(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int priority, int stackSize);

    /* The methods that this class implements */
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
    virtual asynStatus readFile(NDArray **pArray);
    virtual asynStatus writeFile(NDArray *pArray);
    virtual asynStatus closeFile();
    asynStatus writeOctet(asynUser *pasynUser, const char *value,
                                    size_t nChars, size_t *nActual);
protected:
    int NDFileNexusTemplatePath;
    #define FIRST_NDFILE_NEXUS_PARAM NDFileNexusTemplatePath
    int NDFileNexusTemplateFile;
    int NDFileNexusTemplateValid;
    #define LAST_NDFILE_NEXUS_PARAM NDFileNexusTemplateValid

private:
    NXhandle nxFileHandle;
    int bitsPerSample;
    NDColorMode_t colorMode;
    TiXmlDocument configDoc;
    TiXmlElement *rootNode;
    NDAttributeList *pFileAttributes;
    NXname dataPath;
    NXname dataName;
    int imageNumber;

    int processNode(TiXmlNode *curNode, NDArray *);
    int processStreamData(NDArray *);
    void getAttrTypeNSize(NDAttribute *pAttr, int *retType, int *retSize);
    void iterateNodes(TiXmlNode *curNode, NDArray *pArray);
    void findConstText(TiXmlNode *curNode, char *outtext);
    void * allocConstValue(int dataType, size_t length);
    void constTextToDataType(char *inText, int dataType, void *pValue);
    int typeStringToVal( const char * typeStr );
    void loadTemplateFile();

};
#define NUM_NDFILE_NEXUS_PARAMS ((int)(&LAST_NDFILE_NEXUS_PARAM - &FIRST_NDFILE_NEXUS_PARAM + 1))

#endif

