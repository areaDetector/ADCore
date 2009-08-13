/*
 * NDFileTIFF.h
 * Writes NDArrays to TIFF files.
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

typedef enum {
	NDFileNexusTemplatePath = NDPluginFileLastParam,
	NDFileNexusTemplateFile,
	NDFileNexusLastParam
} NDFileNexusParam_t;

static asynParamString_t NDFileNexusParamString[] = {
	{NDFileNexusTemplatePath, "TEMPLATE_FILE_PATH"},
	{NDFileNexusTemplateFile, "TEMPLATE_FILE_NAME"},
};

#define NUM_ND_FILE_NEXUS_PARAMS (sizeof(NDFileNexusParamString)/sizeof(NDFileNexusParamString[0]))

/** Writes NDArrays in the Nexus file format.
    */
class NDFileNexus : public NDPluginFile {
public:
    NDFileNexus(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int priority, int stackSize);

    /* The methods that this class implements */
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
    virtual asynStatus readFile(NDArray **pArray);
    virtual asynStatus writeFile(NDArray *pArray);
    virtual asynStatus closeFile();
    asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo,
                             const char **pptypeName, size_t *psize);

private:
/*    TIFF *output;
*/
	NXhandle nxFileHandle;
	int bitsPerSample;
    NDColorMode_t colorMode;
	TiXmlDocument configDoc;
	TiXmlElement *rootNode;
	int processNode(TiXmlNode *curNode, NDArray *);
	void getAttrTypeNSize(NDAttribute *pAttr, int *retType, int *retSize);
	void iterateNodes(TiXmlNode *curNode, NDArray *pArray);
	void findConstText(TiXmlNode *curNode, char *outtext);
	void * allocConstValue(int dataType, int length);
	void constTextToDataType(char *inText, int dataType, void *pValue);
	int typeStringToVal( const char * typeStr );

};

/* static const char *driverName="NDFileNexus"; */

#endif

