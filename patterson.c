/*
 *			Lyon's Cochlear Model, The Program
 *	   			   Malcolm Slaney
 *			     Advanced Technology Group
 *				Apple Computer, Inc.
 *				 malcolm@apple.com
 *				   November 1990
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
 *	Copyright (c) 1990 by Apple Computer, Inc
 *		All Rights Reserved.
 *
 * $Header: patterson.c,v 1.1 90/12/17 18:04:20 malcolm Exp $
 *
 * $Log:	patterson.c,v $
 * Revision 1.1  90/12/17  18:04:20  malcolm
 * Initial revision
 * 
 */

static char     *RCSid = "$Header: patterson.c,v 1.1 90/12/17 18:04:20 malcolm Exp $";

/*
 *	This file calculates correlations of the signals produced by each 
 *	channel of the cochlea.  The output from these routines is called
 *	a correlogram.  This version uses an implementation suggested
 *	by Roy Patterson and presented at the Fall 1990 Acoustical Society
 *	of America Meeting in San Diego.
 *
 *	There are three types of routines in this file.  They are 
 *		Initialization
 *		Data Transmission
 *		Calculations
 *	The routine InitCorrelation must be called first so that space can
 *	be allocated to save the incoming data to be correlated.  A routine
 *	called SendInputToCorrelation is called for every sample output from
 *	the cochlear model to save the data for later correlation.   Finally
 *	the routine PattersonEarCorrelation gets the data ready for display.
 */

#include	<stdio.h>
#include	<math.h>
#include	"ear.h"
#include	"complex.h"
#include	"filter.h"

/*
 *	Decay Time Constants - There are two time constants in Roy's
 *	Triggered Temporal Integration correlogram.  The first is the
 *	frame rate (correlogram) decay rate.  This is the rate a single
 *	pixel in the correlogram frame decays to nothing.  The 
 *	PATTERSON_FRAME_DECAY_FACTOR is multiplied by the correlogram
 *	sampling rate (often 1/30Hz) to get the time constant for a
 *	one pole decay.
 *
 *	The threshold decays by a fixed amount every sample time.  The
 *	time constant of this delay is the same for all channels and is
 *	given by PATTERSON_THRESHOLD_TIME.  After each sample the threshold
 *	in each channel days by a factor that is a function of this threshold
 *	time.  It should be set to something on the order of the most common
 *	pitch.  But note, when a new threshold is computed for each channel 
 *	the current maximum is incremented by enough so that at the next 
 *	resonance of this channel we are still above the threshold.
 */

#define	PATTERSON_FRAME_DECAY_FACTOR	1.0
#define	PATTERSON_THRESHOLD_TIME	(1/200.0)

static int	CorrelationChannels;		/* Saved # of channels */

static int	CochleagramFifoIndex = 0;	/* Position in FIFO */

static float	*ChannelLocalMax;		/* Last Maximum seen */
static float	*ChannelThreshold;		/* Current Threshold Value */
static float	*ChannelThresholdCompensation;	/* Factor for new Threshold */
static int	*ChannelDecayTime;		/* Last time decayed */

static float	*CochleagramData;		/* Input FIFO Storage */
static float	*CorrelogramData;		/* Storage for Output */

static float    PattersonFrameDecayEpsilon;
static float	PattersonThresholdDecayEpsilon;

#define	CochleagramDataArray(Channel, Index) \
	(CochleagramData[(int32)(Channel)*CorrelogramWidth + \
	((int32)(CochleagramFifoIndex + (Index))%CorrelogramWidth)])
	
						/* How to access data in
						 * the output data array
						 */
#define	CorrelogramDataArray(Channel, Lag) \
	(Output[(int32)(Channel)*CorrelogramWidth + (Lag)])

						/* OK, let's decay a single
						 * output channel by the value
						 * PattersonDecayEpsilon
						 * for each sample since the
						 * last time we did it.  The
						 * last time we decayed the
						 * channel is stored in the
						 * ChannelDecayTime[i] array.
						 */
#define	DecayCorrelogramChannel(chan) { 				\
	register int	j; 						\
	register float	Factor = pow(PattersonFrameDecayEpsilon,	\
					(float)(SampleNumber-		\
					      ChannelDecayTime[chan])), \
			*fp = &CorrelogramData[chan*CorrelogramWidth];	\
									\
	for (j=0; j<CorrelogramWidth; j++) {				\
		fp[j] *= Factor; 					\
	} 								\
	ChannelDecayTime[chan] = SampleNumber;				\
}

						/*
						 * Add a channel's worth of
						 * data into the correlogram
						 * output array.
						 */

#define	AddInChannel(chan) {						\
	register int	j;						\
	register float *fp = &CorrelogramData[chan*CorrelogramWidth];	\
									\
	for (j=0; j<CorrelogramWidth; j++)				\
		fp[CorrelogramWidth-1-j] += CochleagramDataArray(chan, j);\
}

/*
 *	InitCorrelation - This routine should be called once to define the
 *	number of channels in the cochlear model and to set the number of 
 *	samples that should be saved.  The resolution of the correlogram
 *	is determined by the number of samples saved.
 */
PattersonInitCorrelation(Channels, Length, Between)
int	Channels, Length, Between;
{
	extern	int	UseUltra;
	register int	i;
	int32	PictureSize;

	CorrelogramWidth = Length;
	CorrelogramHeight = Channels;
	CorrelationChannels = Channels;
	PictureSize = CorrelogramWidth * CorrelogramHeight;

	if (Debug)
		printf("CorrelogramWidth is %d, Height is %d.\n",
			CorrelogramWidth, CorrelogramHeight);

	ChannelLocalMax = NewFloatArray((int32)CorrelationChannels,
					"PattersonInitCorrelation");
	ChannelThreshold = NewFloatArray((int32)CorrelationChannels,
					"PattersonInitCorrelation");
	ChannelThresholdCompensation = 
			NewFloatArray((int32)CorrelationChannels,
					"PattersonInitCorrelation");
	ChannelDecayTime = NewIntArray((int32)CorrelationChannels,
					"PattersonInitCorrelation");

	CochleagramData = NewFloatArray((int32)PictureSize,
					"PattersonInitCorrelation");
	CorrelogramData = NewFloatArray((int32)PictureSize,
					"PattersonInitCorrelation");

	if (!ChannelLocalMax || !ChannelThreshold || 
	    !ChannelThresholdCompensation || !ChannelDecayTime ||
	    !CorrelogramData || !CochleagramData){
		extern	char *progname;

		fprintf(stderr, 
			"%s: Can't allocate space for Patterson correlogram.\n",
			progname);
		exit(1);
	}
	
						/* Amount to decay each
						 * pixel in the correlogram
						 * after each sample arrives.
						 */
	PattersonFrameDecayEpsilon = 
		1 - EpsilonFromTauFS(PATTERSON_FRAME_DECAY_FACTOR*Between/
								sample_rate,
					sample_rate);

						/* Amount to decay each 
						 * channels' threshold after
						 * each sample arrives.
						 */
	PattersonThresholdDecayEpsilon = 
		1 - EpsilonFromTauFS(PATTERSON_THRESHOLD_TIME, sample_rate);

	if (Debug) {
		printf("PattersonFrameDecayEpsilon is %g.\n",  
			PattersonFrameDecayEpsilon);
		printf("PattersonThresholdDecayEpsilon is %g.\n",  
			PattersonThresholdDecayEpsilon);
	}


#include	"ivdep.h"
	for (i=0; i<CorrelationChannels; i++){
		ChannelLocalMax[i] = 0;
		ChannelThreshold[i] = 0;
		ChannelDecayTime[i] = 0;
		ChannelThresholdCompensation[i] = 
			pow(1/PattersonThresholdDecayEpsilon,
				sample_rate/EarChannelCF(i+2,sample_rate));
	}

#include	"ivdep.h"
	for (i=0; i<PictureSize;i++){
		CochleagramData[i] = 0.0;
		CorrelogramData[i] = 0.0;
	}

#ifndef	PLOT3D
	if (UseUltra){
		InitDisplay();
							/* Set both frame 
							 * buffers to blue.
							 */
		SetDisplayValue(80 + 80*256 + 255*256*256);
		UpdateDisplay(CorrelogramData,0,0,0.0,1.0);
		SetDisplayValue(80 + 80*256 + 255*256*256);
		UpdateDisplay(CorrelogramData,0,0,0.0,1.0);
	}
#endif	/* PLOT3D */
}

/*
 *	SendInputToCorrelation - This routine is called to pass an array of
 *	data from the cochlear model to the correlation routines.  The input
 *	array represents the firing rate of each channel of the cochlear model.
 *	The routine InitCorrelation should be called first to set the 
 *	parameters of the ear model.
 *
 *	The sequence of events is as follows:
 *		For each channel
 *			Check for Threshold Crossing
 *				This really means
 *				    New Data is bigger than ChannelThreshold
 *				or  New Data is less than ChannelLocalMax
 *				    (this works since ChannelLocalMax is
 *				     equal to zero until the Threshold is
 *				     crossed and then it is set to follow
 *				     the maximum.)
 *				Decay the Current Channel's values
 *				Add in New Channel (to all lags)
 *				Reset Threshold
 */
PattersonSendInputToCorrelation(Data)
float	Data[];
{
	register int	i;
	int	One = 1;
	static FILE	*debugfp = 0;

	for (i=0;i<CorrelationChannels;i++){
		if (Data[i] < ChannelLocalMax[i]){	/* Going down? */
			SystemCursor();
			DecayCorrelogramChannel(i);
			AddInChannel(i);
			ChannelThreshold[i] = ChannelLocalMax[i] *
				ChannelThresholdCompensation[i];
			ChannelLocalMax[i] = 0;
		} else {
			ChannelThreshold[i] *= PattersonThresholdDecayEpsilon;
		}
		if (Data[i] > ChannelThreshold[i])	/* Crossed Threshold? */
			ChannelLocalMax[i] = Data[i];
	}

	VMOV(Data,&One,&CorrelogramHeight,
		CochleagramData+CochleagramFifoIndex,&CorrelogramWidth);

	CochleagramFifoIndex = (CochleagramFifoIndex + 1) % CorrelogramWidth;

	if (Debug) {
		if (!debugfp)
			debugfp = fopen("thresh","w");
	
#define	CHAN	40
		fprintf(debugfp, "%g %g\n", Data[CHAN], ChannelThreshold[CHAN]);
	}
}

CheckCochleagram(place)
char	*place;
{
	int	i;

#ifdef	TEST
	for (i=0; i<CorrelogramWidth*CorrelogramHeight;i++){
		if (CochleagramData[i] > 1.0){
			printf("%s: Found bad cochleagram data %g at sample %d",
				place, CochleagramData[i], SampleNumber);
			printf(", channel %d.\n", i/CorrelogramWidth);
			exit(2);
		}
	}
#endif	/* TEST */
}

double
PattersonEarCorrelation(Output)
float	*Output;
{
	float	Max;
	
	if (UseUltra || cfn){
		register int	i;
		register float	*inp, *outp;
		int	Count = CorrelogramHeight * CorrelogramWidth;

		for (i=0;i<CorrelogramHeight;i++)
			DecayCorrelogramChannel(i);

		Max = .004;

		inp = CorrelogramData;
		outp = Output;
#include	"ivdep.h"
		for (i=0; i<Count; i++)
			*outp++ = *inp++;

/*
		picout("cochlea.pic", CochleagramData, Count);
		picout("correlogram.pic", CorrelogramData, Count);
		printf("SampleNumber is %d.\n", SampleNumber);

		exit(1);
 */

		if (UseUltra)
			UpdateDisplay(Output,CorrelogramWidth,
				CorrelogramHeight, 0.0, Max);
	}
	picout("threshold.pic", ChannelThreshold, 
				CorrelationChannels);
	return(Max);
}

