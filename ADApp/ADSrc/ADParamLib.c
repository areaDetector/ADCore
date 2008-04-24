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

#include <asynStandardInterfaces.h>

#include "ADParamLib.h"

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
    void * intParam;
    void * doubleParam;
    void * stringParam;
    asynStandardInterfaces *pasynInterfaces;
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
    pasynInterfaces [in]   Pointer to asynStandardInterfaces structure for callbacks

    returns: Handle to be passed to other parameter system routines or NULL if system cannot be created.
*/
static PARAMS paramCreate( paramIndex startVal, paramIndex nvals, asynStandardInterfaces *pasynInterfaces )
{
    PARAMS params = (PARAMS) calloc( 1, sizeof(paramList ));

    if ((nvals > 0) &&
        (params != NULL) &&
        (pasynInterfaces != NULL) &&
        ((params->vals = (paramVal *) calloc( nvals, sizeof(paramVal))) != NULL ) &&
        ((params->flags = (int *)calloc( nvals, sizeof(int))) != NULL))
    {
        params->startVal = startVal;
        params->nvals = nvals;
        params->nflags = 0;
        params->pasynInterfaces = pasynInterfaces;
    }
    else
    {
        paramDestroy( params );
    }
    
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
static int paramSetString( PARAMS params, paramIndex index, const char *value )
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
    *value = 0;
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
    *value = 0.;
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
    value[0]=0;
    if (index >= 0 && index < params->nvals)
    {
        if (params->vals[index].type == paramString) {
            strncpy(value, params->vals[index].data.sval, maxChars);
            status = PARAM_OK;
        }
    }

    return status;
}

static int intCallback( PARAMS params, int command, int addr, int value)
{
    ELLLIST *pclientList;
    interruptNode *pnode;
    asynStandardInterfaces *pInterfaces = params->pasynInterfaces;
    int address;

    /* Pass int32 interrupts */
    if (!pInterfaces->int32InterruptPvt) return(PARAM_ERROR);
    pasynManager->interruptStart(pInterfaces->int32InterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynInt32Interrupt *pInterrupt = pnode->drvPvt;
        pasynManager->getAddr(pInterrupt->pasynUser, &address);
        if ((command == pInterrupt->pasynUser->reason) &&
            (address == addr)) {
            pInterrupt->callback(pInterrupt->userPvt, 
                                 pInterrupt->pasynUser,
                                 value);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(pInterfaces->int32InterruptPvt);
    return(PARAM_OK);
}

static int doubleCallback( PARAMS params, int command, int addr, double value)
{
    ELLLIST *pclientList;
    interruptNode *pnode;
    asynStandardInterfaces *pInterfaces = params->pasynInterfaces;
    int address;

    /* Pass float64 interrupts */
    if (!pInterfaces->float64InterruptPvt) return(PARAM_ERROR);
    pasynManager->interruptStart(pInterfaces->float64InterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynFloat64Interrupt *pInterrupt = pnode->drvPvt;
        pasynManager->getAddr(pInterrupt->pasynUser, &address);
        if ((command == pInterrupt->pasynUser->reason) &&
            (address == addr)) {
            pInterrupt->callback(pInterrupt->userPvt, 
                                 pInterrupt->pasynUser,
                                 value);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(pInterfaces->float64InterruptPvt);
    return(PARAM_OK);
}

static int stringCallback( PARAMS params, int command, int addr, char *value)
{
    ELLLIST *pclientList;
    interruptNode *pnode;
    asynStandardInterfaces *pInterfaces = params->pasynInterfaces;
    int address;

    /* Pass octet interrupts */
    if (!pInterfaces->octetInterruptPvt) return(PARAM_ERROR);
    pasynManager->interruptStart(pInterfaces->octetInterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynOctetInterrupt *pInterrupt = pnode->drvPvt;
        pasynManager->getAddr(pInterrupt->pasynUser, &address);
        if ((command == pInterrupt->pasynUser->reason) &&
            (address == addr)) {
            pInterrupt->callback(pInterrupt->userPvt, 
                                 pInterrupt->pasynUser,
                                 value, strlen(value), ASYN_EOM_END);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(pInterfaces->octetInterruptPvt);
    return(PARAM_OK);
}


/** Calls the callback routines indicating which parameters have changed.

    This routine should be called whenever you have changed a number of parameters and wish
    to notify someone (via the asyn callback routines) that they have changed.

    params   [in]   Pointer to PARAM handle returned by paramCreate.

    returns: void
*/
static int paramCallCallbacksAddr( PARAMS params, int addr )
{
    int i, index;
    int command;
    int status = PARAM_OK;

    for (i = 0; i < params->nflags; i++)
    {
        index = params->flags[i];
        command = index + params->startVal;
        switch(params->vals[index].type) {
        case paramUndef:
            break;
        case paramInt:
                status |= intCallback( params, command, addr, params->vals[index].data.ival );
            break;
        case paramDouble:
                status |= doubleCallback( params, command, addr, params->vals[index].data.dval );
            break;
        case paramString:
                status |= stringCallback( params, command, addr, params->vals[index].data.sval );
            break;
        }
    }
    params->nflags=0;
    return(status);
}

static int paramCallCallbacks( PARAMS params )
{
    return(paramCallCallbacksAddr(params, 0));
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
  paramGetInteger,
  paramGetDouble,
  paramGetString,
  paramCallCallbacks,
  paramCallCallbacksAddr,
  paramDump
};

paramSupport * ADParam = &ADParamSupport;
