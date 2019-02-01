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
 * $Header: fft.c,v 2.3 90/12/17 18:01:40 malcolm Exp $
 *
 * $Log:	fft.c,v $
 * Revision 2.3  90/12/17  18:01:40  malcolm
 * Added PowerOfTwoGreaterThan function and cleaned up the test program.
 * 
 * Revision 2.2  90/11/06  20:50:09  malcolm
 * Changed copyright message.
 * 
 * Revision 2.1  90/01/28  15:38:43  malcolm
 * Changed storage allocation functions to use NewFloatArray().
 * 
 * Revision 2.0  89/07/25  18:58:35  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.4  89/07/19  12:42:40  malcolm
 * Cleaned up line breaks in the code.
 * 
 * Revision 1.3  88/12/06  21:13:40  malcolm
 * Added test code.
 * 
 * Revision 1.2  88/10/23  23:08:28  malcolm
 * Made compatible with ANSI C (removed pragma's and endif comments).
 * 
 * Revision 1.1  88/10/23  22:41:02  malcolm
 * Initial revision
 * 
 *
 */

static char	*RCSid = "$Header: fft.c,v 2.3 90/12/17 18:01:40 malcolm Exp $";

#include	<math.h>
#include	"timer.h"

PowerOfTwoGreaterThan(i)
int	i;
{
	int	j;

	for (j=0;(1<<j) < i;j++)
		;

	return j;
}

#ifndef	CRAY

float *
initfft(size)
int	size;
{
	return((float *) 0);
}

/*
	fft - calculate FFT

	Carl Crawford
	Purdue University
	W. Lafayette, IN. 47907

	Calling Sequence....fft(real,im,m,iopt)
	Where real and im are the real and imaginary
	parts of the input data.  The result is
	returned in place.  M is the log base 2
	of the number of elements in the array.
	Iopt is equal to 0 for the forward
	transform and 1 for the inverse transform.
*/
fft(a,b,m,iopt)
	float	a[];	/* real part of data */
	float	b[];	/* imaginary part of data */
	int	m;	/* size of data = 2**m */
	int	iopt;	/* 0=dft, 1=idft */
{
	int	nv2,nm1,n,le,le1,ip;
	float   pi,pile1,tmp;
	float	ua,ub,wa,wb,ta,tb,*ap,*bp;
	register	i,j,l;

	n = 1<<m;
	if(iopt){
		for(i=0,ap=a,bp=b;i<n;i++){
			*ap++ /= n;
			*bp++ /= -n;
		}
	}
	nv2 = n/2;
	nm1 = n - 1;
	j = 0;
	for(i=0;i<nm1;i++){
		if(i<j){
			ta = a[j];	tb = b[j];
			a[j] = a[i];	b[j] = b[i];
			a[i] = ta;	b[i] = tb;
		}
		l = nv2;
		while(l < (j+1) ){
			j = j - l;
			l = l / 2;
		}
		j = j + l;
	}
	pi = 3.1415926535;
	for(l=1;l<=m;l++){
		le = 1<<l;
		le1 = le>>1;
		ua = 1.0;	ub = 0.0;
		pile1 = pi / le1;
		wa = cos(pile1);	wb = -sin(pile1);
		for(j=0;j<le1;j++){
			for(i=j;i<n;i += le){
				ip = i + le1;
				ta = a[ip] * ua - b[ip] * ub;
				tb = a[ip] * ub + b[ip] * ua;
				a[ip] = a[i] - ta;
				b[ip] = b[i] - tb;
				a[i] += ta;
				b[i] += tb;
			}
			ua = (tmp = ua) * wa - ub * wb;
			ub = tmp * wb + ub * wa;
		}
	}
						/* For the inverse transform
						 * scale the result by N
						 */
	if(iopt != 0){
		for(i=0;i<n;i++)
			b[i] = -b[i];
	}
}

#else CRAY


static	LastSize = 0;
static	float	*LastWork = 0, *LastInput = 0;

float	*initfft(Size)
int	Size;
{
	int	Init;
	float	*NewFloatArray();

	if (Size > LastSize){
		if (LastWork)
			free(LastWork);
		LastWork = NewFloatArray((long)3*Size*2,"initfft");

		if (LastInput)
			free(LastInput);
		LastInput = NewFloatArray((long)Size*2,"initfft");

		LastSize = Size;
		Init = 1;
		printf("Initing the FFT for size %d.\n", Size);
		CFFT2(&Init, &Init, &Size, LastInput, LastWork, LastInput);
	}

	return(LastWork);
}


fft(a,b,m,iopt)
	float	a[];	/* real part of data */
	float	b[];	/* imaginary part of data */
	int	m;	/* size of data = 2**m */
	int	iopt;	/* 0=dft, 1=idft */
{
	int	Size, Init, i;

	Size = 1<<m;
	initfft(Size);

	if (iopt){
#include	"ivdep.h"
		for (i=0;i<Size;i++) {
			LastInput[2*i] = a[i]/Size;
			LastInput[2*i+1] = b[i]/Size;
		}
	} else {
#include	"ivdep.h"
		for (i=0;i<Size;i++) {
			LastInput[2*i] = a[i];
			LastInput[2*i+1] = b[i];
		}
	}

	if (iopt)
		iopt = -1;
	else
		iopt = 1;

	Init = 0;
	starttimer(5);
	CFFT2(&Init, &iopt, &Size, LastInput, LastWork, LastInput);
	endtimer(5);

#include	"ivdep.h"
	for (i=0;i<Size;i++) {
		a[i] = LastInput[2*i];
		b[i] = LastInput[2*i+1];
	}
}
#endif

#ifdef	MAIN

#define	N	16
#define	logN	4

float	a[N], b[N];
char	*progname = "FFT Test Program";

main(){
	int	i;

	a[1] = 1.0;
	a[N-1] = 1.0;

	printf("First the input data (real, imaginary)\n");
	for (i=0;i<N;i++)
		printf("%d:	%g	%g\n", i, a[i], b[i]);
	fft(a,b,logN,1);

	printf("Here's the FFT of it (real, imaginary)\n");
	for (i=0;i<N;i++)
		printf("%d:	%g	%g\n", i, a[i], b[i]);

	fft(a,b,logN,0);
	printf("Finally, here's the inverse FFT (hould  be same as first)\n");
	for (i=0;i<N;i++)
		printf("%d:	%g	%g\n", i, a[i], b[i]);
}
#endif	/* MAIN */
