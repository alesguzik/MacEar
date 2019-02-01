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
 * $Header: sosfilters.c,v 1.2 90/12/17 18:05:55 malcolm Exp $
 *
 * $Log:	sosfilters.c,v $
 * Revision 1.2  90/12/17  18:05:55  malcolm
 * Clean up for MacEar release 2.1
 * 
 * Revision 1.1  90/11/06  20:56:04  malcolm
 * Initial revision
 * 
 *
 */

static char	*RCSid = "$Header: sosfilters.c,v 1.2 90/12/17 18:05:55 malcolm Exp $";

/*
 *	The routines in this file implement the ear model.  A single input
 *	sample is passed to EARSTEP and the processing for all channels of the
 *	the cochlea model are performed.  EARSTEP is written in both C and
 *	Fortran; this explains the un-C-like calling sequence.
 */

#include	<stdio.h>
#include	<math.h>
#include	"ear.h"

float Sos1State[MaxN], Sos2State[MaxN];
float Agc1State[MaxN+2], Agc2State[MaxN+2], Agc3State[MaxN+2],
	Agc4State[MaxN+2];
float InputState[MaxN], DecimateState1[MaxN], DecimateState2[MaxN];


EARSTEP(input, output)
float	*input;
float	output[];

{
	register int i;
	extern	FILE	*ffp;
	extern int	EarLength;
	extern float	AgcEpsilon1, AgcEpsilon2, AgcEpsilon3, AgcEpsilon4;
	extern float	AgcStage1Target, AgcStage2Target;
	extern float	AgcStage3Target, AgcStage4Target;
	extern float	DecimationEpsilon;
	extern float	a0[], a1[], a2[], b1[], b2[];
	extern int	UseAgc, UseDifference, UseCascade, ComputeFiltered;
	extern int	DecimationFactor;

#ifdef	TEST
	if (SampleNumber < 1){
		printf("a0[0]=%g, a1[0]=%g, a2[0]=%g, b1[0]=%g, b2[0]=%g.\n",
			a0[0], a1[0], a2[0], b1[0], b2[0]);
		printf("a0[1]=%g, a1[1]=%g, a2[1]=%g, b1[1]=%g, b2[1]=%g.\n",
			a0[1], a1[1], a2[1], b1[1], b2[1]);
		printf("a0[2]=%g, a1[2]=%g, a2[2]=%g, b1[2]=%g, b2[2]=%g.\n",
			a0[2], a1[2], a2[2], b1[2], b2[2]);
	}
#endif	/* TEST */

	if (UseCascade){
		for (i= EarLength-1; i>0 ; i--)		/* Don't do preemph 
							 * channel
							 */
			InputState[i] = InputState[i-1];
		InputState[0] = *input;			/* Now fill in the 
							 * preemph
							 */
	} else {
		for (i=0;i<EarLength;i++){		/* Copy input to all 
							 * channels
							 */
			InputState[i] = *input;
		}
	}

	sos(InputState, Sos1State, Sos2State, a0, a1, a2, b1, b2, 
		InputState, EarLength);

	if (UseAgc){
		hwr(InputState, output, EarLength);
		agc(output, Agc1State, output, AgcEpsilon1, 
				AgcStage1Target, EarLength);
		agc(output, Agc2State, output, AgcEpsilon2, 
				AgcStage2Target, EarLength);
		agc(output, Agc3State, output, AgcEpsilon3, 
				AgcStage3Target, EarLength);
		agc(output, Agc4State, output, AgcEpsilon4, 
				AgcStage4Target, EarLength);
	} else {
		for (i=0;i<EarLength;i++)
			output[i] = InputState[i];
	}
	if (ComputeFiltered){
		float	Output[MaxN+1];
		if (UseAgc){
			for (i=0;i< EarLength;i++){
				Output[i] = InputState[i] * 
						fabs(1.0 - Agc1State[i+1]) *
						fabs(1.0 - Agc2State[i+1]) *
						fabs(1.0 - Agc3State[i+1]) *
						fabs(1.0 - Agc4State[i+1]);
			}
			fwrite(Output, EarLength, sizeof(Output[0]), ffp);
		} else {
			fwrite(InputState, EarLength, sizeof(InputState[0]), 
				ffp);
		}
	}
	if (UseDifference){
		difference(output, output, EarLength);
		hwr(output, output, EarLength);
	}
	
	if (DecimationFactor > 0) {
		fos(output, DecimateState1, output, DecimationEpsilon, 
			EarLength);
		fos(output, DecimateState2, output, DecimationEpsilon, 
			EarLength);
	}
}

agc(input, state, output, epsilon, target, n)
float	input[], state[], output[];
float	epsilon, target;
int	n;
{
	register int i;
	float	temp[MaxN+2];
#ifdef	NOCROSSTALK
	float	OneMinusEpsOverThree = (1.0 - epsilon);
#else	/* CROSSTALK */
	float	OneMinusEpsOverThree = (1.0 - epsilon)/3.0;
#endif	/* NOCROSSTALK */
	float	EpsOverTarget = epsilon/target;

	state[0] = state[1];			/* Take care of end effects */  
	state[n+1] = state[n];			/* Take care of end effects */  

#include	"ivdep.h"
	for (i=0;i<n;i++){
		output[i] = fabs((1.0 - state[i+1]) * input[i]);
#ifdef	NOCROSSTALK
		temp[i+1] = output[i] * EpsOverTarget + 
			      OneMinusEpsOverThree * state[i+1];
#else	/* CROSSTALK */
		temp[i+1] = output[i] * EpsOverTarget + 
			      OneMinusEpsOverThree * 
			      (state[i]+state[i+1]+state[i+2]);
#endif	/* NOCROSSTALK */
	}

#include	"ivdep.h"
	for (i=1;i<n+1;i++){
		register float f;
		
		f = temp[i];
		if (f > 1.0)
			f = 1.0;
		state[i] = f;
	}
}

#ifdef	MAIN

float AgcEpsilon1, AgcEpsilon2, AgcEpsilon3, AgcEpsilon4;
float AgcStage1Target, AgcStage2Target, AgcStage3Target, AgcStage4Target;
float DecimationEpsilon;

float	a0[MaxN], a1[MaxN], a2[MaxN], b1[MaxN], b2[MaxN];
float	input[MaxN], state1[MaxN], state2[MaxN];

#define	LENGTH		500
#define	CHANNELS	100

float	Output[CHANNELS];
int	UseAgc = 0;
int	ComputeFiltered = 0;
int	DecimationFactor = 20;
int	EarLength;
int	UseCascade = 1;
int	UseDifference = 1;
FILE	*ffp = NULL;

main(){
	register int i, j;
	float	period;
	
	for (period = 10.0, i=0; i < CHANNELS; i++, period++){
		a0[i] = 1.0;
		a1[i] = 0.0;
		a2[i] = 0.0;
		b1[i] = -2.0 * cos(2.0*PI/period);
		b2[i] = 1.0;
		state1[i] = state2[i] = 0.0;
	}

	fprintf(stderr, "Impulse respose (%d x %d)\n", CHANNELS, LENGTH);
	for (i=0;i<LENGTH;i++){
		if (i == 0)
			for (j=0; j<CHANNELS; j++)
				input[j] = 1.0;
		else
			for (j=0; j<CHANNELS; j++)
				input[j] = 0.0;
		sos(input, state1, state2, a0, a1, a2, b1, b2, 
			Output, CHANNELS);
		for (j=0;j<CHANNELS;j++)
			printf("%g\n", Output[j]);
		printf("\n");
	}
}

#endif	/* MAIN */
