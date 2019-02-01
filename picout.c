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
 * $Header: picout.c,v 2.3 90/12/17 18:04:51 malcolm Exp $
 *
 * $Log:	picout.c,v $
 * Revision 2.3  90/12/17  18:04:51  malcolm
 * Added include of fcntl.h for ThinkC and changed some max's to Max to
 * keep some preprocessors happy.
 * 
 * Revision 2.2  90/11/06  20:53:51  malcolm
 * Added check to make sure the max and min are not equal.
 * 
 * Revision 2.1  89/11/09  23:08:53  malcolm
 * Fixed some unused variables.
 * 
 * Revision 2.0.1.1  89/08/14  11:10:28  malcolm
 * Fixed problem with 16 bit ints (!!$@#$!#$) for LightSpeed when writing
 * binary data with write() system call.
 * 
 * Revision 2.0  89/07/25  18:58:53  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.5  89/07/19  12:49:53  malcolm
 * Changed name of preprocessor flags that control output mode.
 * 
 * Revision 1.4  89/06/20  22:47:52  malcolm
 * Added support for LightSpeed C by changing a bunch of ints to int32.
 * Also added output routine called BytePicout so that byte files could
 * be quantized and output.
 * 
 * Revision 1.3  88/12/06  21:15:19  malcolm
 * Added support (compile time option) for ascii output.
 * 
 * Revision 1.2  88/11/04  16:58:08  malcolm
 * Added missing endif for MAIN.
 * 
 * Revision 1.1  88/10/23  22:43:24  malcolm
 * Initial revision
 * 
 *
 */

static char	*RCSid = "$Header: picout.c,v 2.3 90/12/17 18:04:51 malcolm Exp $";

/*
 *	Debugging file output.
 */

#include	<stdio.h>
#include	<fcntl.h>
#include	"ear.h"

#ifdef	THINK_C
#include	<unix.h>
#endif	/* THINK_C */

picout(name,pic,num)
char	*name;
float	*pic;
int32	num;
{
	int	output;

#ifndef	TEXTOUTPUT
#ifdef	THINK_C
	if ((output = creat(name,O_WRONLY+O_CREAT+O_TRUNC+O_BINARY)) >= 0){
		num *= sizeof(*pic);
		while (num > 0){
			int	writenum;

			writenum = num>32000 ? 32000 : num;
			num -= writenum;
			if (write(output,(char *)pic,(unsigned int)writenum)
					!= writenum){
				extern int	errno;
				printf(
				 "Error %d in the write of %ld bytes to %s.\n",
					errno, (long)num, name);
				exit(0);
			}
		}
		close(output);
	}
#else
	if ((output = creat(name,0644)) >= 0){
		num *= sizeof(*pic);
		if (write(output,pic,(unsigned int)num) != num){
			extern int	errno;
			printf("Error %d in the write of %ld bytes to %s.\n", 
				errno, (long)num, name);
			exit(0);
		}
		close(output);
	} 
#endif	/* THINK_C */
#else	/* !TEXTOUTPUT */
	FILE	*fp;
	
	if ((fp = fopen(name,"w")) != NULL){
		int	i;
		for (i=0;i<num;i++){
			fprintf(fp,"%g\t",pic[i]);
			if (i%4 == 3)
				fprintf(fp,"\n");
		}
		fprintf(fp,"\n");
		fclose(fp);
	}
#endif	/* TEXTOUTPUT */
	else {
		fprintf(stderr, "Couldn't open %s for creating picture output.\n",
				name);
		exit(1);
	}
}

BytePicout(name, pic, num, Min, Max)
char	*name;
float	*pic;
int32	num;
float	Min, Max;
{
	FILE	*fp;
	register int	ch;
	register int32	i;
	register float	gain;
	
	if (Max-Min == 0)
		gain = 0;
	else
		gain = 255.0/(Max-Min);
	
	if (fp = fopen(name,"w")){
		for (i=0;i<num;i++,pic++){
			ch = (*pic - Min)*gain;
			if (ch < 0)
				putc(0,fp);
			else if (ch > 254)
				putc(255,fp);
			else
				putc(ch,fp);
		}
		fclose(fp);
	}
	else {
		fprintf(stderr, "Couldn't open %s for creating picture output.\n",
				name);
		exit(1);
	}
}
	
	
	
	
#ifdef	MAIN

#include	<math.h>
float	a[64][64];

main(){
	int	i, j;

	for (i=0;i<64;i++)
		for (j=0;j<64;j++){
			float	radius;
			radius = sqrt((float)i*i+j*j);
			a[i][j] = sin(radius/4.0)*(1-radius/128.0);
		}
	picout("sin.pic",a,64*64);
}

#endif	/* MAIN */
