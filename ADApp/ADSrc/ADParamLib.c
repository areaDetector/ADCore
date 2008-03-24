/* Simple parameter system that can be used for EPICS area detectors

   This is a simple parameter system designed to make parameter storage and 
   callback notification simpler for EPICS area detector drivers. 

   It is based on the parameter library for the EPICS asyn motor drivers
   written by Nick Rees and Peter Denison from DLS.
   
   Mark Rivers
   University of Chicago
   March 24, 2008


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <epicsString.h>
#include "ADParamLib.h"
#include "ADInterface.h"

typedef enum { paramUndef, paramInt, paramDouble, paramString} paramType;

typedef struct
{
    paramType type;
    union
    {
        double dval;
        int    ival;
        char  *sval;
    } data;
} paramVal;

typedef struct paramList
{
    paramIndex startVal;
    paramIndex nvals;
    int nflags;
    int *flags;
    paramVal * vals;
    paramIntCallback   intCallback;
    paramDoubleCallback doubleCallback;
    paramStringCallback  stringCallback;
    void * intParam;
    void * doubleParam;
    void * stringParam;
} paramList;

/*  Deletes a parameter system created by paramCreate.

    Allocates data structures for a parameter system with the given number of
    parameters. Parameters stored in the system are are accessed via an index number
    ranging from 0 to the number of values minus 1.

    params [in]   Pointer to PARAM handle returned by paramCreate.

*/
static void paramDestroy( PARAMS params )
{
    if (params->vals != NULL) free( params->vals );
    if (params->flags != NULL) free( params->flags );
    free( params );
    params = NULL;
}

/*  Creates a parameter system with a given number of values

    Allocates data structures for a parameter system with the given number of
    parameters. Parameters stored in the system are are accessed via and index number
    ranging from 0 to the number of values minus 1.

    startVal  [in]   Index of first parameter to be created.
    nvals     [in]   Number of parameters.

    Handle to be passed to other parameter system routines or NULL if system cannot be created.
*/
static PARAMS paramCreate( paramIndex startVal, paramIndex nvals )
{
    PARAMS params = (PARAMS) calloc( 1, sizeof(paramList ));

    if ((nvals > 0) &&
        (params != NULL) &&
        ((params->vals = (paramVal *) calloc( nvals, sizeof(paramVal))) != NULL ) &&
        ((params->flags = (int *)calloc( nvals, sizeof(int))) != NULL))
    {
        params->startVal = startVal;
        params->nvals = nvals;
        params->nflags = 0;
    }
    else
    {
        paramDestroy( params );
    }
    
    /* Set some reasonable defaults for some parameters */
    ADParam->setInteger(params, ADMinX,         0);
    ADParam->setInteger(params, ADMinY,         0);
    ADParam->setInteger(params, ADBinX,         1);
    ADParam->setInteger(params, ADBinY,         1);
    ADParam->setDouble( params, ADGain,         1.0);
    ADParam->setDouble (params, ADAcquireTime,  1.0);
    ADParam->setInteger(params, ADNumExposures, 1);
    ADParam->setInteger(params, ADNumFrames,    1);
    ADParam->setInteger(params, ADFrameMode,    ADSingleFrame);
    ADParam->setInteger(params, ADFileNumber,   1);
    ADParam->setInteger(params, ADAutoIncrement,1);
    ADParam->setInteger(params, ADAutoSave,     1);
    ADParam->setInteger(params, ADSaveFile,     1);

    return params;
}


/** Sets the flag indicating that a parameter has changed.


    params [in]   Pointer to PARAM handle returned by paramCreate.
    index  [in]   Index number of the parameter.

    returns Integer indicating 0 (PARAM_OK) for success or non-zero for index out of range. 
*/
static int paramSetFlag( PARAMS params, paramIndex index )
{
    int status = PARAM_ERROR;

    index -= params->startVal;
    if (index >= 0 && index < params->nvals)
    {
        int i;
        /* See if we have already set the flag for this parameter */
        for (i=0; i<params->nflags; i++) if (params->flags[i] == index) break;
        /* If not found add a flag */
        if (i == params->nflags) params->flags[params->nflags++] = index;
        status = PARAM_OK;
    }
    return status;
}

/** Sets the value of an integer parameter.

    Sets the value of the parameter associated with a given index to an integer value.

    params [in]   Pointer to PARAM handle returned by paramCreate.
    index  [in]   Index number of the parameter.
    value  [in]   Value to be assigned to the parameter.

    returns Integer indicating 0 (PARAM_OK) for success or non-zero for index out of range. 
*/
static int paramSetInteger( PARAMS params, paramIndex index, int value )
{
    int status = PARAM_ERROR;

    index -= params->startVal;
    if (index >= 0 && index < params->nvals)
    {
        if ( params->vals[index].type != paramInt ||
             params->vals[index].data.ival != value )
        {
            paramSetFlag(params, index);
            params->vals[index].type = paramInt;
            params->vals[index].data.ival = value;
        }
        status = PARAM_OK;
    }
    return status;
}

/** Sets the value of a double parameter.

    Sets the value of the parameter associated with a given index to a double value.

    params [in]   Pointer to PARAM handle returned by paramCreate.
    index  [in]   Index number of the parameter.
    value  [in]   Value to be assigned to the parameter.

    returns: Integer indicating 0 (PARAM_OK) for success or non-zero for index out of range. 
*/
static int paramSetDouble( PARAMS params, paramIndex index, double value )
{
    int status = PARAM_ERROR;

    index -= params->startVal;
    if (index >=0 && index < params->nvals)
    {
        if ( params->vals[index].type != paramDouble ||
             params->vals[index].data.dval != value )
        {
            paramSetFlag(params, index);
            params->vals[index].type = paramDouble;
            params->vals[index].data.dval = value;
        }
        status = PARAM_OK;
    }
    return status;
}

/** Sets the value of a string parameter.

    Sets the value of the parameter associated with a given index to string value.

    params [in]   Pointer to PARAM handle returned by paramCreate.
    index  [in]   Index number of the parameter.
    value  [in]   Value to be assigned to the parameter.

    returns: Integer indicating 0 (PARAM_OK) for success or non-zero for index out of range. 
*/
static int paramSetString( PARAMS params, paramIndex index, char *value )
{
    int status = PARAM_ERROR;

    index -= params->startVal;
    if (index >=0 && index < params->nvals)
    {
        if ( params->vals[index].type != paramString ||
             strcmp(params->vals[index].data.sval, value))
        {
            paramSetFlag(params, index);
            params->vals[index].type = paramString;
            free(params->vals[index].data.sval);
            params->vals[index].data.sval = epicsStrDup(value);
        }
        status = PARAM_OK;
    }
    return status;
}

/** Gets the value of an integer parameter.

    Returns the value of the parameter associated with a given index as an integer value.

    \param params [in]   Pointer to PARAM handle returned by paramCreate.
    \param index  [in]   Index number of the parameter.
    \param value  [out]  Value of the integer parameter .

    \return Integer indicating 0 (PARAM_OK) for success or non-zero for index out of range or wrong type.
*/
static int paramGetInteger( PARAMS params, paramIndex index, int * value )
{
    int status = PARAM_ERROR;

    index -= params->startVal;
    if (index >= 0 && index < params->nvals)
    {
        if (params->vals[index].type == paramInt) {
            *value = params->vals[index].data.ival;
            status = PARAM_OK;
        }
    }

    return status;
}

/** Gets the value of a double parameter.

    Gets the value of the parameter associated with a given index as a double value.

    params [in]   Pointer to PARAM handle returned by paramCreate.
    index  [in]   Index number of the parameter.
    value  [out]  Value of the double parameter.

    returns: Integer indicating 0 (PARAM_OK) for success or non-zero for index out of range or wrong type. 
*/
static int paramGetDouble( PARAMS params, paramIndex index, double * value )
{
    int status = PARAM_OK;

    index -= params->startVal;
    if (index >= 0 && index < params->nvals)
    {
        if (params->vals[index].type == paramDouble) {
            *value = params->vals[index].data.dval;
            status = PARAM_OK;
        }
    }

    return status;
}

/** Gets the value of a string parameter.

    Gets the value of the parameter associated with a given index as a string.

    params [in]   Pointer to PARAM handle returned by paramCreate.
    index  [in]   Index number of the parameter.
    maxChars  [in]   Maximum number of characters.
    value  [out]  Value of the string parameter.

    returns: Integer indicating 0 (PARAM_OK) for success or non-zero for index out of range or wrong type. 
*/
static int paramGetString( PARAMS params, paramIndex index, int maxChars, char * value )
{
    int status = PARAM_OK;

    index -= params->startVal;
    if (index >= 0 && index < params->nvals)
    {
        if (params->vals[index].type == paramString) {
            strncpy(value, params->vals[index].data.sval, maxChars);
            status = PARAM_OK;
        }
    }

    return status;
}

/** Sets a callback routing to call when parameters change

    This sets the value of a routine which is called whenever the user calls paramCallCallback and
    a value in the parameter system has been changed.

    params   [in]   Pointer to PARAM handle returned by paramCreate.
    callback [in]   Index number of the parameter. This must be a routine that
                           takes three parameters and returns void.
                           The first paramet is the pointer passed as the third parameter to this routine.
			   The second is an integer indicating the number of parameters that have changed.
			   The third is an array of parameter indices that indicates the parameters that
			   have changed.
    param    [in]   Pointer to a paramemter to be passed to the callback routine.

    \return Integer indicating 0 (PARAM_OK) for success or non-zero for index out of range. 
*/
static int paramSetIntCallback( PARAMS params, paramIntCallback callback, void * param )
{
    params->intCallback = callback;
    params->intParam = param;

    /* Force a callback on all defined int parameters if the callback changes */
    if ( params->intCallback )
    {
        int i;
        for (i = 0; i < params->nvals; i++)
            if (params->vals[i].type == paramInt) params->flags[params->nflags++] = i;
    }

    return PARAM_OK;
}

static int paramSetDoubleCallback( PARAMS params, paramDoubleCallback callback, void * param )
{
    params->doubleCallback = callback;
    params->doubleParam = param;

    /* Force a callback on all defined double parameters if the callback changes */
    if ( params->doubleCallback )
    {
        int i;
        for (i = 0; i < params->nvals; i++)
            if (params->vals[i].type == paramDouble)  params->flags[params->nflags++] = i;
    }

    return PARAM_OK;
}

static int paramSetStringCallback( PARAMS params, paramStringCallback callback, void * param )
{
    params->stringCallback = callback;
    params->stringParam = param;

    /* Force a callback on all defined string parameters if the callback changes */
    if ( params->stringCallback )
    {
        int i;
        for (i = 0; i < params->nvals; i++)
            if (params->vals[i].type == paramString)  params->flags[params->nflags++] = i;
    }

    return PARAM_OK;
}

/** Calls the callback routines indicating which parameters have changed.

    This routine should be called whenever you have changed a number of parameters and wish
    to notify someone (via the callback routine) that they have changed.

    params   [in]   Pointer to PARAM handle returned by paramCreate.

    returns: void
*/
static void paramCallCallbacks( PARAMS params )
{
    int i, index;
    int command;

    for (i = 0; i < params->nflags; i++)
    {
        index = params->flags[i];
        command = index + params->startVal;
        switch(params->vals[index].type) {
        case paramUndef:
            break;
        case paramInt:
            if (params->intCallback != NULL ) 
                params->intCallback( params->intParam, command, params->vals[index].data.ival );
            break;
        case paramDouble:
            if (params->doubleCallback != NULL ) 
                params->doubleCallback( params->doubleParam, command, params->vals[index].data.dval );
            break;
        case paramString:
            if (params->stringCallback != NULL ) 
                params->stringCallback( params->stringParam, command, params->vals[index].data.sval );
            break;
        }
    }
    params->nflags=0;
}

/*  Prints the current values in the parameter system to stdout

    This routine prints all the values in the parameter system to stdout. 
    If the values are currently undefined, this is noted.

    params   [in]   Pointer to PARAM handle returned by paramCreate.

    returns: void
*/
static void paramDump( PARAMS params )
{
    int i;

    printf( "Number of parameters is: %d\n", params->nvals );
    for (i =0; i < params->nvals; i++)
    {
        switch (params->vals[i].type)
        {
        case paramDouble:
            printf( "Parameter %d is a double, value %f\n", i+ params->startVal, params->vals[i].data.dval );
            break;
        case paramInt:
            printf( "Parameter %d is an integer, value %d\n", i+ params->startVal, params->vals[i].data.ival );
            break;
        case paramString:
            printf( "Parameter %d is a string, value %s\n", i+ params->startVal, params->vals[i].data.sval );
            break;
        default:
            printf( "Parameter %d is undefined\n", i+ params->startVal );
            break;
        }
    }
}


static paramSupport ADParamSupport =
{
  paramCreate,
  paramDestroy,
  paramSetInteger,
  paramSetDouble,
  paramSetString,
  paramCallCallbacks,
  paramGetInteger,
  paramGetDouble,
  paramGetString,
  paramSetIntCallback,
  paramSetDoubleCallback,
  paramSetStringCallback,
  paramDump
};

paramSupport * ADParam = &ADParamSupport;
