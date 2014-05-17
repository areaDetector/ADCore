/* parseAreaPrefixes.c
 *
 * \author John Hammonds
 *
 * \author Argonne National Laboratory
 *
 * \date May 20, 2010
 *
 * Parse through asyn records, find the ones that contain an ADType info field.  Concatenate a list of channel prefixes for these
 * and write this to a waveform record.
 */
#include <stdio.h>
#include <string.h>
#include <iocsh.h>
#include <dbAccess.h>
#include <dbStaticLib.h>
#include "epicsExport.h"

void parseAreaPrefixes( char *waveformName) {
    int status;
    const char *functionName = "parseAreaPrefixes";
    char inBuffer[128];
    char outBuffer[2048];
    char waveformNameFull[128];
    char *indexPtr;
    int nFound;
    size_t foundLen;
    DBADDR dbaddr;
    DBADDR *pdbaddr = &dbaddr;
    DBENTRY dbentry;
    DBENTRY *pdbentry = &dbentry;
    if (!pdbbase) {
        printf("%s: No database loaded\n", functionName);
        return;
    }
    status = 0;
    nFound = 0;
    outBuffer[0] = '\0';
    dbInitEntry(pdbbase, pdbentry);
    status = dbFindRecordType(pdbentry, "asyn");
    while (!status) {
        status=dbFirstRecord(pdbentry);
        while (!status) {
            strcpy(inBuffer, dbGetRecordName(pdbentry));
            status = dbFindInfo(pdbentry, "ADType");
            if (!status) {
                if (nFound != 0){
                    strcat( outBuffer, " ");
                }
                indexPtr = strstr(inBuffer, "AsynIO");
                foundLen = indexPtr - inBuffer;
                strncat( outBuffer, inBuffer, foundLen);
                nFound++;
            }
            status = dbNextRecord(pdbentry);
        }
    }
    dbFinishEntry(pdbentry);

    /** get the channel for the waveform */

    sprintf ( waveformNameFull, "%s", waveformName);
    status = dbNameToAddr( waveformName, pdbaddr);
    if ( status == 0 ) {
        dbPutField(pdbaddr, DBR_UCHAR, outBuffer, (long)strlen(outBuffer));
    }
    else {
        printf("Cannot find channel %s\n", waveformName);
    }

}

/* information needed by iocsh */
static const iocshArg parseArg0 = {"waveformName", iocshArgString};
static const iocshArg *parseArgs[] = { &parseArg0 };
static const iocshFuncDef parseFuncDef = {"parseAreaPrefixes", 1, parseArgs};

/* Wrapper called by iocsh, select the argumentTypes that parseAreaFixes needs */
static void parseCallFunc(const iocshArgBuf * args) {
    parseAreaPrefixes( args[0].sval);
}

/*Registration routine, runs at startup */
static void parseRegister(void){
    iocshRegister(&parseFuncDef, parseCallFunc);
}

epicsExportRegistrar(parseRegister);
