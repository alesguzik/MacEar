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
 * $Header: utilities.c,v 2.4 90/12/17 18:06:54 malcolm Exp $
 *
 * $Log:	utilities.c,v $
 * Revision 2.4  90/12/17  18:06:54  malcolm
 * Cleaned up correlogram code and removed the NewFloatArray and
 * NewIntArray routines to the file alloc.c
 * 
 * Revision 2.3  90/11/06  20:56:44  malcolm
 * Added gain input variable.  (Sets the gain of the file reading routines.)
 * 
 * Revision 2.2  90/01/28  15:35:05  malcolm
 * Added NewFloatArray and NewIntArray routines.  Also added definitions
 * for function pointers so that either the Licklider or the Shamma
 * style of correlograms can be computed.  Finally added the 's' and 't'
 * and the 'cor=' parameter to support the Shamma style of correlogram.
 * 
 * Revision 2.1  89/11/09  23:14:18  malcolm
 * Removed CalculateResponse option.  Added options for stretching (log) the
 * display and correlograms synced to the video rate.
 * 
 * Revision 2.0.1.2  89/08/10  22:24:45  malcolm
 * Dave Mellinger (at Stanford CCRMA) added support for MaxSamples.  Also 
 * ifndef'ed the declaration of atof.  Finally fixed correlogram spelling.
 * 
 * Revision 2.0.1.1  89/07/28  21:32:05  malcolm
 * Fixed a missing definition for atof in the Macintosh environments.
 * 
 * Revision 2.0  89/07/25  18:59:51  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.3  89/06/20  22:45:58  malcolm
 * Added support (int32 type) for LightSpeed C.
 * 
 * Revision 1.2  89/04/09  16:58:01  malcolm
 * Added support for Ultra frame buffer video taping.  Also added support
 * for a decimation factor of zero indicating no decimation and no filtering.
 * 
 * Revision 1.1  89/02/25  12:13:21  malcolm
 * Initial revision
 * 
 *
 */

static char	*RCSid = "$Header: utilities.c,v 2.4 90/12/17 18:06:54 malcolm Exp $";

#include	<stdio.h>
#ifdef	__STDC__
#include	<stdlib.h>
#endif	/* STDC */
#include	"complex.h"
#include	"filter.h"
#include	"ear.h"

int	Debug = 0;
int	printflag = 1;
int	ImpulseInput = 0;
int	UseAgc = 1;
int	UseCascade = 1;
int	UseDifference = 1;
int	ComputeFiltered = 0;
int	UseUltra = 0;
int	CPUs = 4;
int	VideoRecord = 0;
int	LogDisplay = 0;			/* Stretch Correlation on Log Display */
char	*progname;
char	*ifn = "data.adc";		/* Input File Name */
char	*ofn = "cochlea.pic";		/* Output File for Cochleagram */
char	*ffn = NULL;			/* Filtered Output */
char	*cfn = NULL;			/* Correlogram Directory Name */

float	sample_rate = 16000.0;
int32	DataLength;
int32	MaxSamples = -1;

float	AgcStage1Tau = 0.640;
float	AgcStage2Tau = 0.16;
float	AgcStage3Tau = 0.04;
float	AgcStage4Tau = 0.01;
float	AgcStage1Target = 0.0032;
float	AgcStage2Target = 0.0016;
float	AgcStage3Target = 0.0008;
float	AgcStage4Target = 0.0004;
float	InputGain = 0.10;
int	DecimationFactor = 20;
int	CorrelationStep = 128;
int	CorrelationLags = 256;
float	TauFactor = 3.0;
float	Normalization = .75;
float	UltraHeadroom = 1.0;
int	SharpResponse = 1;
int	TransformCorrelogram = 1;

float	EarBreakFreq = 1000.0;
float	EarQ = 8.0;
float	EarStepFactor = .25;
float	EarSharpness = 5.0;
float	EarZeroOffset = 1.5;
float	EarPreemphCorner = 300.0;

float	AgcEpsilon1, AgcEpsilon2, AgcEpsilon3, AgcEpsilon4;
float	a0[MaxN], a1[MaxN], a2[MaxN], b1[MaxN], b2[MaxN];
float	DecimationEpsilon;
struct filter	EarFilterArray[MaxN];

float	Output[MaxN];
int	EarLength = MaxN;
int	SampleNumber = 0;

int	(*SendInputToCorrelation)() = LickSendInputToCorrelation;
int	(*InitCorrelation)() = LickInitCorrelation;
double	(*EarCorrelation)() = LickEarCorrelation;
char	*CorrelogramType = "Licklider's Model";

FILE	*ffp;

char	*ArgumentTable[] = {
		"if",
		"of",
		"tau1",
		"tau2",
		"tau3",
		"tau4",
		"target1",
		"target2",
		"target3",
		"target4",
		"taufactor",
		"df",
		"breakf",
		"earq",
		"stepfactor",
		"sharpness",
		"offset",
		"preemph",
		"ff",
		"cf",
		"cstep",
		"clag",
		"cpus",
		"normalize",
		"umax",
		"maxsamples",
		"cor",
		"gain",
		0
	};

ProcessOption(p)
char	*p;
{
	int	Flag, cc;

	if (*p == '+')
		Flag = 1;
	else
		Flag = 0;

	while(cc= *++p)switch(cc){
	case 'D':
	case 'd':
		if (Flag)
			Debug ++;
		else
			Debug = 0;
		break;
	case 'p':
		printflag = Flag;
		break;
	case 'i':
		ImpulseInput = Flag;
		break;
	case 'a':
		UseAgc = Flag;
		break;
	case 'c':
		UseCascade = Flag;
		break;
	case 'm':
		UseDifference = Flag;
		break;
	case 'u':
		UseUltra = Flag;
		if (Flag)
			DecimationFactor = 1;
		break;
	case 'v':
		VideoRecord = Flag;
		break;
	case 'l':
		LogDisplay = Flag;
		break;
	case 's':
		SharpResponse = Flag;
		break;
	case 't':
		TransformCorrelogram = Flag;
		break;
	default:
		fprintf(stderr,"bad flag: %c\n",cc);
		exit(1);
	}
}

ProcessArgument(p)
char	*p;
{
#ifndef	unix
	float	atof();
#endif	/* unix */
	
	switch(comm(p,ArgumentTable)){
	case 1:			/* if - input file for speech */
		ifn = p+3;
		break;
	case 2:			/* of - output file for cochlea data */
		ofn = p+3;
		if (!*ofn)
			ofn = 0;
		break;
	case 3:			/* tau1 - first agc time constant */
		AgcStage1Tau = atof(p+5);
		break;
	case 4:			/* tau2 - second agc time constant */
		AgcStage2Tau = atof(p+5);
		break;
	case 5:			/* tau3 - third agc time constant */
		AgcStage3Tau = atof(p+5);
		break;
	case 6:			/* tau4 - fourth agc time constant */
		AgcStage4Tau = atof(p+5);
		break;
	case 7: 		/* target1 - first Agc Target */
		AgcStage1Target = atof(p+8);
		break;
	case 8: 		/* target2 - second Agc Target */
		AgcStage2Target = atof(p+8);
		break;
	case 9: 		/* target3 - third Agc Target */
		AgcStage3Target = atof(p+8);
		break;
	case 10: 		/* target4 - fourth Agc Target */
		AgcStage4Target = atof(p+8);
		break;
	case 11:		/* taufactor */
		TauFactor = atof(p+10);
		break;
	case 12:		/* df - Decimation Factor 1 */
		DecimationFactor = atoi(p+3);
		if (DecimationFactor < 0)
			DecimationFactor = 1;
		break;
	case 13:		/* breakf - Ear Break Frequency */
		EarBreakFreq = atof(p+7);
		break;
	case 14:		/* earq */
		EarQ = atof(p+5);
		break;
	case 15:		/* stepfactor */
		EarStepFactor = atof(p+11);
		break;
	case 16:		/* sharpness */
		EarSharpness = atof(p+10);
		break;
	case 17:		/* offset */
		EarZeroOffset = atof(p+7);
		break;
	case 18:		/* preemph */
		EarPreemphCorner = atof(p+8);
		break;
	case 19:		/* fn - Filtered File Name */
		ffn = p+3;
		if (*ffn){
			ComputeFiltered = 1;
		} else {
			ffn = 0;
		}
		break;
	case 20:		/* cf - Correlation File Name */
		cfn = p+3;
		if (!*cfn)
			cfn = 0;
		break;
	case 21:		/* cstep - Steps between correlations */
		CorrelationStep = atoi(p+6);
		break;
	case 22:		/* clag - Total lags in correlations */
		CorrelationLags = atoi(p+5);
		break;
	case 23:		/* cpus - Number of CPUs to use */
		CPUs = atoi(p+5);
		break;
	case 24:		/* Normalize - For Correlation */
		Normalization = atof(p+10);
		break;
	case 25:		/* umax - Ultra Headroom factor */
		UltraHeadroom = atof(p+5);
		break;
	case 26:		/* maxsamples - limit the number of samples */
		MaxSamples = atoi(p+11);
		break;
	case 27:		/* cor - Type of Correlogram to compute */
		if (p[4] == 'l'){
			SendInputToCorrelation = LickSendInputToCorrelation;
			InitCorrelation = LickInitCorrelation;
			EarCorrelation = LickEarCorrelation;
			CorrelogramType = "Licklider's Model";
		} else if (p[4] == 's'){
			SendInputToCorrelation = ShammaSendInputToCorrelation;
			InitCorrelation = ShammaInitCorrelation;
			EarCorrelation = ShammaEarCorrelation;
			CorrelogramType = "Shamma's Model";
			CPUs = 1;
		} else if (p[4] == 'p'){
			SendInputToCorrelation = 
					PattersonSendInputToCorrelation;
			InitCorrelation = PattersonInitCorrelation;
			EarCorrelation = PattersonEarCorrelation;
			CorrelogramType = "Patterson's Model";
			CPUs = 1;
		} else {
			fprintf(stderr,"%s: Unknown correlogram model (%s).\n",
				progname, p+4);
			fprintf(stderr,
			  "\tPlease use either cor=licklider, cor=shamma or cor=patterson.\n");
			exit(1);
		}
		break;
	case 28:		/* gain - Input Gain to System */
		InputGain = atof(p+5);
		break;
	}
}

PrintStats(){
	printf("Lyon Ear Model\n");
	printf("\tInput File (if).....................%s\n", 
		ImpulseInput? "An Impulse" : ifn);
	printf("\tOutput File (of)....................%s\n", ofn?ofn:"None");
	printf("\tFiltered Sound File (ff)............%s\n", ffn?ffn:"None");
	printf("\tCorrelation Output File (cf)........%s\n", cfn?cfn:"None");
	printf("\tInput Gain (gain)...................%g\n", InputGain);
	if (UseAgc){
	 printf("\tAgc Stage 1 Tau (tau1)..............%gs\n", AgcStage1Tau);
	 printf("\tAgc Stage 2 Tau (tau2)..............%gs\n", AgcStage2Tau);
	 printf("\tAgc Stage 3 Tau (tau3)..............%gs\n", AgcStage3Tau);
	 printf("\tAgc Stage 4 Tau (tau4)..............%gs\n", AgcStage4Tau);
	 printf("\tAgc State 1 Target (target1)........%g\n", AgcStage1Target);
	 printf("\tAgc State 2 Target (target2)........%g\n", AgcStage2Target);
	 printf("\tAgc State 3 Target (target3)........%g\n", AgcStage3Target);
	 printf("\tAgc State 4 Target (target4)........%g\n", AgcStage4Target);
	}
	printf("\tDecimation Tau Factor (taufactor)...%g\n", TauFactor);
	printf("\tDecimation Factor (df)..............%d\n", DecimationFactor);
	printf("\tEar Break Frequency (breakf)........%g\n", EarBreakFreq);
	printf("\tEar Q Factor (earq).................%g\n", EarQ);
	printf("\tEar Step Factor (stepfactor)........%g\n", EarStepFactor);
	printf("\tEar Zero Sharpness (sharpness)......%g\n", EarSharpness);
	printf("\tEar Zero Offset (offset)............%g\n", EarZeroOffset);
	printf("\tEar Preemphasis Corner (preemph)....%g\n", EarPreemphCorner);
	if (UseUltra || cfn){
		printf("\tCorrelogram Type....................%s\n",
			CorrelogramType);
		printf("\tCorrelation Time Step (cstep).......%d samples\n", 
			CorrelationStep);
		printf("\tCorrelation Lags (clag).............%d samples\n", 
			CorrelationLags);
		printf("\tCorrelation Normalize (normalize)...%g\n", 
			Normalization);
		printf("\tUltra Headroom factor (umax)........%g\n", 
			UltraHeadroom);
		if (CorrelogramType[0] == 'S' && SharpResponse)
			printf("\tUse an LIN to sharpen response (+s)\n");

	}
	if (MaxSamples > 0)
		printf("\tNumber of samples (maxsamples)......%ld\n",
			MaxSamples);
	if (!UseAgc)
		printf("\tNo AGC (-a)\n");
	if (!UseCascade)
		printf("\tNo cascade (independent channels) (-c)\n");
	if (!UseDifference)
		printf("\tNo difference of channels (-m)\n");
	if (UseUltra)
		printf("\tAnimate correlogram on Ultra (+u)\n");
	if ((UseUltra || cfn) && LogDisplay)
		printf("\tShow correlogram on log delay scale (+l)\n");
	if (UseUltra && VideoRecord)
		printf("\tRecording video at VTR rates (30 frames/sec )(+v)\n");
	if (Debug)
		printf("\tDebug Mode\n");
#ifndef	THINK_C
	fflush(stdout);
#endif	/* !THINK_C */
}

syntax(){}

ChangeAgcParams()
{
	AgcEpsilon1 = EpsilonFromTauFS(AgcStage1Tau, sample_rate);
	AgcEpsilon2 = EpsilonFromTauFS(AgcStage2Tau, sample_rate);
	AgcEpsilon3 = EpsilonFromTauFS(AgcStage3Tau, sample_rate);
	AgcEpsilon4 = EpsilonFromTauFS(AgcStage4Tau, sample_rate);
}


ChangeDecimationParameters(){
	DecimationEpsilon = EpsilonFromTauFS(
		((DecimationFactor<1) ? 1 : DecimationFactor) * 
			TauFactor / sample_rate,
		sample_rate);
}

/* 
 * This is sort of a sleazy hack.  We want C and Fortran to share a common
 * set of global variables.  There is no way to convince the loader to do
 * it so we fake it.  The structure below corresponds exactly to a common
 * block defined in fear.f.  We have Fortran call the routine INITCOM with
 * the first variable in the common block.  Since Fortran is call by address
 * this gives INITCOM the starting location of the common block.  We just
 * use the CommonStruct to give C an easy hook into the rest of the block.
 * Sleazy, but it works.  And this saves us from passing every single one 
 * of the variables through a function call.
 */
struct CommonStruct {
	int	n;
	int	UseAgc, UseDifference, UseCascade, ComputeFiltered;
	int	DecimationFactor;
	float	DecimationEpsilon;
	float	AgcEpsilon1, AgcEpsilon2;
	float	AgcEpsilon3, AgcEpsilon4;
	float	AgcStage1Target, AgcStage2Target;
	float	AgcStage3Target, AgcStage4Target;
	float	a0[MaxN], a1[MaxN], a2[MaxN], b1[MaxN], b2[MaxN];
	};

#define	InitCommon(x)	CBlock->x = x;

INITCOM(CBlock)
struct CommonStruct *CBlock;
{
	register int i;

	CBlock->n = EarLength;
	InitCommon( UseAgc );
	InitCommon( UseDifference );
	InitCommon( UseCascade );
	InitCommon( ComputeFiltered );
	InitCommon( DecimationFactor );
	InitCommon( DecimationEpsilon );
	InitCommon( AgcEpsilon1 );
	InitCommon( AgcEpsilon2 );
	InitCommon( AgcEpsilon3 );
	InitCommon( AgcEpsilon4 );
	InitCommon( AgcStage1Target );
	InitCommon( AgcStage2Target );
	InitCommon( AgcStage3Target );
	InitCommon( AgcStage4Target );

	for (i=0;i<MaxN;i++){
		InitCommon( a0[i] );
		InitCommon( a1[i] );
		InitCommon( a2[i] );
		InitCommon( b1[i] );
		InitCommon( b2[i] );
	}
}

