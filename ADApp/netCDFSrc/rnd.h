/*
 *	Copyright 1996, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: rnd.h,v 1.1 2008-04-18 19:34:35 rivers Exp $ */
#ifndef _RNDUP

/* useful for aligning memory */
#define	_RNDUP(x, unit)  ((((x) + (unit) - 1) / (unit)) \
	* (unit))
#define	_RNDDOWN(x, unit)  ((x) - ((x)%(unit)))

#define M_RND_UNIT	(sizeof(double))
#define	M_RNDUP(x) _RNDUP(x, M_RND_UNIT)
#define	M_RNDDOWN(x)  __RNDDOWN(x, M_RND_UNIT)

#endif
