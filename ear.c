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
 * $Header: ear.c,v 2.5 91/01/23 11:22:46 malcolm Exp $
 *
 * $Log:	ear.c,v $
 * Revision 2.5  91/01/23  11:22:46  malcolm
 * Added a couple of calls to SystemCursor so that the user can get access
 * to the menus and such when running on slow machines like Mac SE's.
 * 
 * Revision 2.4  90/12/17  17:59:38  malcolm
 * Mostly a cleanup, especially in the correlogram code.  Also added routine
 * to write out correlogram summary information.
 * 
 * Revision 2.3  90/11/06  20:44:51  malcolm
 * Added video recording capability (just affects the timing of correlations.)
 * Also made impulse height a function of the input gain.
 * 
 * Revision 2.2  90/01/28  15:33:33  malcolm
 * Fixed spellings of correlogram.  Also fixed the code that calls the 
 * correlogram code so that it can compute either the Licklider or Shamma
 * styles of correlogram.
 * 
 * Revision 2.1  89/11/09  23:11:40  malcolm
 * Removed the Calculate Response code.  Also added code to output correlograms
 * in sync with NTSC video tape.
 * 
 * Revision 2.0.1.1  89/07/28  21:35:12  malcolm
 * Changed the call to EarCorrelation so that it accepted the maximum
 * value returned.  Changed the ByteOutput code so that it used this maximum
 * when limiting the output level.
 * 
 * Revision 2.0  89/07/25  18:58:21  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.12  89/07/25  18:52:51  malcolm
 * Fixed up some defines for the BYTEOUTPUT option.
 * 
 * Revision 1.11  89/07/21  14:41:37  malcolm
 * Fixed up code that puts job in the real time queue.  It still doesn't
 * run at full speed but its getting better.  Perhaps all that is missing
 * is the plock call.
 * 
 * Revision 1.10  89/07/19  12:39:21  malcolm
 * Cleaned out some obsolete printfs.
 * Made some of the code a little prettier.
 * Made first attempt at a Cray RunRealTime function.
 * 
 * Revision 1.9  89/06/20  22:44:48  malcolm
 * Added support for LightSpeed C and added #ifdef so correlogram output
 * files could be byte format.
 * 
 * Revision 1.8  89/04/09  17:01:58  malcolm
 * Added ultra frame buffer recording stuff and allow DecimationFactor of
 * zero to indicate no decimation AND no filtering.
 * 
 * Revision 1.7  89/02/26  14:54:57  malcolm
 * Removed fixed length ear model code (used to be dependent on value of
 * EAR_LENGTH include variable).  Also moved ear design calls after reading
 * in the input file so the sample rate is correct.  Finally moved output
 * functions to output.c file.
 * 
 * Revision 1.6  89/02/25  12:11:48  malcolm
 * Moved most utility routines (like argument parsing and setting up
 * Fortran ear model) to the utilities.c file.  Also moved the definition
 * of most global variables to this file.  All that is left now is the
 * main program for calculating the ear model output and the correlograms.
 * 
 * Revision 1.5  89/02/24  22:53:46  malcolm
 * Changed impulse height and only get extra CPUs if we are using the
 * Ultra buffer or computing correlations.
 * 
 * Revision 1.4  88/12/04  17:36:09  malcolm
 * Added parameters for number of CPUs (cpus) and normalize factor.  Also
 * changed the default values of cstep to 128 and clag to 256.  Finally called
 * new routine that looks at the file suffix to determine the file type.
 * 
 * Revision 1.3  88/11/03  15:47:52  malcolm
 * Cleaned up the code that determines when the ultra buffer is used.  Added
 * second dummy channel to handle the preemphasis part of the model.  Defined
 * routine that can be called from Fortran to get the state of the global
 * variables into a common block.
 * 
 * Revision 1.2  88/10/23  23:08:14  malcolm
 * Made compatible with ANSI C (removed pragma's and endif comments).
 * 
 * Revision 1.1  88/10/23  22:39:37  malcolm
 * Initial revision
 * 
 *
 */

static char	*RCSid = "$Header: ear.c,v 2.5 91/01/23 11:22:46 malcolm Exp $";

/*
 *	This is the main program for calculating cochleagrams and correlograms.
 *	This routine just does argument parsing and calls other routines to
 *	design the filters and perform the calculations.
 */

#include	<stdio.h>
#include	"complex.h"
#include	"filter.h"
#include	"ear.h"

main(argc,argv)
int	argc;
char	**argv;
{
	int32	i;
	float	*Data, *ReadInputFile();
	float	Output[MaxN];
	float	CorrelogramMax;

	InitParms();

	progname = *argv;
	while(argv++ , --argc){
		if (**argv == '?')
			syntax();
		if(( **argv=='-' || **argv=='+')&& argv[0][1]) {
			ProcessOption(*argv);
		}else {
			ProcessArgument(*argv);
		}
	}
	
	if (ffn){
		ffp = fopen(ffn,"w");
		if (!ffp) {
			fprintf(stderr, 
			    "%s: Can't open %s for writing filtered data.\n",
				progname);
			exit(1);
		}
	}

	CheckParms();

	if (printflag){
		PrintStats();
	}

	OpenOutputFile(ofn);

	if (ImpulseInput){
		DataLength = 512;
		Data = (float *)calloc(DataLength, sizeof(*Data));
		Data[0] = InputGain;
	} else {
		Data = ReadInputFile(ifn, &sample_rate, &DataLength);
	}

	SystemCursor();

	ChangeDecimationParameters();
	ChangeAgcParams();

	SystemCursor();
	DesignEarFilters();

#ifdef	MINUSG
	CPUs = 1;
#endif	/* MINUSG */

#ifdef CRAY
	if (CPUs > 1 && (UseUltra || cfn) ){	
		printf("Getting %d cpus.\n", CPUs);
		GETCPUS(&CPUs);

		/* RunRealTime();	 */

	}
#endif	/* CRAY */

	inittimers();

	if (1) {
		int	FrameCount, CorrelationPictureCount = 0;
		long	CorrelationOutputSize;
		float	*CorrelationOutput = NULL;
		float	SamplesPerFrame, NextMovieTime;
		
		SamplesPerFrame = sample_rate/NTSC_RATE;
		NextMovieTime = SamplesPerFrame;
		FrameCount = 0;

		if (cfn || UseUltra){
			if (!VideoRecord)
				InitCorrelation(EarLength-2, CorrelationLags,
						CorrelationStep);
			else
				InitCorrelation(EarLength-2, CorrelationLags,
						(int)SamplesPerFrame);
			CorrelationOutputSize = (long)CorrelogramWidth * 
						CorrelogramHeight;
			CorrelationOutput = NewFloatArray((int32)
							CorrelationOutputSize,
							"CorrelationOutput");
		}

		for (i=0;i<DataLength;i++){
			SystemCursor();
			SampleNumber = i;

			starttimer(10);
			EARSTEP(&Data[i], Output);
			endtimer(10);
			if (ffp)
				fwrite(Output+2,sizeof(*Output),EarLength-2,
					ffp);

			if (!VideoRecord && (!DecimationFactor || 
						(i % DecimationFactor) == 0) ||
			        (VideoRecord && i && i>NextMovieTime)){
				WriteOutputFile(Output+2,EarLength-2);
			}
			if (cfn || UseUltra){
				SendInputToCorrelation(Output+2);
			}

			if ((cfn || UseUltra)
			    && ((!VideoRecord && i && (i%CorrelationStep) == 0)
			        || (VideoRecord && i && i>NextMovieTime))){
				FrameCount++;
#if	THINK_C || BUILDAPP
			    	printf(
				     "Computing Correlogram for sample %ld.\n",
				     (long)i);
#endif	/* THINK_C */
				CorrelogramMax = 
					EarCorrelation(CorrelationOutput);
				if (cfn) {
					char Name[BUFSIZ];
					sprintf(Name,"%s%05ld(%dx%d)", cfn, 
							(long)FrameCount,
							CorrelogramWidth,
							CorrelogramHeight);
#ifdef	BYTEOUTPUT
					BytePicout(Name, CorrelationOutput,
						(int32)CorrelogramWidth*
							CorrelogramHeight,
						0.0, CorrelogramMax);
#else
					picout(Name, CorrelationOutput,
						(int32)CorrelogramWidth*
							CorrelogramHeight);
#endif	/* BYTEOUTPUT */
				}
				CorrelationPictureCount++;
			}
			if (VideoRecord && i && i>NextMovieTime){
				NextMovieTime += SamplesPerFrame;
			}
		}
		if (!UseUltra){
			if (cfn){
				printf(
				   "Correlogram (%s) is %dx%dx%d.\n",
					cfn, CorrelogramWidth, 
					CorrelogramHeight, 
					CorrelationPictureCount);
			}
		}
	}
	CloseOutputFile();

	if (cfn)
		WriteCorrelationInfo(CorrelogramMax);

	if (ffp) 
		fclose(ffp);
	printtimers();
}

RecordVideo(){
#ifdef	CRAY
	fprintf(stderr,"remsh dumbo \"setenv WF_RECDEV diaq;/usr/local/wave/bin/record\"");
	system("echo hello.");
	system("remsh dumbo \"setenv WF_RECDEV diaq;/usr/local/wave/bin/record\"");
/*
	char	Buffer[512];
	int	i;
	printf("Recording a frame.....");
	clearerr(stdin);
	i = fgets(Buffer,512,stdin);
	printf("fgets got *%d*\n",i);
 */
#endif	/* CRAY */
}

/*
 *	RunRealTime - This is a first attempt at putting the ear model
 *	into the Cray real time queue so it runs really fast.  It doesn't
 *	work yet.
 */

#ifdef	CRAY

#define	CPUFILE	"/dev/cpu/any"

#include	<errno.h>
#include	<sys/cpu.h>
#include	<sys/category.h>

#endif	/* CRAY */

RunRealTime(){
#ifdef	CRAY
	int		i, fd;
	struct cpudev	cpu;

	fd = open(CPUFILE,0);

	if (fd < 0) {
		fprintf(stderr, "Couldn't open %s for reading.\n", CPUFILE);
		return(0);
	}

	cpu.cat = C_PROC;
	cpu.id = 0;
	cpu.word = 0;

	i = ioctl(fd, CP_SETRT,(char *) &cpu);
	if (i < 0){
		fprintf(stderr, "CP_SETRT ioctl failed.\n");
		perror("ear: RunRealTime:");
	}
#endif	/* CRAY */
}


WriteCorrelationInfo(Max)
float	Max;
{
	FILE	*fp;
	char	InfoName[BUFSIZ], *p;

	if (!cfn)
		return;

	strcpy(InfoName, cfn);
	p = InfoName + strlen(InfoName) - 1;

	while (p > InfoName && *p != '/')
		p--;

	if (*p == '/')
		p++;
	*p = 0;

	strcat(InfoName, CorrelationInfoFile);

	printf("Putting information in %s.\n", InfoName);

	fp = fopen(InfoName, "w");

	fprintf(fp, "ImageMax=%g\n", Max);
	fprintf(fp, "ImageMin=0\n");

	fclose(fp);
}
