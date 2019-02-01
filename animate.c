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
 * $Header: animate.c,v 2.3 90/11/06 20:42:27 malcolm Exp $
 *
 * $Log:	animate.c,v $
 * Revision 2.3  90/11/06  20:42:27  malcolm
 * Incremented SampleNumber.  Changed ifdefs from Cray to ULTRA.
 * 
 * Revision 2.2  90/01/28  15:38:00  malcolm
 * Removed reference to the pitch routines.  (This is now done by saving
 * the correlograms in files and running a seperate program.)
 * 
 * Revision 2.1  89/11/09  23:12:56  malcolm
 * Added calls to the pitch routines.
 * 
 * Revision 2.0.1.1  89/08/10  22:30:05  malcolm
 * Made double buffering an ifdef-able parameter.  Also fixed a few endif's
 * that had tokens at the end.
 * 
 * Revision 2.0  89/07/25  18:58:05  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.6  89/07/21  14:37:45  malcolm
 * Moved the ALIMIT call from the correlate code to UpdateDisplay so that
 * only the display values get clipped.
 * 
 * Revision 1.5  89/07/19  12:36:02  malcolm
 * Made annotation faster by precaching all the numbers.  Also we put up the
 * digits of the time with a fixed pitch to save the kerning calculations.
 * 
 * Revision 1.4  89/03/24  16:06:53  malcolm
 * Added code to annotate the display with a numeric sample number.
 * Also, added safeguard so that normalization didn't get too small.
 * 
 * Revision 1.3  88/12/06  21:10:28  malcolm
 * Cleaned up code and removed extraneous #ifdef's.
 * 
 * Revision 1.2  88/10/23  23:07:43  malcolm
 * Made compatible with ANSI C (removed pragma's and endif comments).
 * 
 * Revision 1.1  88/10/23  22:38:11  malcolm
 * Initial revision
 * 
 *
 */

static char	*RCSid = "$Header: animate.c,v 2.3 90/11/06 20:42:27 malcolm Exp $";

/*
 *	Animate - A simple example showing the use of the Ultra Frame Buffer
 *	on the Cray at Apple.
 *
 *	By
 *		Malcolm Slaney (documentation and cleanup)
 *		Sam Dicker (programming and brains)
 *
 *	This example is made up of five files.  This file (animate.c) generates
 *	a large array of floating point data (a diagonal sine wave across the
 *	screen).  Various parts of this array are displayed on the Ultra Frame 
 *	Buffer as fast as possible to get the effect of animation.
 *
 *	A Fortran routine called ftopix converts the array of data into black
 *	and white pixels on the screen.  
 *	
 *	The Makefile for this example is included along with the include file
 *	ub.h and the directives to the segment loader (segdir)
 */
#include <stdio.h>
#include <math.h>			/* To get the sin defintion */
#include "ub.h"				/* To get the Frame Buffer size */
#include "timer.h"

#ifdef	MAIN

#define	XSIZE 512			/* Size of data array */
#define	YSIZE (2*XSIZE)			/* Size of data array */

float amp[YSIZE][XSIZE];		/* Array to hold test data */

int	SampleNumber = 0;
float	sample_rate = 1.0;

main()
{
    register int i, j;

					/* Create a diagonal sine wave */
    for (j = 0; j < YSIZE; j++)
	for (i = 0; i < XSIZE; i++)
	    amp[j][i] = sin((i+j)*2.0*3.1415926/XSIZE);

    InitDisplay();			/* Allocate incore frame buffers/CPUs */

    for (i = 0; ; i = (i+13)%XSIZE){	/* Loop forever displaying image */
	UpdateDisplay(amp[i],XSIZE,XSIZE,-1.0,1.0);
	SampleNumber++;
    }
}
#endif	/* MAIN */

int	*fb[2];				/* Pointers to incore screen buffers */
int	 buf = 0;			/* Which buffer is being filled(0 or 1*/

/* 
 *	InitDisplay - This routine allocates two full screen buffers that
 *	will hold the pixel data.  In addition it reserves 4 CPUs for later
 *	use by the ftopix routine (if it is compiled with microtasking enabled.)
 */
InitDisplay()
{
#ifdef	ULTRA

    fb[0] = UBAllocate();			/* Get a couple of buffers */
#ifdef	DOUBLEBUFFER
    fb[1] = UBAllocate();
#else
    fb[1] = fb[0];
#endif	/* DOUBLEBUFFER */
#endif	/* ULTRA */
}

/*	
 *	UpdateDisplay - Dump an array of data (amps) with dimensions given 
 *	by (width, height) to the Ultra Frame Buffer.  Min and max are the 
 *	minimum and maximum of the data.
 */

UpdateDisplay(amps, width, height, min, max)
float amps[];
int	width, height;
float	min, max;
{
#ifdef	ULTRA
    char Buffer[512];
    int	i;
    extern	int	SampleNumber;
    extern	float	sample_rate;

#if	SLOW || !DOUBLEBUFFER
    UBBusy(fb[buf]);				/* Wait till buffer done. */
#endif

    if (UBCheck(fb[buf]))
	return;

    i = width*height;
    ALIMIT(amps,&i,&min,&max);

    if (max - min < 1e-20)
	max = min + 1.0;

    SetDisplayValue(80 + 80*256 + 255*256*256);

						/* Convert float to pixels */
    FTOPIX(fb[buf], amps, &width, &height, &min, &max);

    starttimer(17);
    sprintf(Buffer,"%10.5f",SampleNumber/sample_rate);
    AnnotateDisplay(fb[buf],Buffer);
    endtimer(17);


    UBUpdate(fb[buf]);				/* Dump buffer to screen */
    buf = 1-buf;				/* Switch to other buffer */
#endif	/* ULTRA */
}

#ifdef	ULTRA
#include "../ub/text.h"
#include "../ub/DogLib.h"
UD_RColor white = { 1., 1., 1. };
UD_RColor black = { 0., 0., 0. };

struct textcache *CSet[128];
#endif	/* ULTRA */

AnnotateDisplay(ub,string)
int	*ub;
char	*string;
{
#ifdef	ULTRA
	int	ix = 50, iy = 80, iwidth = 1280;
	char	*p;
	float	opac = 1.0;

	for (ix = 700-strlen(string)*20, p = string;*p; p++, ix += 20){
		if (!CSet[*p]){
			char	Buffer[2];
			Buffer[0] = *p;
			Buffer[1] = NULL;
			CSet[*p] = buildtext(Buffer, "Times", 52, .5, 1.0, 
						1., 1280);
		}

	    	MIXTEXT(&ub, &ix, &iy, &black, &opac, CSet[*p], 
				&iwidth);
	}
#endif	/* ULTRA */
}
		

SetDisplayValue(value)
int	value;
{
#ifdef	ULTRA
	register int	i, *p;
	long	pixel;

	pixel = (long)value*((long)(1<<32) + 1);
	p = fb[buf];
#include	"ivdep.h"
	for (i=0;i<UB_HWIDTH*UB_HEIGHT;i++)
		p[i] = pixel;
#endif	/* ULTRA */
}
