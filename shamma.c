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
 *	Copyright (c) 1990 by Apple Computer, Inc
 *		All Rights Reserved.
 *
 * $Header: shamma.c,v 1.4 90/12/17 18:05:21 malcolm Exp $
 *
 * $Log:	shamma.c,v $
 * Revision 1.4  90/12/17  18:05:21  malcolm
 * Cleanup correlogram code for MacEar release 2.1
 * 
 * Revision 1.3  90/11/06  20:55:00  malcolm
 * Added some debugging code.
 * 
 * Revision 1.2  90/01/28  15:24:04  malcolm
 * WORKING and TESTED version.  As near as I can tell this version of
 * the Shamma correlogram is working as well as can be expected.  All
 * delay values and best frequencies have been verified.
 * 
 * Revision 1.1  90/01/16  14:24:07  malcolm
 * Initial revision
 *
 *
 */

static char     *RCSid = "$Header: shamma.c,v 1.4 90/12/17 18:05:21 malcolm Exp $";

/*
 *	This file calculates correlations of the signals produced by each 
 *	channel of the cochlea.  The output from these routines is called
 *	a correlogram.  This version uses an implementation first suggested
 *	by Shihab A. Shamma in the following article
 *		"Stereausis:  Binaural processing without neural delays,"
 *		by Shihab Shamma, Naiming Shen and Preetham Gopalaswamy,
 *		Journal of the Acousical Society of America, Volume 86,
 *		Number 3, September 1989, pp 989-1006.
 *
 *	There are three types of routines in this file.  They are 
 *		Initialization
 *		Data Transmission
 *		Calculations
 *	The routine InitCorrelation must be called first so that space can
 *	be allocated to save the incoming data to be correlated.  A routine
 *	called SendInputToCorrelation is called for every sample output from
 *	the cochlear model to save the data for later correlation.  Finally
 *	several routines are provided to calculate the correlation.  The 
 *	different routines use algorithms with varying degrees of robustness
 *	and speed.
 */

#include	<stdio.h>
#include	<math.h>
#include	"ear.h"
#include	"complex.h"
#include	"filter.h"

#ifndef	PI
3.141592653589793234
#endif	/* PI */

static int	CorrelationChannels = 0;		/* Number of Channels
							 */

							/* How to access data 
							 * in the output data 
							 * array
							 */
#define	OutputDataArray(Channel, Lag) \
	(ShammaData[(int32)((Lag)*CorrelationChannels + (Channel))])

static	float	*ShammaData;
static	float	NextWeight, LastWeight;
						/*
						 * Storage for interpolated
						 * data.
						 */
static	float	*ExpandedData;			
#define	FACTOR	1

#define	BestFreq(Channel, Lag) \
     (BestFreqArray[(int32)((Lag)*CorrelationChannels + (Channel))])
static	float	*BestFreqArray;

						/*
						 * Storage for the Shamma
						 * transformation.
						 */
#define	Transform(Delay,Freq) \
     (TransformArray[(int32)(Freq) * CorrelationChannels + (Delay)])

static	int	*TransformArray;

/*
 *	InitCorrelation - This routine should be called once to define the
 *	number of channels in the cochlear model and to set the number of 
 *	samples that should be saved.  The resolution of the correlogram
 *	is determined by the number of samples saved.
 */
ShammaInitCorrelation(Channels, Length, Between)
int	Channels, Length, Between;
{
	extern	char	*progname;
	extern	int	UseUltra;
	extern	int	CorrelationStep;
	extern float	EpsilonFromTauFS();
	register int	i, j;

	CorrelogramWidth = CorrelogramHeight = CorrelationChannels = Channels;

	ShammaData = NewFloatArray((int32)Channels*Channels,
					"ShammaInitCorrelation");

	ExpandedData = NewFloatArray((int32)ceil(FACTOR*Channels), 
					"ShammaInitCorrelation");

#ifndef	PLOT3D
	if (UseUltra){
		InitDisplay();
							/* Set both frame 
							 * buffers to blue.
							 */
		SetDisplayValue(80 + 80*256 + 255*256*256);
		UpdateDisplay(ShammaData,0,0,0.0,1.0);
		SetDisplayValue(80 + 80*256 + 255*256*256);
		UpdateDisplay(ShammaData,0,0,0.0,1.0);
	}
#endif	/* PLOT3D */
	
	NextWeight = EpsilonFromTauFS(2.0*Between,1.0);
	LastWeight = 1.0 - NextWeight;

/*	if (TransformCorrelogram) */ {
		ComputeShammaTransform();
/*		ColorShammaTransform();	*/
	}
}

/*
 *	SendInputToCorrelation - This routine is called to pass an array of
 *	data from the cochlear model to the correlation routines.  The input
 *	array represents the firing rate of each channel of the cochlear model.
 *	The routine InitCorrelation should be called first to set the 
 *	parameters of the ear model.
 */
ShammaSendInputToCorrelation(Data)
float	Data[];
{
	register int	lChannel, i;
	register float	t;

#include	"ivdep.h"
	for (i=0;i<CorrelationChannels*FACTOR;i++){
		int	Index;
		float	Point;
		float	LeftFactor, RightFactor;

		Point = (float)i/(float)FACTOR;
		Index = (int)Point;
		RightFactor = Point - Index;
		LeftFactor = 1 - RightFactor;
		ExpandedData[i] = Data[Index]*LeftFactor + 
					Data[Index+1]*RightFactor;
	}

	for (lChannel=0; lChannel<CorrelationChannels; lChannel++){
		int	Middle = CorrelationChannels/2;
#include	"ivdep.h"
		for (i=0; i<=lChannel; i++){
			t = Data[lChannel] * ExpandedData[
						(int)(FACTOR*lChannel-i)];
			OutputDataArray(lChannel-i,lChannel) = 
			    OutputDataArray(lChannel,lChannel-i) = 
				OutputDataArray(lChannel-i,lChannel)*LastWeight+
						t*NextWeight;
		}
	}
}

#define	Out(x,y) (Output[((y)*Lags)+(x)])
#define	Sout(x,y) (SharpOutput[((y)*Lags)+(x)])

SharpenResponse(Output, SharpOutput, Lags)
float	*Output, *SharpOutput;
int	Lags;
{
	register int	x, y;

	for (x=0;x<Lags;x++)
#include	"ivdep.h"
		for (y=0;y<Lags;y++)
			Sout(x,y) = 0.0;

	for (x=1;x<Lags;x++)
#include	"ivdep.h"
		for (y=0;y<Lags-3;y++)
			Sout(x,y) += Out(x-1,y+3);
	for (x=2;x<Lags;x++)
#include	"ivdep.h"
		for (y=0;y<Lags-2;y++)
			Sout(x,y) += (float)1 * Out(x+-2,y+2);
	for (x=0;x<Lags;x++)
#include	"ivdep.h"
		for (y=0;y<Lags-2;y++)
			Sout(x,y) += (float)-3 * Out(x+0,y+2);
	for (x=3;x<Lags;x++)
#include	"ivdep.h"
		for (y=0;y<Lags-1;y++)
			Sout(x,y) += (float)1 * Out(x+-3,y+1);
	for (x=1;x<Lags;x++)
#include	"ivdep.h"
		for (y=0;y<Lags-1;y++)
			Sout(x,y) += (float)-3 * Out(x+-1,y+1);
	for (x=0;x<Lags-1;x++)
#include	"ivdep.h"
		for (y=0;y<Lags-1;y++)
			Sout(x,y) += (float)5 * Out(x+1,y+1);
	for (x=2;x<Lags;x++)
#include	"ivdep.h"
		for (y=0;y<Lags;y++)
			Sout(x,y) += (float)-3 * Out(x+-2,y+0);
	for (x=0;x<Lags;x++)
#include	"ivdep.h"
		for (y=0;y<Lags;y++)
			Sout(x,y) += (float)5 * Out(x+0,y+0);
	for (x=0;x<Lags-2;x++)
#include	"ivdep.h"
		for (y=0;y<Lags;y++)
			Sout(x,y) += (float)-3 * Out(x+2,y+0);
	for (x=1;x<Lags;x++)
#include	"ivdep.h"
		for (y=1;y<Lags;y++)
			Sout(x,y) += (float)5 * Out(x+-1,y-1);
	for (x=0;x<Lags-1;x++)
#include	"ivdep.h"
		for (y=1;y<Lags;y++)
			Sout(x,y) += (float)-3 * Out(x+1,y-1);
	for (x=0;x<Lags-3;x++)
#include	"ivdep.h"
		for (y=1;y<Lags;y++)
			Sout(x,y) += (float)1 * Out(x+3,y-1);
	for (x=0;x<Lags;x++)
#include	"ivdep.h"
		for (y=2;y<Lags;y++)
			Sout(x,y) += (float)-3 * Out(x+0,y-2);
	for (x=0;x<Lags-2;x++)
#include	"ivdep.h"
		for (y=2;y<Lags;y++)
			Sout(x,y) += (float)1 * Out(x+2,y-2);
	for (x=0;x<Lags-1;x++)
#include	"ivdep.h"
		for (y=3;y<Lags;y++)
			Sout(x,y) += (float)1 * Out(x+1,y-3);
}
			 
static	int TransformDebugged = 0;
/*
 *	EarCorrelation - This routine is actually called from the main program
 *	to display the correlations.  
 */
double
ShammaEarCorrelation(Output)
float	*Output;
{
	float	Max;
	extern	int	UseUltra;
	
	if (UseUltra || cfn){
		char	PlotBuffer[BUFSIZ];
		int	i, Channel;

		starttimer(7);

		if (SharpResponse){
			static float	*SharpBuffer = 0;
			
			if (!SharpBuffer)
				SharpBuffer = 
				    NewFloatArray((int32)CorrelationChannels*
							CorrelationChannels,
							"ShammaEarCorrelation");

			if (TransformCorrelogram){
				extern	int	SampleNumber;

				SharpenResponse(ShammaData,SharpBuffer,
						CorrelationChannels);
#ifdef	POKEATSHAMMA
				if (SampleNumber>11000 && !TransformDebugged
						&& UseUltra){
					Max = 0;
					TransformDebugged = 1;
					for (i=0;i<CorrelationChannels*
						CorrelationChannels;i++){
						if (Output[i] > Max)
							Max = Output[i];
					}
					PokeAtCorrelogram(SharpBuffer,Max);
				}
#endif	/* POKEATSHAMMA */
				ResampleCorrelogram(SharpBuffer,Output,
				    TransformArray,
				    CorrelationChannels*CorrelationChannels);
			} else {
				extern int	SampleNumber;
				SharpenResponse(ShammaData,Output,
						CorrelationChannels);
#ifdef	POKEATSHAMMA
				if (SampleNumber>11000 && !TransformDebugged
						&& UseUltra){
					Max = 0;
					TransformDebugged=1;
					for (i=0;i<CorrelationChannels*
						CorrelationChannels;i++){
						if (Output[i] > Max)
							Max = Output[i];
					}
					PokeAtCorrelogram(Output,Max);
				}
#endif	/* POKEATSHAMMA */
			}
		} else {
			extern int	SampleNumber;
			if (TransformCorrelogram)
				ResampleCorrelogram(ShammaData,Output,
				    TransformArray,
				    CorrelationChannels*CorrelationChannels);
			else {
#include	"ivdep.h"
				for (i=0;
				     i<CorrelationChannels*CorrelationChannels;
				     i++)
					Output[i] = ShammaData[i];
			}
#ifdef	POKEATSHAMMA
			if (SampleNumber>11000 && !TransformDebugged
					&& UseUltra){
				Max = 0;
				TransformDebugged=1;
				for (i=0;i<CorrelationChannels*
					CorrelationChannels;i++){
					if (Output[i] > Max)
						Max = Output[i];
				}
				PokeAtCorrelogram(Output,Max);
			}
#endif	/* POKEATSHAMMA */
		}

#ifdef	PLOT3D
#define	TmpFile	"/tmp/correlogram"
#define	PlotCommand "/v/malcolm/bin/plot3d z=/tmp/correlogram xsize=%d ysize=%d xskip=1 zmax=%g -P yp=5 scfac=.5 tl=\"%10.5f\" phi1=-40 | oultraplot -a"
		sprintf(PlotBuffer,PlotCommand,Lags,CorrelationChannels,
		    AgcStage4Target*sqrt(2.0*CorrelationLags)*10*UltraHeadroom,
		    SampleNumber/sample_rate);
	        Output[Lags*(CorrelationChannels-1)] = 
		    AgcStage4Target*sqrt(2.0*CorrelationLags)*10*UltraHeadroom;
		

		picout(TmpFile,Output,Lags*CorrelationChannels);
		printf("%s\n", PlotBuffer);
		i = system(PlotBuffer);
		if (i < 0){
		     fprintf(stderr,"System() failed.  Try setenv NCPUS=1\n");
		     exit(1);
		}
#else	/* !PLOT3D */
		Max = 0;
		for (i=0;i<CorrelationChannels*CorrelationChannels;i++){
			if (Output[i] > Max)
				Max = Output[i];
		}

		if (UseUltra)
			UpdateDisplay(Output,CorrelationChannels,
				CorrelationChannels, 0.0, Max/8);
#endif	/* PLOT3D */
		endtimer(7);
	}
	return(Max/8);
}

/*
 * CalculateBestFreq - Calculate the best frequency for each position
 *	in the Shamma correlogram.  This is done by simplying averaging
 *	the center frequency of the left and right channel.  The output
 *	of this routine is an array called BestFreqArray (or just BestFreq)
 *	that gives the best frequency as a function of the two channel
 *	indices.
 */

CalculateBestFreq()
{
	int	channel, delta, i;
	float	ChannelFreq[FACTOR*MaxN];
	int	Index;
	float	Point, LeftFrac, RightFrac;

	BestFreqArray = NewFloatArray((int32)CorrelationChannels*
						CorrelationChannels,
					"CalculateBestFreq");

#include	"ivdep.h"
	for (i=0;i<FACTOR*CorrelationChannels;i++){
		Point = i/(float)FACTOR;
		Index = Point;
		RightFrac = Point - Index;
		LeftFrac = 1.0 - RightFrac;
		
		ChannelFreq[i] = LeftFrac*EarChannelCF(Index,sample_rate) + 
				RightFrac*EarChannelCF(Index+1,sample_rate);
	}

	for (channel = 0; channel < CorrelationChannels; channel++){
#include	"ivdep.h"
		for (delta = 0; delta <= channel; delta++){
			BestFreq(channel,channel-delta) =
			 BestFreq(channel-delta,channel) =  /* 1000;	*/
			   sqrt(ChannelFreq[(int)floor(channel*(float)FACTOR)]*
			         ChannelFreq[(int)floor(channel*(float)FACTOR - 
							              delta)]);
		}
	}

	picout("bestfreq.pic",BestFreqArray,
				CorrelationChannels*CorrelationChannels);
}

/*
 *	ComputeChannelDelays - Find the delay between any two channels.
 *		First find the delay at each channel as a function of
 *		frequency (We sample the frequency axis at the center
 *		frequency of each filter stage.  There is nothing magical
 *		about these frequencies but they do have the right spacing.)
 *		We convert this delay into sample times (or pixels in the
 *		correlogram.  Note, this delay includes both the frequency
 *		dependent delay due to the filter but also the one sample
 *		delay due to the cascade of filters.  The output of this
 *		half of the routine is stored in the TotalDelayArray and
 *		is indexed by channel number and by frequency index.
 *
 *		Finally, we compute the difference in delay due to two
 *		different channels in the correlogram.  We do this by
 *		getting the delay at the best frequency for this pixel in
 *		the correlogram (for each of the two channels) and using 
 *		bilinear interpolation to get the real value.
 *
 *		The output of this routine is stored in an array called 
 *		DeltaDelayArray (or just DeltaDelay).
 */
#define	TotalDelay(Channel, Freq) \
     (TotalDelayArray[(int32)((Freq)* CorrelationChannels + (Channel))])

static	float	*TotalDelayArray;

#define	DeltaDelay(lChannel, rChannel) \
     (DeltaDelayArray[(int32)((rChannel)*CorrelationChannels + (lChannel))])

static	float	*DeltaDelayArray;

ComputeChannelDelays()
{
	int	channel, freqindex;
	int	lChannel, rChannel, Index;
	float	Freq, LeftFreq, RightFreq, Left, Right;
	float	LeftDelay, RightDelay, Factor;

	TotalDelayArray = NewFloatArray((int32)CorrelationChannels*
							CorrelationChannels,
					"ComputeChannelDelays");

	for (freqindex = 0;freqindex<CorrelationChannels;freqindex++){
		float	Freq, SamplePhaseDelay, ChannelDelay;
		Freq = EarChannelCF(freqindex,sample_rate);
/*		Freq = 1000.0;			/* Keep everything fixed */
		SamplePhaseDelay = 1;		/* In samples */
#include	"ivdep.h"
		for (channel=0;channel<CorrelationChannels;channel++){
						/* 
						 * Need to disregard the
						 * first two channels of
						 * preemphasis.
						 */
			ChannelDelay = 
				cphase(FilterGain(&EarFilterArray[channel+2],
					Freq))/(2*PI*Freq/sample_rate);
/*			ChannelDelay = 0;	   TESTING	*/

						/* Sum up the effect due
						 * to previous channels
						 */
			if (channel == 0)
				TotalDelay(channel,freqindex) = ChannelDelay;
			else
				TotalDelay(channel,freqindex) = ChannelDelay + 
					TotalDelay(channel-1,freqindex) -
					SamplePhaseDelay;
		}
	}

	picout("TotalDelay.pic",TotalDelayArray,
				CorrelationChannels*CorrelationChannels);

	DeltaDelayArray = NewFloatArray((int32)CorrelationChannels*
						CorrelationChannels,
						"DeltaDelayArray");

	for (lChannel=0;lChannel<CorrelationChannels;lChannel++){
#include	"ivdep.h"
		for (rChannel=0;rChannel<=lChannel;rChannel++){
			Freq = BestFreq(lChannel,rChannel);

						/* Now interpolate wrt
						 * to frequency (since we
						 * have an integer channel)
						 *
						 * First find the position in
						 * the TotalDelay array
						 * and then we can do a bilinear
						 * interpolation
						 */

			Index = ChannelIndex(Freq);
			LeftFreq = EarChannelCF(Index,sample_rate);
			RightFreq = EarChannelCF(Index+1,sample_rate);
			if (Freq > LeftFreq || Freq < RightFreq ){
				printf("At (%d,%d) Freq is %g, LeftFreq is %g, RightFreq is %g.\n",
					lChannel,rChannel,Freq,LeftFreq,RightFreq);
			}
			Right = Factor = (Freq-LeftFreq)/(RightFreq-LeftFreq);
			Left = 1-Factor;
			if (Index+1 < CorrelationChannels) {
				LeftDelay = Left*TotalDelay(lChannel,Index) + 
					   Right*TotalDelay(lChannel,Index+1);
				RightDelay = Left*TotalDelay(rChannel,Index) + 
					   Right*TotalDelay(rChannel,Index+1);
			} else {
				LeftDelay = TotalDelay(lChannel,Index);
				RightDelay = TotalDelay(rChannel,Index);
			}

			DeltaDelay(rChannel,lChannel) = 
				DeltaDelay(lChannel,rChannel) = 
				fabs(RightDelay-LeftDelay);
#ifdef	POKEATSHAMMA
			if (lChannel==61 && rChannel==57){
				printf("EarChannelCF[0,160000] is %g.\n",
					EarChannelCF(0,16000));
				printf("EarChannelCF[1,160000] is %g.\n",
					EarChannelCF(1,16000));
				printf("Index is %d.\n", Index);
				printf("BestFreq is %g.\n", Freq);
				printf("TotalDelay(%d,%d) is %g.\n",
				 lChannel,Index,TotalDelay(lChannel,Index));
				printf("TotalDelay(%d,%d) is %g.\n",
				 lChannel,Index+1,TotalDelay(lChannel,Index+1));
				printf("TotalDelay(%d,%d) is %g.\n",
				 rChannel,Index,TotalDelay(rChannel,Index));
				printf("TotalDelay(%d,%d) is %g.\n",
				 rChannel,Index+1,TotalDelay(rChannel,Index+1));
				printf("Left and Right Freq are %g and %g.\n",
					LeftFreq, RightFreq);
				printf("Left and Right Factors are %g, %g.\n",
					Left, Right);
				printf("LeftDelay is %g.\n", LeftDelay);
				printf("RightDelay is %g.\n", RightDelay);
				printf("Filter 0 is:\n");
				PrintFilter(&EarFilterArray[0]);
				printf("Filter 1 is:\n");
				PrintFilter(&EarFilterArray[1]);
				printf("Filter 2 is:\n");
				PrintFilter(&EarFilterArray[2]);
				printf("Filter 3 is:\n");
				PrintFilter(&EarFilterArray[3]);
				printf("Filter 82 is:\n");
				PrintFilter(&EarFilterArray[82]);
			}
#endif	/* POKEATSHAMMA */
		}
	}

	free(TotalDelayArray);			/* Don't need this anymore */
	picout("DeltaDelay.pic",DeltaDelayArray,
			CorrelationChannels*CorrelationChannels);
}

FindBestPixel(Delay,Freq)
float	Delay, Freq;
{
	register int	i;
	int	num, skip;
	static	float *ErrorArray = 0;
	float	s, t;

	if (!ErrorArray)
		ErrorArray = NewFloatArray(CorrelationChannels*
						CorrelationChannels,
						"FindBestPixel");

#include	"ivdep.h"
	for (i=0;i<CorrelationChannels*CorrelationChannels;i++){
		s = DeltaDelayArray[i] - Delay;
		t = BestFreqArray[i] - Freq;
		ErrorArray[i] = sqrt(10000*s*s+t*t);
	}

	num = CorrelationChannels*CorrelationChannels;
	skip = 1;
	i = ISMIN(&num, ErrorArray, &skip) - 1;
	if (ErrorArray[i] > 300)
		return 0;
	return i;
}

ComputeShammaTransform()
{
	int	Freq, Delay;

	CalculateBestFreq();
	ComputeChannelDelays();

	TransformArray = NewIntArray((int32)CorrelationChannels*
					CorrelationChannels,
					"ComputeShammaTransform");
	
	for (Freq=0;Freq < CorrelationChannels;Freq++){
		for (Delay=0;Delay < CorrelationChannels;Delay++){
		    Transform(Delay,Freq) = 
			    FindBestPixel((float)Delay,
			   		 EarChannelCF(Freq,sample_rate));
		}
	}
	picout("transform.pic",TransformArray,
			CorrelationChannels*CorrelationChannels);
}

			 
	
ResampleCorrelogram(Old, New, Transformation, Num)
float	*Old, *New;
int	*Transformation, Num;
{
	register int i;

	Old[0] = 0.0;

#include	"ivdep.h"
	for (i=0;i<Num;i++)
		New[i] = Old[Transformation[i]];
}


TestShammaTransform(){
	int	x = 42, y = 42;
	register int	i;
	float	*OutputBuffer;

	OutputBuffer = NewFloatArray(CorrelationChannels*CorrelationChannels);

	while (!feof(stdin)){
		char	Buffer[512];

		printf("Enter location in the Correlogram domain: ");
		fflush(stdout);
		i = (int)gets(Buffer);
		if (!i)
			continue;

		i = sscanf(Buffer,"%d %d", &x, &y);

		if (i<2 || x < 0 || y < 0){
			printf("i is %d\n",i);
			continue;
		}

#include	"ivdep.h"
		for (i=0;i<CorrelationChannels*CorrelationChannels;i++)
			ShammaData[i] = 0.0;

		printf("At pixel %d,%d in the Correlogram Domain:\n", x, y);
		i = Transform(x,y);
		printf("\tBest pixel in Shamma domain is %d, %d.\n",
			i%CorrelationChannels,i/CorrelationChannels);
		printf("\tBest frequency in Shamma domain is %g.\n",
			BestFreqArray[i]);
		printf("\tBest sample delay in Shamma domain is %g.\n",
			DeltaDelayArray[i]);
	}
	free(OutputBuffer);
#include	"ivdep.h"
	for (i=0;i<CorrelationChannels*CorrelationChannels;i++)
		ShammaData[i] = 0.0;
	printf("Finished testing Shamma Transform.\n");
}

#ifdef	CRAY

ColorShammaTransform(){
	float	Freq, Delay;
	register int	i;
	float	*OutputBuffer;

	OutputBuffer = NewFloatArray(CorrelationChannels*CorrelationChannels);

	while (!feof(stdin)){
		char	Buffer[512];
		int	FreqIndex, DelayIndex, j;

		printf("Enter freq and delay (in ms): ");
		fflush(stdout);
		i = (int)gets(Buffer);
		if (!i)
			continue;

		i = sscanf(Buffer,"%g %g", &Freq, &Delay);

		if (i<2 || Freq < 0 || Delay < 0){
			printf("i is %d\n",i);
			continue;
		}

#include	"ivdep.h"
		for (i=0;i<CorrelationChannels*CorrelationChannels;i++)
			ShammaData[i] = 0.0;

		FreqIndex = ChannelIndex(Freq);
		DelayIndex = Delay/1000*sample_rate;
		printf("FreqIndex is %d, DelayIndex is %d.\n",
			FreqIndex, DelayIndex);
		for (i=FreqIndex;i>=0;i--){
			j = TransformArray[i*CorrelationChannels+DelayIndex];
			ShammaData[j] = 1.;
		}
		for (i=0;i<CorrelationChannels;i++)
			OutputDataArray(i,i) = .5;
/*		ResampleCorrelogram(ShammaData,OutputBuffer,TransformArray,
			CorrelationChannels*CorrelationChannels);	*/
		UpdateDisplay(ShammaData,CorrelationChannels,
			CorrelationChannels,0.0,1.0);
	}
	free(OutputBuffer);
#include	"ivdep.h"
	for (i=0;i<CorrelationChannels*CorrelationChannels;i++)
		ShammaData[i] = 0.0;
	printf("Finished testing Shamma Transform.\n");
}

PokeAtCorrelogram(OutputDataArray,Max)
float	*OutputDataArray, Max;
{
	int MarkedPixelX = 22, MarkedPixelY = 22;

	MarkedPixelX++; MarkedPixelY++;		/* Fortran Offset */
	MRKPXL(&MarkedPixelX, &MarkedPixelY);
	UpdateDisplay(OutputDataArray,CorrelationChannels,
		CorrelationChannels,0.0,Max/8);
	MarkedPixelX--; MarkedPixelY--;		/* Fortran Offset */

	clearerr(stdin);
	while (!feof(stdin)){
		char	Buffer[512];
		int	i;

		printf("Enter x and y location: ");
		fflush(stdout);
		i = (int)gets(Buffer);
		if (!i)
			continue;

		if (Buffer[0] == 'q'){
			break;
		} else if (Buffer[0] == 'h'){
			MarkedPixelX--;
			if (MarkedPixelX <0)
				MarkedPixelX = 0;
		} else if (Buffer[0] == 'l'){
			MarkedPixelX++;
			if (MarkedPixelX >= CorrelationChannels)
				MarkedPixelX = CorrelationChannels-1;
		} else if (Buffer[0] == 'k'){	/* Picture is upside down. */
			MarkedPixelY--;
			if (MarkedPixelY < 0)
				MarkedPixelY = 0;
		} else if (Buffer[0] == 'j'){
			MarkedPixelY++;
			if (MarkedPixelY >= CorrelationChannels)
				MarkedPixelY = CorrelationChannels-1;
		} else {
			int	i;

			i = sscanf(Buffer,"%d %d", &MarkedPixelX, 
				&MarkedPixelY);
			if (i<2 || MarkedPixelX < 0 || MarkedPixelY < 0){
				printf("i is %d\n",i);
				continue;
			}
		}

		MarkedPixelX++; MarkedPixelY++;		/* Fortran Offset */
		MRKPXL(&MarkedPixelX, &MarkedPixelY);
		UpdateDisplay(OutputDataArray,CorrelationChannels,
			CorrelationChannels,0.0,Max/8);
		MarkedPixelX--; MarkedPixelY--;		/* Fortran Offset */
		printf("At %d,%d:\n",MarkedPixelX, MarkedPixelY);
		printf("\tBest Frequency is %g.\n", BestFreq(MarkedPixelX,
							     MarkedPixelY));
		printf("\tDelay is %g samples, %gms or %g cycles.\n",
						DeltaDelay(MarkedPixelX,
							     MarkedPixelY),
						1000*DeltaDelay(MarkedPixelX,
						     MarkedPixelY)/sample_rate,
						DeltaDelay(MarkedPixelX,
								MarkedPixelY)/
						     sample_rate*
						     BestFreq(MarkedPixelX,
							      MarkedPixelY));
		printf("\tValue in correlogram is %g.\n",
			OutputDataArray[MarkedPixelY*CorrelationChannels+
					MarkedPixelX]);
	}
	MarkedPixelX = MarkedPixelY = -1;
	MRKPXL(&MarkedPixelX, &MarkedPixelY);
}

#endif	/* CRAY */

#ifndef	CRAY

ISMIN(n,sx,incx)
int	n, incx;
float	*sx;
{
	int	i, mini = 0;
	float	Min = sx[0];

	for (i=incx;i<n;i += incx){
		if (sx[i] < Min){
			Min = sx[i];
			mini = i;
		}
	}
	return (mini+1);			/* Just like Fortran */
}
			
#endif	/* CRAY */
