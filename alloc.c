/*
 *			Lyon's Cochlear Model, The Program
 *	   			   Malcolm Slaney
 *			     Advanced Technology Group
 *				Apple Computer, Inc.
 *				 malcolm@apple.com
 *				   November 1988
 *
 *	This program implements a model of acoustic propagation and detection
 *	in the human cochlea.  This model was first described by Richard F.
 *	Lyon.  Please see 
 *		Malcolm Slaney, "Lyon's Cochlear Model, the Mathematica 
 *		Notebook," Apple Technical Report #13, 1988
 *	for more information.  This report is available from the Apple 
 *	Corporate Library.
 *
 *	Warranty Information
 *	Even though Apple has reviewed this software, Apple makes no warranty
 *	or representation, either express or implied, with respect to this
 *	software, its quality, accuracy, merchantability, or fitness for a 
 *	particular purpose.  As a result, this software is provided "as is,"
 *	and you, its user, are assuming the entire risk as to its quality
 *	and accuracy.
 *
 *	Copyright (c) 1988-1990 by Apple Computer, Inc
 *		All Rights Reserved.
 *
 * $Header: alloc.c,v 1.1 90/12/17 17:56:21 malcolm Exp $
 *
 * $Log:	alloc.c,v $
 * Revision 1.1  90/12/17  17:56:21  malcolm
 * Initial revision
 * 
 *
 */

static char	*RCSid = "$Header: alloc.c,v 1.1 90/12/17 17:56:21 malcolm Exp $";

#include	<stdio.h>
#ifdef	__STDC__
#include	<stdlib.h>
#endif	/* STDC */

#include	"ear.h"

float	*
NewFloatArray(size,usage)
int32	size;
char	*usage;
{
	float	*p;
	extern char *progname;

	p = (float *)calloc(sizeof(*p),size);
	if (!p){
		fprintf(stderr,"%s: Can't allocate %ld floats for %s.\n", 
				progname, size, usage);
		exit(1);
	}
	return p;
}

int	*
NewIntArray(size,usage)
int32	size;
char	*usage;
{
	int	*p;
	extern char *progname;

	p = (int *)calloc(sizeof(*p),size);
	if (!p){
		fprintf(stderr,"%s: Can't allocate %ld floats for %s.\n", 
			progname, size, usage);
		exit(1);
	}
	return p;
}

