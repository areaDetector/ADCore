/* mcaMain */

#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "iocsh.h"
#include <epicsThread.h>

int main(int argc,char *argv[])
{

	if(argc>=2) {    
        iocsh(argv[1]);
        epicsThreadSleep(.2);
    }
    iocsh(NULL);
    return(0);
}
