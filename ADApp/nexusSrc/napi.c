/*---------------------------------------------------------------------------
  NeXus - Neutron & X-ray Common Data Format
  
  Application Program Interface Routines
  
  Copyright (C) 1997-2006 Mark Koennecke, Przemek Klosowski
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.
 
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
 
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 
  For further information, see <http://www.neutron.anl.gov/NeXus/>

----------------------------------------------------------------------------*/

/* static const char* rscid = "$Id: napi.c 1429 2010-02-24 19:15:27Z Freddie Akeroyd $"; */	/* Revision inserted by CVS */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include "napi.h"
#include "nxstack.h"

/**
 * \mainpage NeXus API documentation
 * \section sec_purpose Purpose of API
 * The NeXus Application Program Interface is a suite of subroutines, written in C but with wrappers in Fortran 77 and 90. 
 *  The subroutines call HDF routines to read and write the NeXus files with the correct structure. 
 * An API serves a number of useful purposes: 
 * - It simplifies the reading and writing of NeXus files. 
 * - It ensures a certain degree of compliance with the NeXus standard. 
 * - It allows the development of sophisticated input/output features such as automatic unit conversion. This has not been implemented yet. 
 * - It hides the implementation details of the format. In particular, the API can read and write HDF4, 
     HDF5 (and shortly XML) files using the same routines. 
 * For these reasons, we request that all NeXus files are written using the supplied API. We cannot be 
 * sure that anything written using the underlying HDF API will be recognized by NeXus-aware utilities. 
 *
 * \section sec_core Core API
 * The core API provides the basic routines for reading, writing and navigating NeXus files. It is designed to be modal; 
 * there is a hidden state that determines which groups and data sets are open at any given moment, and 
 * subsequent operations are implicitly performed on these entities. This cuts down the number of parameters 
 * to pass around in API calls, at the cost of forcing a certain pre-approved mode d'emploi. This mode d'emploi 
 * will be familiar to most: it is very similar to navigating a directory hierarchy; in our case, NeXus groups are the 
 * directories, which contain data sets and/or other directories. 
 *
 * The core API comprises several functional groups which are listed on the \b Modules tab 
 *
 * C programs that call the above routines should include the following header file: 
 * \code
 * #include "napi.h"
 * \endcode
 */



/*---------------------------------------------------------------------
 Recognized and handled napimount URLS
 -----------------------------------------------------------------------*/
#define NXBADURL 0
#define NXFILE 1

/*--------------------------------------------------------------------*/
/* static int iFortifyScope; */
/*----------------------------------------------------------------------
  This is a section with code for searching the NX_LOAD_PATH
  -----------------------------------------------------------------------*/
#ifdef _WIN32
#define LIBSEP ";"
#define PATHSEP "\\"
#else
#define LIBSEP ":"
#define PATHSEP "/"
#endif

#include "nx_stptok.h"

/*---------------------------------------------------------------------
 wrapper for getenv. This is a future proofing thing for porting to OS
 which have different ways of accessing environment variables
 --------------------------------------------------------------------*/ 
static char *nxgetenv(const char *name){
  return getenv(name);
}
/*----------------------------------------------------------------------*/
static int canOpen(char *filename){
  FILE *fd = NULL;

  fd = fopen(filename,"r");
  if(fd != NULL){
    fclose(fd);
    return 1;
  } else {
    return 0;
  }
} 
/*--------------------------------------------------------------------*/
static char *locateNexusFileInPath(char *startName){
  char *loadPath = NULL, *testPath = NULL, *pPtr = NULL;
  char pathPrefix[256];
  size_t length;

  if(canOpen(startName)){
    return strdup(startName);
  }

  loadPath = nxgetenv("NX_LOAD_PATH");
  if(loadPath == NULL){
    /* file not found will be issued by upper level code */
    return strdup(startName);
  }

  pPtr = stptok(loadPath,pathPrefix,255,LIBSEP);
  while(pPtr != NULL){
    length = strlen(pathPrefix) + strlen(startName) + strlen(PATHSEP) + 2;
    testPath = (char*)malloc(length*sizeof(char));
    if(testPath == NULL){
      return strdup(startName);
    }
    memset(testPath,0,length*sizeof(char));
    strcpy(testPath, pathPrefix);
    strcat(testPath,PATHSEP);
    strcat(testPath,startName);
    if(canOpen(testPath)){
      return(testPath);
    }
    free(testPath);
    pPtr = stptok(pPtr,pathPrefix,255,LIBSEP);
  }
  return  strdup(startName);
}
/*------------------------------------------------------------------------
  HDF-5 cache size special stuff
  -------------------------------------------------------------------------*/
long nx_cacheSize =  1024000; /* 1MB, HDF-5 default */

NXstatus NXsetcache(long newVal)
{
  if(newVal > 0)
  {
    nx_cacheSize = newVal;
    return NX_OK;
  }
  return NX_ERROR;
}
    
/*-----------------------------------------------------------------------*/

#ifdef NXXML
static NXstatus NXisXML(CONSTCHAR *filename)
{
  FILE *fd = NULL;
  char line[132];

  fd = fopen(filename,"r");
  if(fd) {
    fgets(line,131,fd);
    fclose(fd);
    if(strstr(line,"?xml") != NULL){
      return NX_OK;
    }
  }
  return NX_ERROR;
}
#endif

/*-------------------------------------------------------------------------*/
  static void NXNXNXReportError(void *pData, char *string)
  {
    printf("%s \n",string);
  }
  /*---------------------------------------------------------------------*/
  void *NXpData = NULL;
  void (*NXIReportError)(void *pData, char *string) = NXNXNXReportError;
  /*---------------------------------------------------------------------*/
  extern void NXMSetError(void *pData, 
			      void (*NewError)(void *pD, char *text))
  {
    NXpData = pData;
    NXIReportError = NewError;
  }
/*----------------------------------------------------------------------*/
extern ErrFunc NXMGetError(){
  return NXIReportError;
}

/*----------------------------------------------------------------------*/
void NXNXNoReport(void *pData, char *string){
  /* do nothing */
}  
/*----------------------------------------------------------------------*/

static ErrFunc last_errfunc = NXNXNXReportError;

extern void NXMDisableErrorReporting()
{
    last_errfunc = NXMGetError();
    NXMSetError(NXpData, NXNXNoReport);
}

extern void NXMEnableErrorReporting()
{
    NXMSetError(NXpData, last_errfunc);
}

/*----------------------------------------------------------------------*/
#ifdef HDF5
#include "napi5.h"
#endif
#ifdef HDF4
#include "napi4.h"
#endif
#ifdef NXXML
#include "nxxml.h"
#endif  
  /* ---------------------------------------------------------------------- 
  
                          Definition of NeXus API

   ---------------------------------------------------------------------*/
static int determineFileType(CONSTCHAR *filename)
{
  FILE *fd = NULL;
  int iRet;
  
  /*
    this is for reading, check for existence first
  */
  fd = fopen(filename,"r");
  if(fd == NULL){
    return -1;
  }
  fclose(fd);
#ifdef HDF5
  iRet=H5Fis_hdf5((const char*)filename);
  if( iRet > 0){
    return 2;
  }
#endif  
#ifdef HDF4
  iRet=Hishdf((const char*)filename);
  if( iRet > 0){
    return 1;
  }
#endif
#ifdef NXXML
  iRet = NXisXML(filename);
  if(iRet == NX_OK){
    return 3;
  }
#endif
  /*
    file type not recognized
  */
  return 0;
}
/*---------------------------------------------------------------------*/
static pNexusFunction handleToNexusFunc(NXhandle fid){
  pFileStack fileStack = NULL;
  fileStack = (pFileStack)fid;
  if(fileStack != NULL){
    return peekFileOnStack(fileStack);
  } else {
    return NULL;
  }
}
/*--------------------------------------------------------------------*/
static NXstatus   NXinternalopen(CONSTCHAR *userfilename, NXaccess am, 
           pFileStack fileStack);
/*----------------------------------------------------------------------*/
NXstatus   NXopen(CONSTCHAR *userfilename, NXaccess am, NXhandle *gHandle){
  int status;
  pFileStack fileStack = NULL;

  *gHandle = NULL;
  fileStack = makeFileStack();
  if(fileStack == NULL){
    NXIReportError (NXpData,"ERROR: no memory to create filestack");
      return NX_ERROR;
  }
  status = NXinternalopen(userfilename,am,fileStack);
  if(status == NX_OK){
    *gHandle = fileStack;
  }

  return status;
}
/*-----------------------------------------------------------------------*/
static NXstatus   NXinternalopen(CONSTCHAR *userfilename, NXaccess am, pFileStack fileStack)
  {
    int hdf_type=0;
    int iRet=0;
    pNexusFunction fHandle = NULL;
    NXstatus retstat = NX_ERROR;
    char error[1024];
    char *filename = NULL;
    int my_am = (am & NXACCMASK_REMOVEFLAGS);
        
    /* configure fortify 
    iFortifyScope = Fortify_EnterScope();
    Fortify_CheckAllMemory();
    */
    
    /*
      allocate data
    */
    fHandle = (pNexusFunction)malloc(sizeof(NexusFunction));
    if (fHandle == NULL) {
      NXIReportError (NXpData,"ERROR: no memory to create Function structure");
      return NX_ERROR;
    }
    memset(fHandle, 0, sizeof(NexusFunction)); /* so any functions we miss are NULL */
       
    /*
      test the strip flag. Elimnate it for the rest of the tests to work
    */
    fHandle->stripFlag = 1;
    if(am & NXACC_NOSTRIP){
      fHandle->stripFlag = 0;
      am = (NXaccess)(am & ~NXACC_NOSTRIP);
    }

    if (my_am==NXACC_CREATE) {
      /* HDF4 will be used ! */
      hdf_type=1;
      filename = strdup(userfilename);
    } else if (my_am==NXACC_CREATE4) {
      /* HDF4 will be used ! */
      hdf_type=1;   
      filename = strdup(userfilename);
    } else if (my_am==NXACC_CREATE5) {
      /* HDF5 will be used ! */
      hdf_type=2;   
      filename = strdup(userfilename);
    } else if (my_am==NXACC_CREATEXML) {
      /* XML will be used ! */
      hdf_type=3;   
      filename = strdup(userfilename);
    } else {
      filename = locateNexusFileInPath((char *)userfilename);
      if(filename == NULL){
	NXIReportError(NXpData,"Out of memory in NeXus-API");
	free(fHandle);
	return NX_ERROR;
      }
      /* check file type hdf4/hdf5/XML for reading */
      iRet = determineFileType(filename);
      if(iRet < 0) {
	snprintf(error,1023,"failed to open %s for reading",
		 filename);
	NXIReportError(NXpData,error);
	free(filename);
	return NX_ERROR;
      }
      if(iRet == 0){
	snprintf(error,1023,"failed to determine filetype for %s ",
		 filename);
	NXIReportError(NXpData,error);
	free(filename);
	free(fHandle);
	return NX_ERROR;
      }
      hdf_type = iRet;
    }
    if(filename == NULL){
	NXIReportError(NXpData,"Out of memory in NeXus-API");
	return NX_ERROR;
    }

    if (hdf_type==1) {
      /* HDF4 type */
#ifdef HDF4
    NXhandle hdf4_handle = NULL;
      retstat = NX4open((const char *)filename,am,&hdf4_handle);
      if(retstat != NX_OK){
	free(fHandle);
	free(filename);
	return retstat;
      }
      fHandle->pNexusData=hdf4_handle;
      NX4assignFunctions(fHandle);
      pushFileStack(fileStack,fHandle,filename);
#else
      NXIReportError (NXpData,
         "ERROR: Attempt to create HDF4 file when not linked with HDF4");
      retstat = NX_ERROR;
#endif /* HDF4 */
      free(filename);
      return retstat; 
    } else if (hdf_type==2) {
      /* HDF5 type */
#ifdef HDF5
    NXhandle hdf5_handle = NULL;
      retstat = NX5open(filename,am,&hdf5_handle);
      if(retstat != NX_OK){
	free(fHandle);
	free(filename);
	return retstat;
      }
      fHandle->pNexusData=hdf5_handle;
      NX5assignFunctions(fHandle);
      pushFileStack(fileStack,fHandle, filename);
#else
      NXIReportError (NXpData,
	 "ERROR: Attempt to create HDF5 file when not linked with HDF5");
      retstat = NX_ERROR;
#endif /* HDF5 */
      free(filename);
      return retstat;
    } else if(hdf_type == 3){
      /*
	XML type
      */
#ifdef NXXML
    NXhandle xmlHandle = NULL;
      retstat = NXXopen(filename,am,&xmlHandle);
      if(retstat != NX_OK){
	free(fHandle);
	free(filename);
	return retstat;
      }
      fHandle->pNexusData=xmlHandle;
      NXXassignFunctions(fHandle);
      pushFileStack(fileStack,fHandle, filename);
#else
      NXIReportError (NXpData,
	 "ERROR: Attempt to create XML file when not linked with XML");
      retstat = NX_ERROR;
#endif
    } else {
      NXIReportError (NXpData,
          "ERROR: Format not readable by this NeXus library");
      retstat = NX_ERROR;
    }
    if (filename != NULL) {
 	free(filename); 
    }
    return retstat;
  }

/* ------------------------------------------------------------------------- */

  NXstatus  NXclose (NXhandle *fid)
  { 
    NXhandle hfil; 
    int status;
    pFileStack fileStack = NULL;
    pNexusFunction pFunc=NULL;
    if (*fid == NULL)
    {
	return NX_OK;
    }
    fileStack = (pFileStack)*fid;
    pFunc = peekFileOnStack(fileStack);
    hfil = pFunc->pNexusData;
    status =  pFunc->nxclose(&hfil);
    pFunc->pNexusData = hfil;
    free(pFunc);
    popFileStack(fileStack);
    if(fileStackDepth(fileStack) < 0){
      killFileStack(fileStack);
      *fid = NULL;
    }
    /* we can't set fid to NULL always as the handle points to a stack of files for external file support */
    /* 
    Fortify_CheckAllMemory();
    */

    return status;   
  }

  /*-----------------------------------------------------------------------*/   

  NXstatus  NXmakegroup (NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass) 
  {
     pNexusFunction pFunc = handleToNexusFunc(fid);
     return pFunc->nxmakegroup(pFunc->pNexusData, name, nxclass);   
  }
  /*------------------------------------------------------------------------*/
static int analyzeNapimount(char *napiMount, char *extFile, int extFileLen, 
			    char *extPath, int extPathLen){
  char *pPtr = NULL, *path = NULL;
  int length;

  memset(extFile,0,extFileLen);
  memset(extPath,0,extPathLen);
  pPtr = strstr(napiMount,"nxfile://");
  if(pPtr == NULL){
    return NXBADURL;
  }
  path = strrchr(napiMount,'#');
  if(path == NULL){
    length = (int)strlen(napiMount) - 9;
    if(length > extFileLen){
      NXIReportError(NXpData,"ERROR: internal errro with external linking");
      return NXBADURL;
    }
    memcpy(extFile,pPtr+9,length);
    strcpy(extPath,"/");
    return NXFILE;
  } else {
    pPtr += 9;
    length = (int)(path - pPtr);
    if(length > extFileLen){
      NXIReportError(NXpData,"ERROR: internal errro with external linking");
      return NXBADURL;
    }
    memcpy(extFile,pPtr,length);
    length = (int)strlen(path-1);
    if(length > extPathLen){
      NXIReportError(NXpData,"ERROR: internal error with external linking");
      return NXBADURL;
    }
    strcpy(extPath,path+1);
    return NXFILE;
  }
  return NXBADURL;
}
  /*------------------------------------------------------------------------*/

  NXstatus  NXopengroup (NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass)
  {
    int status, attStatus, type = NX_CHAR, length = 1023;
    NXaccess access = NXACC_RDWR;
    NXlink breakID;
    pFileStack fileStack;    
    char nxurl[1024], exfile[512], expath[512];
    ErrFunc oldError;
    pNexusFunction pFunc = NULL;

    fileStack = (pFileStack)fid;
    pFunc = handleToNexusFunc(fid);

    status = pFunc->nxopengroup(pFunc->pNexusData, name, nxclass);  
    if(status == NX_OK){
      pushPath(fileStack, (char *)name);
    }
    oldError = NXMGetError();
    NXIReportError = NXNXNoReport;
    attStatus = NXgetattr(fid,"napimount",nxurl,&length, &type);
    NXIReportError = oldError;
    if(attStatus == NX_OK){
      /*
	this is an external linking group
      */
      status = analyzeNapimount(nxurl,exfile,511,expath,511);
      if(status == NXBADURL){
	return NX_ERROR;
      }
      status = NXinternalopen(exfile,access, fileStack);
      if(status == NX_ERROR){
	return status;
      }
      status = NXopenpath(fid,expath);
      NXgetgroupID(fid,&breakID);
      setCloseID(fileStack,breakID);
    }
    
    return status;
  } 

  /* ------------------------------------------------------------------- */

  NXstatus  NXclosegroup (NXhandle fid)
  {
    int status;
    pFileStack fileStack = NULL;
    NXlink closeID, currentID;

    pNexusFunction pFunc = handleToNexusFunc(fid);
    fileStack = (pFileStack)fid;
    if(fileStackDepth(fileStack) == 0){
      status = pFunc->nxclosegroup(pFunc->pNexusData);  
      if(status == NX_OK){
	popPath(fileStack);
      }
      return status;
    } else {
      /* we have to check for leaving an external file */
      NXgetgroupID(fid,&currentID);
      peekIDOnStack(fileStack,&closeID);
      if(NXsameID(fid,&closeID,&currentID) == NX_OK){
	NXclose(&fid);
	status = NXclosegroup(fid);
      } else {
	status = pFunc->nxclosegroup(pFunc->pNexusData);
	if(status == NX_OK){
	  popPath(fileStack);
	}
      }
      return status;
    }
  }   

  /* --------------------------------------------------------------------- */
  
  NXstatus  NXmakedata (NXhandle fid, CONSTCHAR *name, int datatype, 
                                  int rank, int dimensions[])
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return pFunc->nxmakedata(pFunc->pNexusData, name, datatype, rank, dimensions); 
  }


 /* --------------------------------------------------------------------- */
  
  NXstatus  NXcompmakedata (NXhandle fid, CONSTCHAR *name, int datatype, 
                           int rank, int dimensions[],int compress_type, int chunk_size[])
  {
    pNexusFunction pFunc = handleToNexusFunc(fid); 
    return pFunc->nxcompmakedata (pFunc->pNexusData, name, datatype, rank, dimensions, compress_type, chunk_size); 
  } 
  
 
  /* --------------------------------------------------------------------- */

  NXstatus  NXcompress (NXhandle fid, int compress_type)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid); 
    return pFunc->nxcompress (pFunc->pNexusData, compress_type); 
  }
  
 
  /* --------------------------------------------------------------------- */
  
  NXstatus  NXopendata (NXhandle fid, CONSTCHAR *name)
  {
    int status;
    pFileStack fileStack = NULL;

    pNexusFunction pFunc = handleToNexusFunc(fid); 
    fileStack = (pFileStack)fid;
    status = pFunc->nxopendata(pFunc->pNexusData, name); 
    if(status == NX_OK){
      pushPath(fileStack,(char *)name);
    }
    return status;
  } 


  /* ----------------------------------------------------------------- */
    
  NXstatus  NXclosedata (NXhandle fid)
  { 
    int status;
    pFileStack fileStack = NULL;

    pNexusFunction pFunc = handleToNexusFunc(fid);
    fileStack = (pFileStack)fid;
    status = pFunc->nxclosedata(pFunc->pNexusData);
    if(status == NX_OK){
      popPath(fileStack);
    }
    return status;
  }

  /* ------------------------------------------------------------------- */

  NXstatus  NXputdata (NXhandle fid, void *data)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return pFunc->nxputdata(pFunc->pNexusData, data);
  }

  /* ------------------------------------------------------------------- */

  NXstatus  NXputattr (NXhandle fid, CONSTCHAR *name, void *data, 
                                  int datalen, int iType)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    if (datalen > 1 && iType != NX_CHAR)
    {
	NXIReportError(NXpData,"NXputattr: numeric arrays are not allowed as attributes - only character strings and single numbers");
	return NX_ERROR;
    }
    else
    {
        return pFunc->nxputattr(pFunc->pNexusData, name, data, datalen, iType);
    }
  }

  /* ------------------------------------------------------------------- */

  NXstatus  NXputslab (NXhandle fid, void *data, int iStart[], int iSize[])
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return pFunc->nxputslab(pFunc->pNexusData, data, iStart, iSize);
  }

  /* ------------------------------------------------------------------- */

  NXstatus  NXgetdataID (NXhandle fid, NXlink* sRes)
  {  
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return pFunc->nxgetdataID(pFunc->pNexusData, sRes);
  }


  /* ------------------------------------------------------------------- */

  NXstatus  NXmakelink (NXhandle fid, NXlink* sLink)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return pFunc->nxmakelink(pFunc->pNexusData, sLink);
  }
  /* ------------------------------------------------------------------- */

  NXstatus  NXmakenamedlink (NXhandle fid, CONSTCHAR *newname,  NXlink* sLink)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return pFunc->nxmakenamedlink(pFunc->pNexusData, newname, sLink);
  }
  /* --------------------------------------------------------------------*/
  NXstatus  NXopensourcegroup(NXhandle fid)
  {
    char target_path[512];
    int status, type = NX_CHAR, length = 511;

    status = NXgetattr(fid,"target",target_path,&length,&type);
    if(status != NX_OK)
    {
      NXIReportError(NXpData,"ERROR: item not linked");
      return NX_ERROR;
    }
    return NXopengrouppath(fid,target_path);
  }
  /*----------------------------------------------------------------------*/

  NXstatus  NXflush(NXhandle *pHandle)
  {
    NXhandle hfil; 
    pFileStack fileStack = NULL;
    int status;
   
    pNexusFunction pFunc=NULL;
    fileStack = (pFileStack)*pHandle;
    pFunc = peekFileOnStack(fileStack);
    hfil = pFunc->pNexusData;
    status =  pFunc->nxflush(&hfil);
    pFunc->pNexusData = hfil;
    return status; 
  }


  /*-------------------------------------------------------------------------*/
  
  NXstatus  NXmalloc (void** data, int rank, 
				   int dimensions[], int datatype)
  {
    int i;
    size_t size = 1;
    *data = NULL;
    for(i=0; i<rank; i++)
    size *= dimensions[i];
    if ((datatype == NX_CHAR) || (datatype == NX_INT8) 
	|| (datatype == NX_UINT8)) {
        /* allow for terminating \0 */
      size += 2;
      }
      else if ((datatype == NX_INT16) || (datatype == NX_UINT16)) {
      size *= 2;
      }    
      else if ((datatype == NX_INT32) || (datatype == NX_UINT32) 
	       || (datatype == NX_FLOAT32)) {
        size *= 4;
      }    
      else if ((datatype == NX_INT64) || (datatype == NX_UINT64)){
        size *= 8;
      }    
      else if (datatype == NX_FLOAT64) {
        size *= 8;
      }
      else {
        NXIReportError (NXpData,
			"ERROR: NXmalloc - unknown data type in array");
        return NX_ERROR;
    }
    *data = (void*)malloc(size);
     memset(*data,0,size);
    return NX_OK;
  }
    
  /*-------------------------------------------------------------------------*/

  NXstatus  NXfree (void** data)
  {
    if (data == NULL) {
       NXIReportError (NXpData, "ERROR: passing NULL to NXfree");
       return NX_ERROR;
    }
    if (*data == NULL) {
       NXIReportError (NXpData,"ERROR: passing already freed pointer to NXfree");
       return NX_ERROR;
    }
    free(*data);
    *data = NULL;
    return NX_OK;
  }

  /* --------------------------------------------------------------------- */
           
 
  NXstatus  NXgetnextentry (NXhandle fid, NXname name, NXname nxclass, int *datatype)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return pFunc->nxgetnextentry(pFunc->pNexusData, name, nxclass, datatype);  
  }
/*----------------------------------------------------------------------*/
/*
**  TRIM.C - Remove leading, trailing, & excess embedded spaces
**
**  public domain by Bob Stout
*/
#define NUL '\0'

char *nxitrim(char *str)
{
      char *ibuf = str;
      size_t i = 0;

      /*
      **  Trap NULL
      */

      if (str)
      {
            /*
            **  Remove leading spaces (from RMLEAD.C)
            */

            for (ibuf = str; *ibuf && isspace(*ibuf); ++ibuf)
                  ;
	    str = ibuf;

            /*
            **  Remove trailing spaces (from RMTRAIL.C)
            */
	    i = strlen(str);
            while (--i >= 0)
            {
                  if (!isspace(str[i]))
                        break;
            }
            str[++i] = NUL;
      }
      return str;
}
  /*-------------------------------------------------------------------------*/

  NXstatus  NXgetdata (NXhandle fid, void *data)
  {
    int status, type, rank, iDim[NX_MAXRANK];
    char *pPtr, *pPtr2;

    pNexusFunction pFunc = handleToNexusFunc(fid);
    status = pFunc->nxgetinfo(pFunc->pNexusData, &rank, iDim, &type); /* unstripped size if string */
    /* only strip one dimensional strings */
    if ( (type == NX_CHAR) && (pFunc->stripFlag == 1) && (rank == 1) )
    {
	pPtr = (char*)malloc(iDim[0]+5);
        memset(pPtr, 0, iDim[0]+5);
        status = pFunc->nxgetdata(pFunc->pNexusData, pPtr); 
	pPtr2 = nxitrim(pPtr);
	strncpy((char*)data, pPtr2, strlen(pPtr2)); /* not NULL terminated by default */
	free(pPtr);
    }
    else
    {
        status = pFunc->nxgetdata(pFunc->pNexusData, data); 
    }
    return status;
  }
/*---------------------------------------------------------------------------*/
  NXstatus  NXgetrawinfo (NXhandle fid, int *rank, 
				    int dimension[], int *iType)
  {
    int status;

    pNexusFunction pFunc = handleToNexusFunc(fid);
    status = pFunc->nxgetinfo(pFunc->pNexusData, rank, dimension, iType);
    return status;
  }
  /*-------------------------------------------------------------------------*/
 
  NXstatus  NXgetinfo (NXhandle fid, int *rank, 
				    int dimension[], int *iType)
  {
    int status;
    char *pPtr = NULL;

    pNexusFunction pFunc = handleToNexusFunc(fid);
    status = pFunc->nxgetinfo(pFunc->pNexusData, rank, dimension, iType);
    /*
      the length of a string may be trimmed....
    */
    /* only strip one dimensional strings */
    if((*iType == NX_CHAR) && (pFunc->stripFlag == 1) && (*rank == 1)){
      pPtr = (char *)malloc((dimension[0]+1)*sizeof(char));
      if(pPtr != NULL){
	memset(pPtr,0,(dimension[0]+1)*sizeof(char));
	pFunc->nxgetdata(pFunc->pNexusData, pPtr);
	dimension[0] = (int)strlen(nxitrim(pPtr));
	free(pPtr);
      }
    } 
    return status;
  }
  
  /*-------------------------------------------------------------------------*/

  NXstatus  NXgetslab (NXhandle fid, void *data, 
				    int iStart[], int iSize[])
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return pFunc->nxgetslab(pFunc->pNexusData, data, iStart, iSize);
  }
  
  
  /*-------------------------------------------------------------------------*/

  NXstatus  NXgetnextattr (NXhandle fileid, NXname pName,
                                     int *iLength, int *iType)
  {
    pNexusFunction pFunc = handleToNexusFunc(fileid);
    return pFunc->nxgetnextattr(pFunc->pNexusData, pName, iLength, iType);  
  }
 

  /*-------------------------------------------------------------------------*/

  NXstatus  NXgetattr (NXhandle fid, char *name, void *data, int* datalen, int* iType)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return pFunc->nxgetattr(pFunc->pNexusData, name, data, datalen, iType);  
  }


  /*-------------------------------------------------------------------------*/

  NXstatus  NXgetattrinfo (NXhandle fid, int *iN)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return pFunc->nxgetattrinfo(pFunc->pNexusData, iN);  
  }


  /*-------------------------------------------------------------------------*/

  NXstatus  NXgetgroupID (NXhandle fileid, NXlink* sRes)
  {
    pNexusFunction pFunc = handleToNexusFunc(fileid);
    return pFunc->nxgetgroupID(pFunc->pNexusData, sRes);  
  }

  /*-------------------------------------------------------------------------*/

  NXstatus  NXgetgroupinfo (NXhandle fid, int *iN, NXname pName, NXname pClass)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return pFunc->nxgetgroupinfo(pFunc->pNexusData, iN, pName, pClass);  
  }

  
  /*-------------------------------------------------------------------------*/

  NXstatus  NXsameID (NXhandle fileid, NXlink* pFirstID, NXlink* pSecondID)
  {
    pNexusFunction pFunc = handleToNexusFunc(fileid);
    return pFunc->nxsameID(pFunc->pNexusData, pFirstID, pSecondID);  
  }

  /*-------------------------------------------------------------------------*/
  
  NXstatus  NXinitattrdir (NXhandle fid)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return pFunc->nxinitattrdir(pFunc->pNexusData);
  }
  /*-------------------------------------------------------------------------*/
  
  NXstatus  NXsetnumberformat (NXhandle fid, 
					    int type, char *format)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    if(pFunc->nxsetnumberformat != NULL)
    {
      return pFunc->nxsetnumberformat(pFunc->pNexusData,type,format);
    }
    else
    {
      /*
	silently ignore this. Most NeXus file formats do not require
        this
      */
      return NX_OK;
    }
  }
  
  
  /*-------------------------------------------------------------------------*/
 
  NXstatus  NXinitgroupdir (NXhandle fid)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return pFunc->nxinitgroupdir(pFunc->pNexusData);
  }
/*----------------------------------------------------------------------*/
 NXstatus  NXinquirefile(NXhandle handle, char *filename, 
				int filenameBufferLength){
  pFileStack fileStack;
  char *pPtr = NULL;
  int length;

  fileStack = (pFileStack)handle;
  pPtr = peekFilenameOnStack(fileStack);
  if(pPtr != NULL){
    length = (int)strlen(pPtr);
    if(length > filenameBufferLength){
      length = filenameBufferLength -1;
    }
    memset(filename,0,filenameBufferLength);
    memcpy(filename,pPtr, length);
    return NX_OK;
  } else {
    return NX_ERROR;
  }
}
/*------------------------------------------------------------------------*/
NXstatus  NXisexternalgroup(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass, 
			    char *url, int urlLen){
  int status, attStatus, length = 1023, type = NX_CHAR;
  ErrFunc oldError;
  char nxurl[1024];

  pNexusFunction pFunc = handleToNexusFunc(fid);

  status = pFunc->nxopengroup(pFunc->pNexusData, name,nxclass);
  if(status != NX_OK){
    return status;
  }
  oldError = NXMGetError();
  NXIReportError = NXNXNoReport;
  attStatus = NXgetattr(fid,"napimount",nxurl,&length, &type);
  NXIReportError = oldError;
  pFunc->nxclosegroup(pFunc->pNexusData);
  if(attStatus == NX_OK){
    length = (int)strlen(nxurl);
    if(length > urlLen){
      length = urlLen - 1;
    }
    memset(url,0,urlLen);
    memcpy(url,nxurl,length);
    return attStatus;
  } else {
    return NX_ERROR;
  }
}
/*------------------------------------------------------------------------*/
NXstatus  NXlinkexternal(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass, 
			 CONSTCHAR *url){
  int status, type = NX_CHAR, length;
  pNexusFunction pFunc = handleToNexusFunc(fid);

  status = pFunc->nxopengroup(pFunc->pNexusData,name,nxclass);
  if(status != NX_OK){
    return status;
  }
  length = (int)strlen(url);
  status = NXputattr(fid, "napimount",(void *)url,length, type);
  if(status != NX_OK){
    return status;
  }
  pFunc->nxclosegroup(pFunc->pNexusData);
  return NX_OK;
}
/*------------------------------------------------------------------------
  Implementation of NXopenpath 
  --------------------------------------------------------------------------*/
static int isDataSetOpen(NXhandle hfil)
{
  NXlink id;
  
  /*
    This uses the (sensible) feauture that NXgetdataID returns NX_ERROR
    when no dataset is open
  */
  if(NXgetdataID(hfil,&id) == NX_ERROR)
  {
    return 0;
  }
  else 
  {
    return 1;
  }
}
/*----------------------------------------------------------------------*/
static int isRoot(NXhandle hfil)
{
  NXlink id;
  
  /*
    This uses the feauture that NXgetgroupID returns NX_ERROR
    when we are at root level
  */
  if(NXgetgroupID(hfil,&id) == NX_ERROR)
  {
    return 1;
  }
  else 
  {
    return 0;
  }
}
/*--------------------------------------------------------------------
  copies the next path element into element.
  returns a pointer into path beyond the extracted path
  ---------------------------------------------------------------------*/
static char *extractNextPath(char *path, NXname element)
{
  char *pPtr, *pStart;
  size_t length;

  pPtr = path;
  /*
    skip over leading /
  */
  if(*pPtr == '/')
  {
    pPtr++;
  }
  pStart = pPtr;
  
  /*
    find next /
  */
  pPtr = strchr(pStart,'/');
  if(pPtr == NULL)
  {
    /*
      this is the last path element
    */
    strcpy(element,pStart);
    return NULL;
  } else {
    length = pPtr - pStart;
    strncpy(element,pStart,length);
    element[length] = '\0';
  }
  return pPtr + 1;
}
/*-------------------------------------------------------------------*/
static NXstatus gotoRoot(NXhandle hfil)
{
    int status;

    if(isDataSetOpen(hfil))
    {
      status = NXclosedata(hfil);
      if(status == NX_ERROR)
      {
	return status;
      }
    }
    while(!isRoot(hfil))
    {
      status = NXclosegroup(hfil);
      if(status == NX_ERROR)
      {
	return status;
      }
    }
    return NX_OK;
}
/*--------------------------------------------------------------------*/
static int isRelative(char *path)
{
  if(path[0] == '.' && path[1] == '.')
    return 1;
  else
    return 0;
}
/*------------------------------------------------------------------*/
static NXstatus moveOneDown(NXhandle hfil)
{
  if(isDataSetOpen(hfil))
  {
    return NXclosedata(hfil);
  } 
  else
  {
    return NXclosegroup(hfil);
  }
}
/*-------------------------------------------------------------------
  returns a pointer to the remaining path string to move up
  --------------------------------------------------------------------*/
static char *moveDown(NXhandle hfil, char *path, int *code)
{
  int status;
  char *pPtr;

  *code = NX_OK;

  if(path[0] == '/')
  {
    *code = gotoRoot(hfil);
    return path;
  } 
  else 
  {
    pPtr = path;
    while(isRelative(pPtr))
    {
      status = moveOneDown(hfil);
      if(status == NX_ERROR)
      {
	*code = status;
	return pPtr;
      }
      pPtr += 3;
    }
    return pPtr;
  }
} 
/*--------------------------------------------------------------------*/
static NXstatus stepOneUp(NXhandle hfil, char *name)
{
  int datatype;
  NXname name2, xclass;
  char pBueffel[256];  

  /*
    catch the case when we are there: i.e. no further stepping
    necessary. This can happen with paths like ../
  */
  if(strlen(name) < 1)
  {
      return NX_OK;
  }
  
  NXinitgroupdir(hfil);
  while(NXgetnextentry(hfil,name2,xclass,&datatype) != NX_EOD)
  {
    
    if(strcmp(name2,name) == 0)
    {
      if(strcmp(xclass,"SDS") == 0)
      {
	return NXopendata(hfil,name);
      } 
      else
      {
	return NXopengroup(hfil,name,xclass);
      }
    }
  }
  snprintf(pBueffel,255,"ERROR: NXopenpath cannot step into %s",name);
  NXIReportError (NXpData, pBueffel);
  return NX_ERROR;              
}
/*--------------------------------------------------------------------*/
static NXstatus stepOneGroupUp(NXhandle hfil, char *name)
{
  int datatype;
  NXname name2, xclass;
  char pBueffel[256];  

  /*
    catch the case when we are there: i.e. no further stepping
    necessary. This can happen with paths like ../
  */
  if(strlen(name) < 1)
  {
      return NX_OK;
  }
  
  NXinitgroupdir(hfil);
  while(NXgetnextentry(hfil,name2,xclass,&datatype) != NX_EOD)
  {
    
    if(strcmp(name2,name) == 0)
    {
      if(strcmp(xclass,"SDS") == 0)
      {
	return NX_EOD;
      } 
      else
      {
	return NXopengroup(hfil,name,xclass);
      }
    }
  }
  snprintf(pBueffel,255,"ERROR: NXopenpath cannot step into %s",name);
  NXIReportError (NXpData, pBueffel);
  return NX_ERROR;              
}
/*---------------------------------------------------------------------*/
NXstatus  NXopenpath(NXhandle hfil, CONSTCHAR *path)
{
  int status, run = 1;
  NXname pathElement;
  char *pPtr;

  if(hfil == NULL || path == NULL)
  {
    NXIReportError(NXpData,
     "ERROR: NXopendata needs both a file handle and a path string");
    return NX_ERROR;
  }

  pPtr = moveDown(hfil,(char *)path,&status);
  if(status != NX_OK)
  {
    NXIReportError (NXpData, 
		    "ERROR: NXopendata failed to move down in hierarchy");
    return status;
  }

  while(run == 1)
  {
    pPtr = extractNextPath(pPtr, pathElement);
    status = stepOneUp(hfil,pathElement);
    if(status != NX_OK)
    {
      return status;
    }
    if(pPtr == NULL)
    {
      run = 0;
    }
  }
  return NX_OK;
}
/*---------------------------------------------------------------------*/
NXstatus  NXopengrouppath(NXhandle hfil, CONSTCHAR *path)
{
  int status, run = 1;
  NXname pathElement;
  char *pPtr;

  if(hfil == NULL || path == NULL)
  {
    NXIReportError(NXpData,
     "ERROR: NXopendata needs both a file handle and a path string");
    return NX_ERROR;
  }

  pPtr = moveDown(hfil,(char *)path,&status);
  if(status != NX_OK)
  {
    NXIReportError (NXpData, 
		    "ERROR: NXopendata failed to move down in hierarchy");
    return status;
  }

  while(run == 1)
  {
    pPtr = extractNextPath(pPtr, pathElement);
    status = stepOneGroupUp(hfil,pathElement);
    if(status == NX_ERROR)
    {
      return status;
    }
    if(pPtr == NULL || status == NX_EOD)
    {
      run = 0;
    }
  }
  return NX_OK;
}
/*---------------------------------------------------------------------*/
NXstatus NXIprintlink(NXhandle fid, NXlink* link)
{
     pNexusFunction pFunc = handleToNexusFunc(fid);
     return pFunc->nxprintlink(pFunc->pNexusData, link);   
}
/*----------------------------------------------------------------------*/
NXstatus NXgetpath(NXhandle fid, char *path, int pathlen){
  int status;
  pFileStack fileStack = NULL;

  fileStack = (pFileStack)fid;
  status = buildPath(fileStack,path,pathlen);
  if(status != 1){
    return NX_ERROR;
  } 
  return NX_OK;
}

/*--------------------------------------------------------------------
  format NeXus time. Code needed in every NeXus file driver
  ---------------------------------------------------------------------*/
char *NXIformatNeXusTime(){
    time_t timer;
    char* time_buffer = NULL;
    struct tm *time_info;
    const char* time_format;
    long gmt_offset;
#ifdef USE_FTIME
    struct timeb timeb_struct;
#endif 

    time_buffer = (char *)malloc(64*sizeof(char));
    if(!time_buffer){
      NXIReportError(NXpData,"Failed to allocate buffer for time data");
      return NULL;
    }

#ifdef NEED_TZSET
    tzset();
#endif 
    time(&timer);
#ifdef USE_FTIME
    ftime(&timeb_struct);
    gmt_offset = -timeb_struct.timezone * 60;
    if (timeb_struct.dstflag != 0)
    {
        gmt_offset += 3600;
    }
#else
    time_info = gmtime(&timer);
    if (time_info != NULL)
    {
        gmt_offset = (long)difftime(timer, mktime(time_info));
    }
    else
    {
        NXIReportError (NXpData, 
        "Your gmtime() function does not work ... timezone information will be incorrect\n");
        gmt_offset = 0;
    }
#endif 
    time_info = localtime(&timer);
    if (time_info != NULL)
    {
        if (gmt_offset < 0)
        {
            time_format = "%04d-%02d-%02dT%02d:%02d:%02d-%02d:%02d";
        }
        else
        {
            time_format = "%04d-%02d-%02dT%02d:%02d:%02d+%02d:%02d";
        }
        sprintf(time_buffer, time_format,
            1900 + time_info->tm_year,
            1 + time_info->tm_mon,
            time_info->tm_mday,
            time_info->tm_hour,
            time_info->tm_min,
            time_info->tm_sec,
            abs(gmt_offset / 3600),
            abs((gmt_offset % 3600) / 60)
        );
    }
    else
    {
        strcpy(time_buffer, "1970-01-01T00:00:00+00:00");
    }
    return time_buffer;
}
/*----------------------------------------------------------------------
                 F77 - API - Support - Routines
  ----------------------------------------------------------------------*/
  /*
   * We store the whole of the NeXus file in the array - that way
   * we can just pass the array name to C as it will be a valid
   * NXhandle. We could store the NXhandle value in the FORTRAN array
   * instead, but that would mean writing far more wrappers
   */
  NXstatus  NXfopen(char * filename, NXaccess* am, 
                                 NXhandle pHandle)
  {
	NXstatus ret;
 	NXhandle fileid = NULL;
	ret = NXopen(filename, *am, &fileid);
	if (ret == NX_OK)
	{
	  memcpy(pHandle, fileid, getFileStackSize());
	}
	else
	{
	  memset(pHandle, 0, getFileStackSize());
	}
	if (fileid != NULL)
	{
	    free(fileid);
	}
	return ret;
  }
/* 
 * The pHandle from FORTRAN is a pointer to a static FORTRAN
 * array holding the NexusFunction structure. We need to malloc()
 * a temporary copy as NXclose will try to free() this
 */
  NXstatus  NXfclose (NXhandle pHandle)
  {
    NXhandle h;
    NXstatus ret;
    h = (NXhandle)malloc(getFileStackSize());
    memcpy(h, pHandle, getFileStackSize());
    ret = NXclose(&h);		/* does free(h) */
    memset(pHandle, 0, getFileStackSize());
    return ret;
  }
  
/*---------------------------------------------------------------------*/  
  NXstatus  NXfflush(NXhandle pHandle)
  {
    NXhandle h;
    NXstatus ret;
    h = (NXhandle)malloc(getFileStackSize());
    memcpy(h, pHandle, getFileStackSize());
    ret = NXflush(&h);		/* modifies and reallocates h */
    memcpy(pHandle, h, getFileStackSize());
    return ret;
  }
/*----------------------------------------------------------------------*/
  NXstatus  NXfmakedata(NXhandle fid, char *name, int *pDatatype,
		int *pRank, int dimensions[])
  {
    NXstatus ret;
    static char buffer[256];
    int i, *reversed_dimensions;
    reversed_dimensions = (int*)malloc(*pRank * sizeof(int));
    if (reversed_dimensions == NULL)
    {
        sprintf (buffer, 
        "ERROR: Cannot allocate space for array rank of %d in NXfmakedata", 
                *pRank);
        NXIReportError (NXpData, buffer);
	return NX_ERROR;
    }
/*
 * Reverse dimensions array as FORTRAN is column major, C row major
 */
    for(i=0; i < *pRank; i++)
    {
	reversed_dimensions[i] = dimensions[*pRank - i - 1];
    }
    ret = NXmakedata(fid, name, *pDatatype, *pRank, reversed_dimensions);
    free(reversed_dimensions);
    return ret;
  }

/*-----------------------------------------------------------------------*/
  NXstatus  NXfcompmakedata(NXhandle fid, char *name, 
                int *pDatatype,
		int *pRank, int dimensions[],
                int *compression_type, int chunk[])
  {
    NXstatus ret;
    static char buffer[256];
    int i, *reversed_dimensions, *reversed_chunk;
    reversed_dimensions = (int*)malloc(*pRank * sizeof(int));
    reversed_chunk = (int*)malloc(*pRank * sizeof(int));
    if (reversed_dimensions == NULL || reversed_chunk == NULL)
    {
        sprintf (buffer, 
      "ERROR: Cannot allocate space for array rank of %d in NXfcompmakedata", 
         *pRank);
        NXIReportError (NXpData, buffer);
	return NX_ERROR;
    }
/*
 * Reverse dimensions array as FORTRAN is column major, C row major
 */
    for(i=0; i < *pRank; i++)
    {
	reversed_dimensions[i] = dimensions[*pRank - i - 1];
	reversed_chunk[i] = chunk[*pRank - i - 1];
    }
    ret = NXcompmakedata(fid, name, *pDatatype, *pRank, 
        reversed_dimensions,*compression_type, reversed_chunk);
    free(reversed_dimensions);
    free(reversed_chunk);
    return ret;
  }
/*-----------------------------------------------------------------------*/
  NXstatus  NXfcompress(NXhandle fid, int *compr_type)
  { 
      return NXcompress(fid,*compr_type);
  }
/*-----------------------------------------------------------------------*/
  NXstatus  NXfputattr(NXhandle fid, char *name, void *data, 
                                   int *pDatalen, int *pIType)
  {
    return NXputattr(fid, name, data, *pDatalen, *pIType);
  }


  /*
   * implement snprintf when it is not available 
   */
  int nxisnprintf(char* buffer, int len, const char* format, ... )
  {
	  int ret;
	  va_list valist;
	  va_start(valist,format);
	  ret = vsprintf(buffer, format, valist);
	  va_end(valist);
	  return ret;
  }

/*--------------------------------------------------------------------------*/
NXstatus NXfgetpath(NXhandle fid, char *path, int *pathlen)
{
  return NXgetpath(fid,path,*pathlen);
}

const char* NXgetversion()
{
    return NEXUS_VERSION ;
}
