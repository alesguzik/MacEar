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
 * $Header: timer.c,v 2.3 91/02/27 14:26:14 malcolm Exp $
 *
 * $Log:	timer.c,v $
 * Revision 2.3  91/02/27  14:26:14  malcolm
 * Added optional timer.names file instead of .timers since some Mac
 * programs don't like leading periods.
 * 
 * Revision 2.2  90/12/17  18:06:08  malcolm
 * Added support for A/UX.
 * 
 * Revision 2.1  90/11/06  20:56:24  malcolm
 * Changed copyright and also added support for NeXT.
 * 
 * Revision 2.0.1.1  89/08/10  22:19:46  malcolm
 * David Mellinger (at CCRMA@Stanford) added NeXT support.
 * 
 * Revision 2.0  89/07/25  18:59:44  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.7  89/07/25  18:52:17  malcolm
 * Added support for the SGI machine.
 * 
 * Revision 1.6  89/07/19  12:50:31  malcolm
 * Changed printfs to use fixed width fields so that maybe output will line
 * up on all machines.
 * 
 * Revision 1.5  89/04/09  17:05:54  malcolm
 * Changed name of rt subroutine to realtime.
 * 
 * Revision 1.4  89/02/27  10:46:12  malcolm
 * Added support for Stellar and Sequent
 * 
 * Revision 1.3  89/02/24  22:52:45  malcolm
 * Added support for Lightspeeds Think C Compiler
 * 
 * Revision 1.2  88/11/04  16:58:28  malcolm
 * Added Macintosh and standard Unix versions of rt.
 * 
 * Revision 1.1  88/10/23  22:43:54  malcolm
 * Initial revision
 * 
 *
 */

static char	*RCSid = "$Header: timer.c,v 2.3 91/02/27 14:26:14 malcolm Exp $";

/*
 *	The following implement an elementary performance analysis tool.  This
 *	was necessary because the tools on the Cray were not able to handle
 *	a program that combined C and Fortran.
 */

#include	<stdio.h>
#include	<ctype.h>
#include	"ear.h"

#if	MPW || THINK_C
#define	HZ 1

unsigned long realtime(){
	unsigned long	time;
	GetDateTime(&time);
	return (time);
}
#endif	/* Macintosh OS */

#if	sun || vax || stellar || sequent || sgi || NeXT || _AUX_SOURCE
#include	<sys/types.h>
#include	<sys/times.h>
#include	<sys/param.h>

unsigned long realtime(){
	struct tms buffer;

	times(&buffer);
	return(buffer.tms_utime + buffer.tms_stime);
}
#endif

#if	sequent || NeXT
#define	HZ	60
#endif

#ifdef	cray
#include	<sys/types.h>
#endif

#ifndef	HZ
#define	HZ	1
#endif


static int	starttime;

long	timertime[NTIMERS];
long	timercount[NTIMERS];

inittimers(){
	int	i;

	for (i=0;i<NTIMERS;i++){
		timertime[i] = 0;
		timercount[i] = 0;
	}

	starttime = realtime();
}

printtimers(){
	int	runtime, i;
	FILE	*fp;
	char	Buffer[BUFSIZ], *p;

	fp = fopen(".timers","r");
	if (!fp)
		fp = fopen("timer.names","r");

	runtime = realtime()-starttime;
	if (fp){
		printf("Total Time      Calls   ms/Call       Percentage   Where\n");
		while (!feof(fp)){
			if (fgets(Buffer, BUFSIZ, fp) == 0)
				continue;
			if (Buffer[0] == '#')
				continue;
			i = atoi(Buffer);
			if (i < 0 || i >= NTIMERS)
				continue;
			if (!timercount[i])
				continue;
			for (p=Buffer;*p;p++)
				if (!isspace(*p))
					break;
			for (;*p;p++)
				if (!isdigit(*p))
					break;
			for (;*p;p++)
				if (!isspace(*p))
					break;
			printf("%7.3gs     %8ld   %7.3g      %7.3g",
				timertime[i]/(float)HZ, timercount[i],
				timertime[i]/(float)HZ/timercount[i]*1000,
				100.0*timertime[i]/(float)runtime);
			printf("     %s", p);		/* To fix LSC bug */
		}
		printf("%7gs\n", runtime/(float)HZ);
		fclose(fp);
	} else {
		printf("Total Time      Calls   ms/Call       Percentage   Where\n");
		for (i=0;i<NTIMERS;i++){
			if (timercount[i] == 0)
				continue;
			printf("%d:	%gs  	%ld	%g  	%g\n",
				i, timertime[i]/(float)HZ, timercount[i],
				timertime[i]/(float)HZ/timercount[i]*1000,
				100.0*timertime[i]/(float)runtime);
		}
		printf("Total	%gs\n", runtime/(float)HZ);
	}
}
