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
 *	Copyright (c) 1988-1989 by Apple Computer, Inc
 *		All Rights Reserved.
 *
 * $Header: earfilters.c,v 2.1 90/11/06 20:49:14 malcolm Exp $
 *
 * $Log:	earfilters.c,v $
 * Revision 2.1  90/11/06  20:49:14  malcolm
 * Removed all the type (sos or hydro) specific code from this file and
 * moved elsewhere.  Now all that remains are the calculations that
 * are independent of model type.
 * 
 * Revision 2.0  89/07/25  18:58:29  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.4  89/07/21  14:39:58  malcolm
 * Changed decimation filter code so that it creates a filter when the
 * decimation factor is greater than 0 (instead of 1.)  Thus df=1 gives
 * no decimation but do use a low pass decimation filter (with cutoff
 * given by taufactor) and df=0 means no decimation AND no filter.
 * 
 * Revision 1.3  88/11/04  16:57:19  malcolm
 * Took out extra arguments to the EARSTEP call.  Now everything is passwd
 * in external globals.
 * 
 * Revision 1.2  88/10/23  23:08:21  malcolm
 * Made compatible with ANSI C (removed pragma's and endif comments).
 * 
 * Revision 1.1  88/10/23  22:40:21  malcolm
 * Initial revision
 * 
 *
 */

static char	*RCSid = "$Header: earfilters.c,v 2.1 90/11/06 20:49:14 malcolm Exp $";

/*
 *	The routines in this file implement the parts of the ear model cascade.
 *	These functions are used by both the second order section and the
 *	hydrodynamics models.  These are the common routines.
 */

#include	<stdio.h>
#include	<math.h>
#include	"ear.h"

/* SOS - Calculate a bunch of second order sections.
 *	y(n) = a0*x(n) + a1*x(n-1) + a2*x(n-2) - b1*y(n-1) - b2*y(n-2)
 *
 *	Both input and output can be the same vector of numbers.
 */
sos(input, state1, state2, a0, a1, a2, b1, b2, output, n)
float	input[], state1[], state2[], a0[], a1[], a2[], b1[], b2[], output[];
int	n;
{
	register int i;
	register float tempin;

#include	"ivdep.h"
	for (i=0;i<n;i++){
		tempin = input[i];
		output[i] = a0[i] * tempin                     + state1[i];
		state1[i] = a1[i] * tempin - b1[i] * output[i] + state2[i];
		state2[i] = a2[i] * tempin - b2[i] * output[i];
	}
}

hwr(input, output, n)
float input[], output[];
int	n;
{
	register int i;
	register float temp;

#include	"ivdep.h"
	for (i=0;i<n;i++){
		temp = input[i];
		if (temp < 0.0)
			output[i] = 0.0;
		else
			output[i] = temp;
	}
}

difference(input, output, n)
float	input[], output[];
int	n;
{
	register int i;

	for (i=n-1;i>0;i--){
		output[i] = input[i-1] - input[i];
	}
	output[0] = output[1];
}

fos(input, state, output, gain, n)
float	input[], state[], output[], gain;
int	n;
{
	register int i;
	register float OneMinusGain = 1.0 - gain;

#include	"ivdep.h"
	for (i=0;i<n;i++)
		state[i] = output[i] = gain*input[i] + OneMinusGain*state[i];
}

