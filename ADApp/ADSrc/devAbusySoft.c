/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* devBoSoft.c */
/* base/src/dev devBoSoft.c,v 1.10 2003/04/01 21:02:17 mrk Exp */

/* devBoSoft.c - Device Support Routines for  Soft Binary Output*/
/*
 *      Original Author: Bob Dalesio
 *      Current Author:  Marty Kraimer
 *      Date:            6-1-90
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "abusyRecord.h"
#include "epicsExport.h"

static long init_record();

/* Create the dset for devBoSoft */
static long write_bo();

struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	write_bo;
}devAbusySoft={
	5,
	NULL,
	NULL,
	init_record,
	NULL,
	write_bo
};
epicsExportAddress(dset,devAbusySoft);

static long init_record(abusyRecord *pbo)
{
 
   long status=0;
 
    /* dont convert */
   status=2;
   return status;
 
} /* end init_record() */

static long write_bo(abusyRecord *pbo)
{
    long status;

    status = dbPutLink(&pbo->out,DBR_USHORT,&pbo->val,1);

    return(status);
}
