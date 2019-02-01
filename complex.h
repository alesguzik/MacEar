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
 *
 * $Header: complex.h,v 2.1 90/12/17 17:57:21 malcolm Exp $
 *
 * $Log:	complex.h,v $
 * Revision 2.1  90/12/17  17:57:21  malcolm
 * Added flags so that include file would only be included once.
 * 
 * Revision 2.0  89/07/25  18:58:16  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.2  88/10/27  23:12:14  malcolm
 * Moved definition of PI into this file.
 * 
 * Revision 1.1  88/10/23  22:39:10  malcolm
 * Initial revision
 * 
 *
 */

#ifndef	_COMPLEX_H
#define	_COMPLEX_H

#include	<math.h>

typedef struct {
	float real;
	float im;
} complex;

#ifndef PI
#define PI 3.141592653589792434
#endif  /* PI */


extern	double	fmax(), fmin();
extern	double	real(), aimag(), cmag(), cphase(), cmag2();
extern	complex	cadd(), csub(), cmul(), cdiv(), cmplx(), csqrt(), conjugate();
extern	complex	iToPower(), cexp(), cis();

#endif	/* _COMPLEX_H */
