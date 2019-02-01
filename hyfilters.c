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
 * $Header: hyfilters.c,v 1.3 91/02/27 12:36:17 malcolm Exp $
 *
 * $Log:	hyfilters.c,v $
 * Revision 1.3  91/02/27  12:36:17  malcolm
 * A bunch of bug fixes by Dick.  "Final" version.
 * 
 * Revision 1.2  90/12/17  18:03:25  malcolm
 * Added references to the Patterson correlogram code to the test main program.
 * 
 * Revision 1.1  90/11/06  21:00:39  malcolm
 * Initial revision
 * 
 */

static char	*RCSid = "$Header: hyfilters.c,v 1.3 91/02/27 12:36:17 malcolm Exp $";

/*
 *	This file designs the second order filters used to model the cochlea.
 *	Compile with -DMAIN to test these functions.
 */

#include	<stdio.h>
#include	"complex.h"
#include	"filter.h"
#include	"ear.h"

float Sos1State[MaxN], Sos2State[MaxN], FosState[MaxN];
float Agc1State[MaxN+2], Agc2State[MaxN+2], Agc3State[MaxN+2],
	Agc4State[MaxN+2];
float InputState[MaxN], DecimateState1[MaxN], DecimateState2[MaxN];

EARSTEP(input, output)
float	*input;
float	output[];
{
	register int i;
	register float tempin;

	if (UseCascade){
		for (i= EarLength-1; i>0 ; i--)		/* Don't do preemph 
							 * channel
							 */
			InputState[i] = InputState[i-1];
		InputState[0] = *input;			/* Now fill in the 
							 * preemph
							 */
	} else {
#include	"ivdep.h"
		for (i=0;i<EarLength;i++){		/* Copy input to all 
							 * channels
							 */
			InputState[i] = *input;
		}
	}

	tempin = InputState[0];
	InputState[0] = a0[0] * tempin                         + Sos1State[0];
	Sos1State[0] =  a1[0] * tempin - b1[0] * InputState[0] + Sos2State[0];
	Sos2State[0] =  a2[0] * tempin - b2[0] * InputState[0];
	tempin = InputState[1];
	InputState[1] = a0[1] * tempin                         + Sos1State[1];
	Sos1State[1] =  a1[1] * tempin - b1[1] * InputState[1] + Sos2State[1];
	Sos2State[1] =  a2[1] * tempin - b2[1] * InputState[1];

	for (i=2;i<EarLength;i++){
		register float	ytm1, ytm2, aa, bb, diff, inp, out;

		bb = b1[i];
		aa = a1[i];
		ytm1 = Sos1State[i];		/* y(t-1) */
		ytm2 = Sos2State[i];		/* y(t-2) */
		diff = ytm2 - ytm1;
		out = ((InputState[i] - ytm1)*bb + diff*aa) * bb - 
			      ytm2 + ytm1 + ytm1;

		Sos2State[i] = Sos1State[i];
		Sos1State[i] = out;
						/* zeroes */
		inp = 0.25*(out + 2.0*ytm1 + ytm2); 
						/* now do the real pole */
		ytm1 = FosState[i];
						/* b2 is epsilon */
		FosState[i] = InputState[i] = ytm1 + b2[i]*(inp - ytm1);
	}

					/* spacediff filter, not optional: */
	difference(InputState, output, EarLength); 
	if (ComputeFiltered){
		fwrite(output, EarLength, sizeof(InputState[0]), ffp);
	      }
	hwr(output, output, EarLength);


	if (UseAgc ){
		agc(output, Agc1State, AgcEpsilon1, 
		AgcStage1Target, EarLength);
		agc(output, Agc2State, AgcEpsilon2, 
		AgcStage2Target, EarLength);
		agc(output, Agc3State, AgcEpsilon3, 
		AgcStage3Target, EarLength);
		agc(output, Agc4State, AgcEpsilon4, 
		AgcStage4Target, EarLength);
		for (i=2;i<EarLength;i++){
		  register float newa1;
					/* scale by dampscaling */
		  newa1 = a0[i]* 0.2 * (2.0 + 
					Agc1State[i] + Agc2State[i] + 
					Agc3State[i] + Agc4State[i] );
		  /* now limit to maxdamping (guard against huge transient)*/
		  a1[i] = min(newa1,a2[i]);
		}		
	}

					/* 
					 * Optional difference & HWR after 
					 * first HWR 
					 */
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

agc(input, state, epsilon, target, n)
float	input[], state[];
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

						/* now use elements 2 through 
						 * n-1, aligned with input 
						 * rather than off by one
						 */
	state[1] = state[2];			/* Take care of end effects */
	state[n] = state[n-1];			/* Take care of end effects */ 

#include	"ivdep.h"
	for (i=2;i<n;i++){
#ifdef	NOCROSSTALK
		temp[i] = input[i] * EpsOverTarget + 
			      OneMinusEpsOverThree * state[i];
#else	/* CROSSTALK */
		temp[i] = input[i] * EpsOverTarget + 
			      OneMinusEpsOverThree * 
			      (state[i-1]+state[i]+state[i+1]);
#endif	/* NOCROSSTALK */
	}

#include	"ivdep.h"
	for (i=2;i<n;i++){
		register float f;
		
		f = temp[i];
		/* this limit is optional */
		/* if (f > 2.0) f = 2.0; */
		state[i] = f;
	}
}

#ifdef	MAIN

float AgcEpsilon1, AgcEpsilon2, AgcEpsilon3, AgcEpsilon4;
float AgcStage1Target, AgcStage2Target, AgcStage3Target, AgcStage4Target;
float DecimationEpsilon;

float	a0[MaxN], a1[MaxN], a2[MaxN], b1[MaxN], b2[MaxN];
float	state1[MaxN], state2[MaxN];

#define	LENGTH		500
#define	CHANNELS	100

int	UseAgc;
int	ComputeFiltered;
int	DecimationFactor;
int	EarLength;
int	UseCascade;
int	UseDifference;
FILE	*ffp = NULL;

main(){
	register int i, j;
	float	period, input;
	float	Output[MaxN];
	
	InitParms();
	UseCascade = 0;
	UseAgc = 0;
	UseDifference = 0;
	Debug = 1;
	DecimationFactor = 0;

	DesignEarFilters();
	ChangeDecimationParameters();

	fprintf(stderr, "Impulse Response of ear filters (%d x %d).\n",
		CHANNELS, LENGTH);

	for (i=0;i<LENGTH;i++){
		if (i == 0)
			input = 1.0;
		else
			input = 0.0;
		EARSTEP(&input,Output);
		for (j=0;j<CHANNELS;j++)
			printf("%g\n", Output[j]);
		printf("\n");
	}
}

double LickEarCorrelation(){}			/* Just to make loader happy */
LickInitCorrelation(){}
LickSendInputToCorrelation(){}
double ShammaEarCorrelation(){}
ShammaInitCorrelation(){}
ShammaSendInputToCorrelation(){}
double PattersonEarCorrelation(){}
PattersonInitCorrelation(){}
PattersonSendInputToCorrelation(){}

#endif	/* MAIN */
