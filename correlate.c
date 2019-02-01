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
 * $Header: correlate.c,v 2.4 90/12/17 17:57:40 malcolm Exp $
 *
 * $Log:	correlate.c,v $
 * Revision 2.4  90/12/17  17:57:40  malcolm
 * Big cleanup for the version 2.1 MacEar release.
 * 
 * Revision 2.3  90/11/06  20:43:59  malcolm
 * Added definition of ULTRA.  Also added UltraHeadroom factor to the
 * calculation of the maximum output from the correlation.
 * 
 * Revision 2.2  90/01/28  15:31:32  malcolm
 * Fixed spelling of correlogram.  Also changed all externally called 
 * routines so that Lick(lider) was part of the name.  This was done so
 * that the Shamma and the Licklider routines have similar names and 
 * calling protocols.  Also incorporated calls to NewFloatArray routine.
 * 
 * Revision 2.1  89/11/09  23:08:05  malcolm
 * Added log (stretched) display option.  Cleaned up some code.
 * 
 * Revision 2.0.1.1  89/07/28  21:36:09  malcolm
 * Fixed the way that the maximum correlation output was computed.  Also 
 * added this calculation to the FFT, Simple and Hartley Correlation
 * routines.  Changed the EarCorrelation routine so it would return the
 * calculated maximum.  Added a few calls to SystemCursor so that other
 * programs on the Macintosh could get some CPU time.
 * 
 * Revision 2.0  89/07/25  18:58:17  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.7  89/07/21  14:38:41  malcolm
 * Cleaned up lots of code.  Changed check for zero (when looking for bad
 * data in correlations) to check for less than 1e-10.  Also changed call
 * to fracpower to call to the power function so that everything agreed
 * with the Fortran world.
 * 
 * Revision 1.6  89/07/19  12:37:37  malcolm
 * Added some timers to fine tune the correlation pieces.
 * Also added code so that Plot3d could be called on the output of each
 * step when making ultra animation.
 * 
 * Revision 1.5  89/06/20  22:46:43  malcolm
 * Added support for LightSpeed C.  This meant changing lots of ints to
 * be int32.  Also added routine to stretch the correlogram display.  This
 * allows the time (or pitch) axis to be logarithmic.
 * 
 * Revision 1.4  89/02/24  22:58:14  malcolm
 * Fixed non-ansi #else clause.
 * 
 * Revision 1.3  88/12/06  21:11:03  malcolm
 * Cleaned up code and added lots of comments.
 * 
 * Revision 1.2  88/10/23  23:08:10  malcolm
 * Made compatible with ANSI C (removed pragma's and endif comments).
 * 
 * Revision 1.1  88/10/23  22:39:24  malcolm
 * Initial revision
 * 
 *
 */

static char	*RCSid = "$Header: correlate.c,v 2.4 90/12/17 17:57:40 malcolm Exp $";

/*
 *	This file calculates correlations of the signals produced by each 
 *	channel of the cochlea.  The output from these routines is called
 *	a correlogram.
 *
 *	Correlograms are computed by passing the output of the cochlear model
 *	to this module every sample time.  The vector (of length 
 *	CorrelogramHeight) of values is stored in an array for later 
 *	computation.  Periodically the routine EarCorrelation should be called
 *	to compute the autocorrelation of each channel and return an array
 *	of autocorrelations.
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

#ifndef	PI
3.141592653589793234
#endif	/* PI */

int	CorrelogramHeight;
int	CorrelogramWidth;			/* 
						 * Size of the correlogram.  
						 * The Init routine looks
						 * at the request and sets 
						 * these global variables so 
						 * that other people know 
						 * the correct sizes.
						 *
						 * The actual correlogram 
						 * output routines store the
						 * data into an array of this 
						 * size.
						 */

static float	*CorrelationData = NULL;	/* Actual storage for 
						 * the correlation 
						 * input data.  Length
						 * x Channels
						 */
static int32	CorrelationIndex = 0;		/* Current index into data */
static int	CorrelationDataLength = 0;	/* Length of input data */
static int	CorrelationFFTLength = 0;	/* Length of FFT's */
static int	CorrelationLogLength = 0;	/* Log of FFT Length */

						/* How to access data 
						 * in the input data array 
						 */
#define	CorrelationDataArray(Channel, Index) \
	(CorrelationData[(int32)(Channel)*CorrelationDataLength + \
	((int32)(CorrelationIndex + (Index))%CorrelationDataLength)])
	
						/* How to access data in
						 * the output data array
						 */
#define	OutputDataArray(Channel, Lag) \
	(Output[(int32)(Channel)*CorrelogramWidth + (Lag)])

static float	*Real, *Im;			/* Pointer to FFT 
						 * Storage
						 */

/*
 *	InitCorrelation - This routine should be called once to define the
 *	number of channels in the cochlear model and to set the number of 
 *	samples that should be saved.  The resolution of the correlogram
 *	is determined by the number of samples saved.
 */
LickInitCorrelation(Channels, Length, Between)
int	Channels, Length, Between;		/* Actual number of channels
						 * and desired correlogram 
						 * length.  Between is the
						 * number of data points 
						 * between correlograms.
						 */
{
	extern	char	*progname;
	
							/* Make it even */
							/* Save the size for
							 * other people who
							 * might need it.
							 */
	CorrelogramHeight = (Channels+1)& -2;
	CorrelogramWidth = Length;

							/* Need to pick a
							 * data length that is
							 * twice as long as the
							 * desired output so 
							 * that everything 
							 * works when windowed.
							 */
	if (Length > Between)
		CorrelationDataLength = 2*Length;
	else
		CorrelationDataLength = 2*Between;
	CorrelationIndex = 0;

							/* Some parameters 
							 * needed for the FFT 
							 * based correlation.
							 */
	for (CorrelationLogLength = 0; 
		(1<<CorrelationLogLength) < CorrelationDataLength; 
		CorrelationLogLength++);
	CorrelationLogLength ++;			/* Twice as long to 
							 * prevent circular 
							 * convolutions 
							 */
	CorrelationFFTLength = 1 << CorrelationLogLength;
	
#ifdef	DEBUG
	printf("Channels is %d, Length is %d, CorrelationFFTLength is %d, \n",
		Channels, Length, CorrelationFFTLength);
	printf("CorrelationDataLength is %d.\n", CorrelationDataLength);
#endif	/* DEBUG */
							/* Allocate an Extra 
							 * channel's worth of 
							 * storage so that
							 * correlations two at 
							 * a time (with FFT)
							 * work ok.
							 */
	CorrelationData = (float *)calloc((CorrelogramHeight+1L)*
							CorrelationDataLength, 
						sizeof(*CorrelationData));
	if (!CorrelationData){
		fprintf(stderr, 
			"%s: Can't allocate %d bytes for correlation data.\n",
			progname, (Channels+1L)*Length*sizeof(*CorrelationData));
		exit(1);
	}
	Real = (float *)calloc(CorrelationFFTLength+1L,sizeof(*Real));
	Im = (float *)calloc(CorrelationFFTLength+1L,sizeof(*Im));
	if (!Real || !Im){
		fprintf(stderr, 
			"%s: Can't allocate %ld bytes for correlation FFTs.\n",
			progname, (CorrelationFFTLength+1L)*2*
					sizeof(*CorrelationData));
		exit(1);
	}
#ifndef	PLOT3D
	if (UseUltra){
		InitDisplay();
							/* Set both frame 
							 * buffers to blue.
							 */
		SetDisplayValue(80 + 80*256 + 255*256*256);
		UpdateDisplay(CorrelationData,0,0,0.0,1.0);
		SetDisplayValue(80 + 80*256 + 255*256*256);
		UpdateDisplay(CorrelationData,0,0,0.0,1.0);
	}
#endif	/* PLOT3D */
}

#ifdef	DEBUG
/*
 *	CheckCorrelation - This routine is used to verify that the output 
 *	of the correlation routines is normalized as it should be.  This is
 *	used only for debugging.
 */
CheckCorrelation(where)
char	*where;
{
	register int32 i, Count;

	Count = CorrelogramHeight*CorrelationDataLength;
	for (i=0;i<Count;i++) {
		if (CorrelationData[i] > 1.0 || CorrelationData[i] < -1.0){
			printf("%s: Got bad correlation data at %d (%g).\n",
				where, i, CorrelationData[i]);
			exit(0);
		}
	}
}
#endif	/* DEBUG */

/*
 *	SendInputToCorrelation - This routine is called to pass an array of
 *	data from the cochlear model to the correlation routines.  The input
 *	array represents the firing rate of each channel of the cochlear model.
 *	The routine InitCorrelation should be called first to set the 
 *	parameters of the ear model.
 */
LickSendInputToCorrelation(Data)
float	Data[];
{
	register int	Channel;
	register float	f;
	int	i, One = 1;

	VMOV(Data,&One,&CorrelogramHeight,
		CorrelationData+CorrelationIndex,&CorrelationDataLength);

	CorrelationIndex = (CorrelationIndex + 1) % CorrelationDataLength;
}


/*
 *	MakeHamming - This routine makes a hamming window of any desired 
 *	length.  This routine maintains a cache to remember the last Hamming
 *	window created.  If the requested size is td.  Otherwise it deallocates
 *	the old Hamming window and creates a new one.
 */
static float	*HammingWindow = NULL;
static int	HammingLength = 0;

float *
MakeHamming(Length)
int	Length;
{
	int	i;

	if (!HammingWindow || Length != HammingLength){
		if (HammingWindow)
			free(HammingWindow);
		HammingWindow = NewFloatArray((int32)Length,"MakeHamming");
		HammingLength = Length;
		printf("Making a hamming window of length %d.\n", Length);
		for (i=0;i<Length;i++)
			HammingWindow[i] = .54 + .46 * cos(PI*(i-Length/2.0)/
								(Length/2.0));
	}
	return(HammingWindow);
}

static float	*CompensatorWindow = NULL;
static float	CompensatorLength = 0;

float *
MakeCompensator(Length)
int	Length;
{
	register int	i, lag;
	float	*Hamming = MakeHamming(Length), sum;

	if (!CompensatorWindow || Length != CompensatorLength){
		if (CompensatorWindow)
			free(CompensatorWindow);
		CompensatorWindow = NewFloatArray((int32)Length,
							"MakeCompensator");
		CompensatorLength = Length;
		printf("Making a Compensator window of length %d.\n", Length);
		for (lag = 0;lag<Length;lag++){
			sum = 0.0;
			for (i=0;i+lag<Length;i++){
				sum += Hamming[i]*Hamming[i+lag];
			}
			CompensatorWindow[i] = sum / CompensatorWindow[0];
		}
	}
	picout("Compensator",CompensatorWindow,Length*8);
	exit(1);
	return(CompensatorWindow);
}

/*
 *	SimpleCorrelation - This routine calculates the autocorrelation of
 *	each ear channel by brute force.  The resulting "CorrelogramWidth" 
 *	outputs for each channel are placed into the Output array.
 */
SimpleCorrelation(Output)
float	*Output;
{
	int	Lag, i, Channel;
	float	Sum, Max=0, Scaling;
	
	for (Channel = 0; Channel < CorrelogramHeight; Channel++){
		SystemCursor();
		for (Lag = 0; Lag < CorrelogramWidth; Lag++){
			Sum = 0.0;
			for (i = Lag; i < CorrelationDataLength; i++){
				Sum += CorrelationDataArray(Channel, i) *
					CorrelationDataArray(Channel, i - Lag);
			}
			if (Lag == 0) { 
				if (Sum == 0.0)
					Scaling = 1.0;
				else
					Scaling = 1/pow(Sum,Normalization);
			}
			OutputDataArray(Channel, Lag) = Sum * Scaling; 
		}
	} 
}

/*
 *	FFTCorrelation - Use FFT to compute autocorrelation.
 *	To save time use the symettry of the real FFT and do two FFT's at once.
 *	Stick the first data in the real part and the second data in the 
 *	imaginary part of the input.  Remember for real valued inputs the 
 *	output of the FFT will have even reals and odd imaginary components.  
 *	To get the output values do
 *		Real of first	even of real
 *		Im of first	odd of im
 *		
 *		Real of second	even of imaginary
 *		Im of second	odd of real
 *
 *	This routine is the preferred way to calculate the autocorrelation
 *	on all machines except for the Cray.
 */
FFTCorrelation(Output)
float	*Output;
{
	int	Channel, i, j;
	float	FirstScale, SecondScale;
	float	FirstReal, FirstIm, SecondReal,SecondIm, FirstMag, SecondMag;
	float	*Hamming = MakeHamming(CorrelationDataLength);
	
	for (Channel=0;Channel < CorrelogramHeight; Channel += 2){
		SystemCursor();
							/* Fill the data into 
							 * the real and 
							 * imaginary parts
							 * of the FFT array 
							 */
		for (i=0; i<CorrelationDataLength; i++){
			Real[i] = CorrelationDataArray(Channel, i) * Hamming[i];
			Im[i] = CorrelationDataArray(Channel+1, i) * Hamming[i];
			Real[CorrelationFFTLength-i] = 0.0;
			Im[CorrelationFFTLength-i] = 0.0;
		}
							/* Zero pad the rest of
							 * the array.
							 */
		for (i=CorrelationDataLength;i<=CorrelationFFTLength/2;i++){
			Real[i] = Im[i] = 0.0;
			Real[CorrelationFFTLength-i] = 0.0;
			Im[CorrelationFFTLength-i] = 0.0;
		}
		
							/* Now do the FFT */
		SystemCursor();
		starttimer(1);
		fft(Real, Im, CorrelationLogLength, 0);
		endtimer(1);
		SystemCursor();

							/* Fill in an extra 
							 * point at the end so 
							 * when we use symettry
							 * to split the FFT 
							 * output into two
							 * we don't have to 
							 * deal with the [0] 
							 * case specially
							 */
		starttimer(2);
		Real[CorrelationFFTLength] = Real[0];
		Im[CorrelationFFTLength] = Im[0];

#if	CRAY
		FMAG(Real,Im,&CorrelationFFTLength);
#else	
#include	"ivdep.h"
		for (i=0;i<=CorrelationFFTLength/2;i++){
			j = CorrelationFFTLength - i;
			FirstReal =  (Real[i] + Real[j]) /2.0;
			FirstIm =    (Im[i]   - Im[j])   /2.0;
			SecondReal = (Im[i]   + Im[j])   /2.0;
			SecondIm =   (Real[i] - Real[j]) /2.0;
		
			FirstMag = FirstReal*FirstReal + FirstIm*FirstIm;
			SecondMag = SecondReal*SecondReal + SecondIm*SecondIm;
			
			Real[i] = Real[j] = FirstMag;
			Im[i] = Im[j] = SecondMag;
		}
#endif
		endtimer(2);
		
		SystemCursor();
		starttimer(3);
		fft(Real, Im, CorrelationLogLength, 1);
		endtimer(3);
		SystemCursor();
	
							/* Hack Alert
							 * The following code
							 * tries to normalize
							 * the autocorrelations
							 * by the zero lag 
							 * term. Normally this 
							 * is ok but when the 
							 * input is all zeros 
							 * then after
							 * fft'ing you are left
							 * with random bit 
							 * noise.  This check
							 * tries only does the
							 * scaling if the zero
							 * lag is really bigger
							 * than the rest.
							 */

		starttimer(4);
		if (Real[0] <= 0.0e-10 || Real[0] < Real[1] || 
		    Real[0] < Real[2]){
			FirstScale = 0.0;
		} else {
			FirstScale = 1/pow(Real[0],Normalization);
		}  

		if (Im[0] <= 0.0e-10 || Im[0] < Im[1] || Im[0] < Im[2]){
			SecondScale = 0.0;
		} else {
			SecondScale = 1/pow(Im[0],Normalization);
		}

		for (i=0;i<CorrelogramWidth; i++){
			OutputDataArray(Channel, i)  = Real[i]*FirstScale;
			OutputDataArray(Channel+1, i)  = Im[i]*SecondScale;
		}
		endtimer(4);
	}
}

/*
 *	HartleyCorrelation - This is the prototype of a routine that will
 *	eventually use the Hartley transform to do the correlation.  The 
 *	Bracewell/Hartley transform has all the nice properties of the FFT
 *	but it is real valued so we don't have to do the 2-in-one hack that
 *	was used to make the FFTCorrelation run fast.
 */
HartleyCorrelation(Output)
float	*Output;
{
	int	i, Channel, Lag;
	float	Sum, Max=0, Scaling, FirstReal, FirstIm;
	float	*Hamming = MakeHamming(CorrelationDataLength);
	
	for (Channel = 0; Channel < CorrelogramHeight; Channel++){
							/* Fill the data into 
							 * the real part
							 * of the FFT array 
							 */
		for (i=0; i<CorrelationDataLength; i++){
			Real[i] = CorrelationDataArray(Channel, i) * Hamming[i];
			Real[CorrelationFFTLength-i] = 0.0;
			Im[i] = Im[CorrelationFFTLength-i] = 0.0;
		}
							/* Zero pad the rest of
							 * the array.
							 */
		for (i=CorrelationDataLength;i<=CorrelationFFTLength/2;i++){
			Real[i] = Im[i] = 0.0;
			Real[CorrelationFFTLength-i] = 0.0;
			Im[CorrelationFFTLength-i] = 0.0;
		}
		
							/* Now do the FFT */
		starttimer(11);
		fft(Real, Im, CorrelationLogLength, 0);
		endtimer(11);

		starttimer(12);
		Real[CorrelationFFTLength] = Real[0];
		Im[CorrelationFFTLength] = Im[0];

#include	"ivdep.h"
		for (i=0;i<CorrelationFFTLength;i++){
			FirstReal =  Real[i];
			FirstIm =    Im[i];
			Real[i] = FirstReal*FirstReal + FirstIm*FirstIm;
			Im[i] = 0;
		}
		endtimer(12);
		
		starttimer(13);
		fft(Real, Im, CorrelationLogLength, 1);
		endtimer(13);

		if (Real[0] <= 0.0 || Real[0] < Real[1] || Real[0] < Real[2]){
			Scaling = 0.0;
		} else {
			Scaling = 1/pow(Real[0],Normalization);
		}  
		for (i=0;i<CorrelogramWidth; i++){
			OutputDataArray(Channel, i) = Real[i]*Scaling;
		}
	}
}


#ifdef	CRAY

/*
 *	CrayCorrelation - This routine calls a Fortran routine (that has been
 *	vectorized and parallelized) to really do the correlations fast on
 *	the Cray.  See the file fcor.f for the definition of the FCOR function.
 */
CrayCorrelation(Output)
float	*Output;
{
	int	i;
	float	*window = MakeHamming(CorrelationDataLength);
	float	*work;
	float	*initfft();
	extern float Normalization;

	work = initfft(CorrelationFFTLength);

#ifdef	TEST
	for (i=0;i<CorrelationDataLength;i++)
		CorrelationData[2*CorrelationDataLength + i] = 
				1+sin(i*3.141592/10.0);
	CorrelationIndex = 0;
#endif	/* TEST */

	starttimer(14);
	FCOR(CorrelationData, Output, window, work, &CorrelogramWidth,
		&CorrelogramHeight, &CorrelationDataLength, 
		&CorrelationFFTLength, &CorrelationIndex, &Normalization);
	endtimer(14);
}

#endif	CRAY
	
/*
 *	EarCorrelation - This routine is actually called from the main program
 *	to do the correlations.  It just starts up a timer, calls the 
 *	appropriate correlation code and optionally displays the result on 
 *	the Ultra Buffer (if you are running on a Cray.)
 */
double
LickEarCorrelation(Output)
float	*Output;
{
	float	Max;
	
	starttimer(0);
	
#ifdef	CRAY
	CrayCorrelation(Output);
#else
	FFTCorrelation(Output);
#endif
	
	if (LogDisplay)
		StretchDisplay(Output, CorrelogramWidth, CorrelogramHeight);

	Max = CorrelogramWidth * AgcStage4Target * AgcStage4Target * .125 * 
		UltraHeadroom;
	Max = Max / pow(Max,Normalization);

	if (UseUltra){
		char	PlotBuffer[BUFSIZ];
		int	i, Lag, Channel;

		starttimer(7);

#ifdef	PLOT3D
#define	TmpFile	"/tmp/correlogram"
#define	PlotCommand "/v/malcolm/bin/plot3d z=/tmp/correlogram xsize=%d ysize=%d xskip=1 zmax=%g -P yp=5 scfac=.5 tl=\"%10.5f\" phi1=-40 | oultraplot -a"
		sprintf(PlotBuffer, PlotCommand, CorrelogramWidth,
		    CorrelogramHeight,
		    AgcStage4Target*sqrt(2.0*CorrelogramWidth)*10*UltraHeadroom,
		    SampleNumber/sample_rate);
	        Output[CorrelogramWidth*(CorrelogramHeight-1)] = 
		    AgcStage4Target*sqrt(2.0*CorrelogramWidth)*10*UltraHeadroom;
		

		picout(TmpFile,Output,CorrelogramWidth*CorrelogramHeight);
		printf("%s\n", PlotBuffer);
		i = system(PlotBuffer);
		if (i < 0){
		     fprintf(stderr,"System() failed.  Try setenv NCPUS=1\n");
		     exit(1);
		}
#else	/* !PLOT3D */
		UpdateDisplay(Output,CorrelogramWidth,CorrelogramHeight,
					0.0, Max);
#endif	/* PLOT3D */
		endtimer(7);
	}
	endtimer(0);

	return(Max);
}

static	int	*index;
static	float	*warp, *frac, *frac1, *newd;
static	int	StretchStarted = 0;

StretchDisplay(data,lags,channels)
float	*data;
int	lags, channels;
{
	register int i,j;
	
	if (!StretchStarted){
		float	alpha, beta;

		index = (int *) malloc(sizeof(*index)*lags);
		warp = NewFloatArray((int32) lags, "StretchDisplay");
		frac = NewFloatArray((int32) lags, "StretchDisplay");
		frac1 = NewFloatArray((int32) lags, "StretchDisplay");
		newd = NewFloatArray((int32) lags, "StretchDisplay");

		if (!index || !warp || !frac || !frac1 || !newd){
			fprintf(stderr,
				"Can't allocate space for StretchDisplay.\n");
			exit(1);
		}
						/* 
						 * Want to solve
						 *  warp(x) = beta e^(alpha x)
						 * with warp(0) = 10 and
						 *      warp(lags) = lags.
						 */
		beta = 20.0;
		alpha = log(lags/beta)/lags;
		for (i=0;i<lags;i++){
			warp[i] = beta * pow(2.718,alpha*i);

			index[i] = floor(warp[i]);
			frac[i] = warp[i]-index[i];
			frac1[i] = 1-frac[i];
			picout("warp.d",warp,lags);
		}
		StretchStarted = 1;
	}

	for (j=0;j<channels;j++){
		float	*row;

		row = &data[j*lags];

#include	"ivdep.h"
		for (i=0;i<lags;i++)
			newd[i] = row[index[i+1]]*frac1[i+1] +
				  row[index[i+1]+1]*frac[i+1];
#include	"ivdep.h"
		for (i=0;i<lags;i++)
			row[i] = newd[i];
	}
}

#ifdef	MAIN

#define	NChannels	2
#define	NLength		32

char	*progname = "correlate test";

/*
 *	Main Program (Test Code) - This main program generates two channels
 *	of data and then calls FFTCorrelation to make sure the output is 
 *	correct.  One channel is a sine wave so the output should also be
 *	a sine wave.  The other channel has a square pulse so the output should
 *	be a triangle wave.
 */

int	UseUltra = 0, LogDisplay = 0;
float	AgcStage4Target = .0004;
float	Normalization = .75;
float	UltraHeadroom = 1.0;
float	sample_rate = 16000.0;
int	SampleNumber = 0;

main(){
	register int	i;

	float	Data[NChannels], *Output;

	LickInitCorrelation(NChannels, NLength, NLength);
	
	for (i=0;i<NLength; i++){
		Data[1] = sin(i*4.0/NLength*2*PI);
		Data[0] = (i < NLength/2)? 1.0 : 0.0;
		LickSendInputToCorrelation(Data);
	}
	
	Output = (float *)calloc(NLength*NChannels,sizeof(*Output));
	if (!Output){
		fprintf(stderr, 
		     "%s: Couldn't allocate %d bytes for output correlation.\n",
			progname, NLength*NChannels*sizeof(*Output));
		exit(1);
	}
	
	FFTCorrelation(Output);
	
	for (i=0;i<NLength; i++){
		printf("%4d:	%10g	%10g\n",i,Output[0*CorrelogramWidth+i], 
						Output[1*CorrelogramWidth+i]);
	}
}

#endif	/* MAIN */
