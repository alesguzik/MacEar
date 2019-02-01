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
 *
 * $Header: ear.h,v 2.3 90/12/17 18:00:28 malcolm Exp $
 *
 * $Log:	ear.h,v $
 * Revision 2.3  90/12/17  18:00:28  malcolm
 * Cleanup and added VMOV definition.
 * 
 * Revision 2.2  90/01/28  15:21:38  malcolm
 * Fixed spelling of correlogram.  Added definitions for new Licklider/
 * Shamma correlogram routines.  Also added definitions for NewFloatArray
 * and NewIntArray.
 * 
 * Revision 2.1  89/11/09  23:05:52  malcolm
 * Added NTSC defines (for making videos), added support for Steve Rubin's
 * UNIX shell.
 * 
 * Revision 2.0.1.2  89/08/10  22:18:55  malcolm
 * Added MaxSamples definition and fixed correlogram spelling.
 * 
 * Revision 2.0.1.1  89/07/28  21:34:07  malcolm
 * Added code for SystemCursor on the Macintosh so that other applications
 * can run.  Also added definition of EarCorrelation so that a float is 
 * returned.
 * 
 * Revision 2.0  89/07/25  18:58:24  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.5  89/07/25  18:53:35  malcolm
 * Cleaned up some of the comments and added a lot of stuff about file 
 * formats.
 * 
 * Revision 1.4  89/07/19  12:59:19  malcolm
 * Changed some printfs around for the Mac.  Changed printf to rubin_printf
 * when building a Macintosh application.
 * 
 * Revision 1.3  89/06/20  22:43:54  malcolm
 * Added support for LightSpeed C.
 * 
 * Revision 1.2  89/04/09  17:03:25  malcolm
 * Moved Macintosh includes into this file.  Also added an extern declarations
 * for all state variables.
 * 
 * Revision 1.1  88/10/23  22:39:48  malcolm
 * Initial revision
 * 
 *
 */

/*
 *	Output formats.
 *	This code supports a number of different output formats.  You should
 *	define one of the following formats and undef the rest.  These options
 *	determine the style of output for the cochleagram and the correlogram.
 *	
 *	Header Formats
 *	First determine whether there should be a header on the files.  The
 *	best supported is NOHEADER.  The OGC people wanted a different format
 *	based on the CMU syncrep format.
 *	
 *	In the NOHEADER format the data is output in the order which it is
 *	computed.  The cochleagram output is written 84 channels per time
 *	step with channels ordered from high frequency to low (base to
 *	apex of the cochlea.)  The correlogram output is one two dimensional
 *	time step per file.  Each file has the autocorrelation of each
 *	channel written in order from base to apex.  The output value is
 *	the zero lag for channel 0 (at the base),  the second value is the
 *	autocorrelation output for a time lag of one sample interval of the
 *	channel 0.
 *	
 */
#define	NOHEADER 		/* Define Simplest Output File Format */
#undef	OGC			/* Let's do it the OGC way */
#undef	SPHYNX			/* Let's put out data for Kai Fu Lee's Sphynx */

/*
 *	Data Formats
 *	There are three different styles of output data.  Which one you should
 *	use depends on what you are going to do with the data.  If you are
 *	going to read the data with custom written software than binary 
 *	floating point is fastest.  If you are going to look at the output
 *	using an editor than ASCII text output is best.  If you are going
 *	to use an image display program than 8 bit byte output will be the
 *	most natural.
 *	
 *	Both the cochleagram and the correlogram are output in the format
 *	specified below.
 *	FLOATOUTPUT - Output the data in the machine's native binary floating
 *		point format.  This is the fastest method of IO since the data
 *		is copied directly from the array to the output file.  This is
 *		the default action.
 *	TEXTOUTPUT - Convert the data into ASCII and output the results in 
 *		a readable format.  This format is useful for reading the
 *		data in editors and programs like spreadsheets that want the
 *		data in a human readable format.  This option is often nice
 *		when verifying the output of the program.
 *	BYTEOUTPUT - Convert the data into 8 bit unsigned bytes for display 
 *		by a B&W display program.  Both the cochleagram and the 
 *		correlogram outputs are always non negative so a zero byte
 *		is zero.  The maximum output value (255) is a function of
 *		the final AGC target value.
 */

#if	!defined(FLOATOUTPUT) && !defined(TEXTOUTPUT) && !defined(BYTEOUPUT)

#undef	FLOATOUTPUT			/* Binary Floating Point Output */
#undef	TEXTOUTPUT			/* Readable ASCII Text Output */
#define	BYTEOUTPUT			/* 8 Bit Unsigned Byte Output */

#endif

#define	CorrelationInfoFile	".info"	/* Name of information file for */
					/* correlation directory. */

/*
 *	The definitions that follow are not for user configuration.  You
 *	shouldn't change any of the following parameters unless you know
 *	what you are doing.
 */

#include	"timer.h"
#include	"complex.h"
#include	"filter.h"

#ifdef	THINK_C
#include	<unix.h>
#include	<stdlib.h>

#define	int32	long			/* Can't use 16 bit ints... */

#endif	/* THINK_C */

#ifndef	int32
#define	int32	int
#endif

					/* A few of the arrays in this program
					 * are allocated at compile time to
					 * improve the performance.  This means
					 * that they have fixed size that is
					 * a function of the maximum number
					 * of channels expected.  This number
					 * is set to 180 so that the arrays
					 * can fit on a Mac.  If you're not
					 * using a Mac then you can set this 
					 * as high as you want.
					 *
					 * Note, if you change this number you
					 * must also change this parameter at
					 * the beginning of fear.f
					 */
#define	MaxN 180

#define	EAR_LENGTH		88
#define	LOG_RESPONSE_LENGTH	8
#define	RESPONSE_LENGTH		(1<<LOG_RESPONSE_LENGTH)


#ifndef	PI
#define	PI 3.141592653589792434
#endif	/* PI */

#define	NTSC_RATE	29.985
#define	NTSC_INCREMENT	(1/NTSC_RATE)

#if	THINK_C || BUILDAPP

#define	printf	rubin_printf		/* Use Unix shell version */
#define	main	_main			/* Use Steve's main */
#define	exit	rubin_exit		/* Use Unix shell version */

#ifdef	BUILDAPP			/* Only defined in MPW */

#include	<traps.h>
#include	<desk.h>

#endif	/* BUILDAPP */

#define	SystemCursor()	{short chr; gra_nextevent(&chr);}

/* #define	SystemCursor()		SystemTask();	*/

#else	/* THINK_C || BUILDAPP */

#define	SystemCursor()	;

#endif	/* THINK_C || BUILDAPP */

extern	float	AMAX(), AMIN();

#define min(a,b) (((a) > (b)) ? (b) : (a))
#define max(a,b) (((a) < (b)) ? (b) : (a))

#ifndef	CRAY

#define	VMOV(input, input_incp, lengthp, output, output_incp) { \
	register int	i; 					\
	float	*pin = (input), *pout = (output);		\
								\
	for (i=0; i< *(lengthp); i++){				\
		*pout = *pin;					\
		pin += *(input_incp);				\
		pout += *(output_incp);				\
	}							\
}

#endif	/* CRAY */



					/* Global State
					 * Used by the rest of the programs 
					 * Mostly definited in utilities.c
					 */

extern	int	Debug;
extern	int	printflag;
extern	int	ImpulseInput;
extern	int	UseAgc;
extern	int	UseCascade;
extern	int	UseDifference;
extern	int	CalculateResponse;
extern	int	ComputeFiltered;
extern	int	UseUltra;
extern	int	CPUs;
extern	int	VideoRecord;
extern	int	LogDisplay;
extern	char	*progname;
extern	char	*ifn;			/* Input File Name */
extern	char	*ofn;			/* Output File for Cochleagram */
extern	char	*ffn;			/* Filtered Output */
extern	char	*cfn;			/* Correlogram Directory Name */

extern	float	sample_rate;
extern	int32	DataLength;
extern	int32	MaxSamples;

extern	float	AgcStage1Tau;
extern	float	AgcStage2Tau;
extern	float	AgcStage3Tau;
extern	float	AgcStage4Tau;
extern	float	AgcStage1Target;
extern	float	AgcStage2Target;
extern	float	AgcStage3Target;
extern	float	AgcStage4Target;
extern	int	DecimationFactor;
extern	int	CorrelationStep;
extern	int	CorrelationLags;
extern	int	CorrelogramWidth;
extern	int	CorrelogramHeight;
extern	float	TauFactor;
extern	float	Normalization;
extern	float	UltraHeadroom;
extern	float	InputGain;

extern	float	EarBreakFreq;
extern	float	EarQ;
extern	float	EarStepFactor;
extern	float	EarSharpness;
extern	float	EarZeroOffset;
extern	float	EarPreemphCorner;

extern	float	AgcEpsilon1, AgcEpsilon2, AgcEpsilon3, AgcEpsilon4;
extern	float	a0[], a1[], a2[], b1[], b2[];
extern	float	DecimationEpsilon;
extern	struct	filter	EarFilterArray[];

extern	float	Output[];
extern	int	EarLength;
extern	FILE	*ffp;

extern	int	SampleNumber;

extern	int	LickInitCorrelation();
extern	int	LickSendInputToCorrelation();
extern	double	LickEarCorrelation();

extern	int	ShammaInitCorrelation();
extern	int	ShammaSendInputToCorrelation();
extern	double	ShammaEarCorrelation();

extern	int	PattersonInitCorrelation();
extern	int	PattersonSendInputToCorrelation();
extern	double	PattersonEarCorrelation();

extern	char	*CorrelogramType;
extern	int	SharpResponse;
extern	int	TransformCorrelogram;

extern int	(*SendInputToCorrelation)();
extern int	(*InitCorrelation)();
extern double	(*EarCorrelation)();

extern float	*NewFloatArray();
extern int	*NewIntArray();

