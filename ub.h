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
 * $Header: ub.h,v 2.1 90/12/17 18:06:43 malcolm Exp $
 *
 * $Log:	ub.h,v $
 * Revision 2.1  90/12/17  18:06:43  malcolm
 * Cleanup for MacEar release 2.1
 * 
 * Revision 2.0  89/07/25  18:59:49  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.1  88/10/23  22:44:17  malcolm
 * Initial revision
 * 
 *
 */

/*	#define	NTSC	0 */

#ifdef	NTSC
#define UB_WIDTH 	752
#define UB_HEIGHT 	484
#else	/* !NTSC */
#define UB_WIDTH 	1280
#define UB_HEIGHT 	1024
#endif	/* NTSC */

#define UB_HWIDTH 	(UB_WIDTH/2)
#define SIGDISPLAY	56
#define SIGRECORD	57

int	*UBAllocate();
int	*CurrentUltraBuffer();
