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
 * $Header: hydesign.c,v 1.2 90/12/17 18:02:49 malcolm Exp $
 *
 * $Log:	hydesign.c,v $
 * Revision 1.2  90/12/17  18:02:49  malcolm
 * Added references to the Patterson correlogram code to the test main program.
 * 
 * Revision 1.1  90/11/06  21:00:12  malcolm
 * Initial revision
 * 
 */

static char	*RCSid = "$Header: hydesign.c,v 1.2 90/12/17 18:02:49 malcolm Exp $";

/*
 *	This file designs the second order filters used to model the cochlea.
 *	Compile with -DMAIN to test these functions.
 */

#include	<stdio.h>
#include	"complex.h"
#include	"filter.h"
#include	"ear.h"

/*
 *	DesignEarFilters - This routine designs all of the filters needed for
 *	the ear model.  Most of the real work is done in the file eardesign.c.
 *	Mostly this routine is responsible for calling the filter design 
 *	functions and filling in the arrays that the actual ear model code
 *	needs to function.
 *
 *	The first stage is the ear filter bank models the outer and middle 
 *	ears and is a simple high pass filter.  The second stage has a zero
 *	at DC to model the conversion of acoustic pressure into basilar
 *	membrane motion, a zero at the Nyquist rate to compensate for the
 *	narrow spacing of channels and finally the first pair of poles from
 *	the cascade of ear filters.
 */

DesignEarFilters(){
	int	i;
	float	cf, prevcf;
	struct filter *fp;

	fp = OuterMiddleEarFilter(sample_rate);
	a2[0] = FilterNumeratorCoeff(fp,0);
	a1[0] = FilterNumeratorCoeff(fp,1);
	a0[0] = FilterNumeratorCoeff(fp,2);
	b2[0] = 0.0;
	b1[0] = 0.0;
	EarFilterArray[0] = *fp;

	fp = EarFrontFilter(sample_rate);
	a2[1] = FilterNumeratorCoeff(fp,0);
	a1[1] = FilterNumeratorCoeff(fp,1);
	a0[1] = FilterNumeratorCoeff(fp,2);
	b2[1] = FilterDenominatorCoeff(fp,0);
	b1[1] = FilterDenominatorCoeff(fp,1);
	EarFilterArray[1] = *fp;

	for (i=2;i<MaxN;i++){
		cf = EarChannelCF(i-1, sample_rate);

		if (cf < 20){
			a0[0] = a1[0] = b1[i] = 0.0;
			EarLength = min(i,EarLength);
		} else {
			float	ot = cf/sample_rate*2*PI;
			float	nomq = 1/(0.15*2.0); /* to agree with agc */
			float	minq = 1/(2.0-ot);
			float	maxdamping = 1/minq;
			float	dampscaling = nomq/(nomq+(minq-0.5));
			b2[i] = 1.0-exp(-ot); /* epsilon for the real pole */
			b1[i] = ot;		/* bb = omega*T */
			a0[i] = dampscaling; /* scale to real poles at .5 */
			a1[i] = (1/nomq)*dampscaling; /* damping, init min */
			a2[i] = maxdamping; /* used to limit a1 */
		}
	}

	if (Debug) {
		FILE *fp;
		fp = fopen("coeffs.list","w");
		if (!fp){
		    fprintf(stderr,
		    "ear: Couldn't open coeffs.list for coefficient list.\n");
		    return;
		}
		for (i=0;i<EarLength;i++){
			fprintf(fp,"((%g %g %g)\n", a2[i], a1[i], a0[i]);
			fprintf(fp," (%g %g %g))\n", b2[i], b1[i], 1.0);
		}
		fclose(fp);
	}
}

/*
 *	MaximumEarCF - The maximum filter center frequency in this model is
 *	limited by the behaviour of the digital filters.  At high frequencies
 *	(relative to the sampling rate) the poles of the filter become real
 *	when the Q isn't very high.
 */
float MaximumEarCF(fs)
float	fs;
{
	return( fs / 5.0 );
}

/*
 *	EarChannelCF - Compute the center frequency of each channel of the
 *	ear filter bank.  We use the EarChannel array to cache the results
 *	so that we don't need to recompute them everytime.
 */
#define	MaxChannels	256
static	float	EarChannelCFResults[MaxChannels];

float EarChannelCF(index, fs)
int	index;
float	fs;
{
	if (index >= 0 && index < MaxChannels && 
	    EarChannelCFResults[index] != 0.0)
		return EarChannelCFResults[index];
	if (index == 0)
		return EarChannelCFResults[index] = MaximumEarCF(fs);
	else {
		float	cf, result;
		cf = EarChannelCF(index-1, fs);
		result = cf - EarStepFactor * 
				sqrt(cf*cf+EarBreakFreq*EarBreakFreq)/EarQ;

		if (result < 1)			/* Arbitrary Lower Limit */
			result = 1;

		if (index >= 0 && index < MaxChannels)
			EarChannelCFResults[index] = result;
		return(result);
	}
}

ChannelIndex(Freq)
float	Freq;
{
	register int	i;

	for (i=0;EarChannelCFResults[i] > 0;i++){
		if (EarChannelCFResults[i] < Freq)
			return i-1;			/* Back up one */
	}

	return i-2;
}

InitParms()
{
	Debug = 0;
	printflag = 1;
	ImpulseInput = 0;
	UseAgc = 1;
	UseCascade = 1;
	UseDifference = 1;
	ComputeFiltered = 0;
	UseUltra = 0;
	CPUs = 4;
	VideoRecord = 0;
	LogDisplay = 0;			/* Stretch Correlation on Log Display */
	ifn = "data.adc";		/* Input File Name */
	ofn = "cochlea.pic";		/* Output File for Cochleagram */
	ffn = NULL;			/* Filtered Output */
	cfn = NULL;			/* Correlogram Directory Name */

	sample_rate = 16000.0;
	MaxSamples = -1;

	AgcStage1Tau = 0.010;
	AgcStage2Tau = 0.020;
	AgcStage4Tau = 0.040;
	AgcStage3Tau = 0.080;
	AgcStage1Target = 0.0022;
	AgcStage2Target = 0.0016;
	AgcStage4Target = 0.0011;
	AgcStage3Target = 0.0008;
	DecimationFactor = 20;
	CorrelationStep = 128;
	CorrelationLags = 256;
	TauFactor = 3.0;
	Normalization = .75;
	UltraHeadroom = 10.0;
	SharpResponse = 1;
	TransformCorrelogram = 1;

	EarBreakFreq = 650.0;
	EarQ = 8.0;
	EarStepFactor = .25;
	EarSharpness = 5.0;
	EarZeroOffset = 1.5;
	EarPreemphCorner = 300.0;
	InputGain = 1e-3;
}

CheckParms()
{
}

#ifdef	MAIN

main(argc, argv)
int	argc;
char	**argv;
{
	int	index;
	float	sampling_rate;
	float	cf;
	double	atof();

	InitParms();

	if (argc < 3){
		printf("syntax: %s index sampling_rate\n",
			argv[0]);
		exit(1);
	}
	index = atoi(argv[1]);
	sampling_rate = atof(argv[2]);

	EarQ = 4;
	EarStepFactor=.125;

	CheckParms();

	cf = EarChannelCF(index, sampling_rate);
	printf("The center frequency of the %d'th filter (%g sampling rate)",
		index, sampling_rate);
	printf(" is %g.\n", cf);

	printf("EpsilonFromTauFS(5.0/cf,%g) is %g.\n",sampling_rate,
		EpsilonFromTauFS(5.0/cf, sampling_rate));

	printf("FirstOrderFromTau(5.0/cf,%g) are ", sampling_rate);
	PrintPolynomial(FirstOrderFromTau(5.0/cf, sampling_rate));
	printf("\n");

	printf("FirstOrderFromCorner(.25*cf,%g) are ", sampling_rate);
	PrintPolynomial(FirstOrderFromCorner(.25*cf, sampling_rate));
	printf("\n");

	printf("SecondOrderFromCenterQ(.25*cf,2,%g) are ", sampling_rate);
	PrintPolynomial(SecondOrderFromCenterQ(.25*cf, 2.0, sampling_rate));
	printf("\n");

	printf("OuterMiddleEarFilter(%g) is: \n",sampling_rate);
	PrintFilter(OuterMiddleEarFilter(sampling_rate));

	printf("EarFrontFilter(%g) is: \n", sampling_rate);
	PrintFilter(EarFrontFilter(sampling_rate));

	printf("EarStageFilter(%g,%g,1.032525595) is: \n", cf, sampling_rate);
	PrintFilter(EarStageFilter(cf, sampling_rate, 1.032525595));
	printf("\tGroup delay of this filter at CF is %g.\n",
		FilterGroupDelay(EarStageFilter(cf,sampling_rate,1.032525595),
				 cf, sampling_rate));
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
		
