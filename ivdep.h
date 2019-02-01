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
 * $Header: ivdep.h,v 2.0 89/07/25 18:58:46 malcolm Exp $
 *
 * $Log:	ivdep.h,v $
 * Revision 2.0  89/07/25  18:58:46  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.2  89/04/09  17:04:13  malcolm
 * Added missing double quote in include file specification.
 * 
 * Revision 1.1  88/10/23  23:08:53  malcolm
 * Initial revision
 * 
 *
 */


/*
 *	Note the following hack is necessary so that pre-ANSI C compilers
 *	won't get upset by the #pragma directive.  Only if the compiler is
 *	capable of handling the directive (go ahead and vectorize the loop
 *	the vectors are independent) is it included.
 */

#if	ANSI || CRAY
#include	"ivdep.cray.h"
#endif	/* ANSI */

