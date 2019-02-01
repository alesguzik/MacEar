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
 * $Header: timer.h,v 2.1 90/12/17 18:06:20 malcolm Exp $
 *
 * $Log:	timer.h,v $
 * Revision 2.1  90/12/17  18:06:20  malcolm
 * Added support for A/UX.
 * 
 * Revision 2.0  89/07/25  18:59:47  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.2  89/04/09  17:04:40  malcolm
 * Changed name of rt subroutine on the Cray to fit new name.
 * 
 * Revision 1.1  88/10/23  22:44:09  malcolm
 * Initial revision
 * 
 *
 */

#define	NTIMERS	20

extern long	timertime[NTIMERS];
extern long	timercount[NTIMERS];
extern unsigned long realtime();

#define starttimer(i)	(timercount[i]++, timertime[i] -= realtime())
#define endtimer(i)	(timertime[i] += realtime())

#ifdef	CRAY
#define	realtime	rtclock
#endif	
