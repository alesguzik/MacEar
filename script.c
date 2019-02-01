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
 * $Header: script.c,v 2.2 90/11/06 20:54:53 malcolm Exp $
 *
 * $Log:	script.c,v $
 * Revision 2.2  90/11/06  20:54:53  malcolm
 * Changed copyright messsage.
 * 
 * Revision 2.1  89/11/09  23:16:35  malcolm
 * Rewrote video starting condition (when starting in the middle.)
 * 
 * Revision 2.0  89/07/25  18:58:55  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 *
 */

static char	*RCSid = "$Header: script.c,v 2.2 90/11/06 20:54:53 malcolm Exp $";

#include	<stdio.h>
#include	"complex.h"
#include	"filter.h"
#include	"ear.h"

#define	IMPULSE_HEIGHT .0001

main(argc,argv)
int	argc;
char	**argv;
{
	int	i;
	FILE	*ofp;

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
	
	inittimers();

	Animation2();
}

Animation1()
{
	register int	i;
	int	one = 1, two = 2, three = 3, four = 4, Channel;
	float	Output[MaxN], *Sound, GETAGC();
	float	*Data, *ReadInputFile();

	UseDifference = 0;
	DecimationFactor = 1;
	
	if (ImpulseInput){
		DataLength = 512;
		Data = (float *)calloc(DataLength, sizeof(*Data));
		Data[0] = IMPULSE_HEIGHT;
	} else {
		float 	*Signal;
		Signal = ReadInputFile(ifn, &sample_rate, &DataLength);
		Data = (float *) malloc(3*DataLength*sizeof(*Data));
		for (i=0;i<DataLength;i++)
			Data[i] = Data[i+DataLength] = Data[i+2*DataLength] =
				Signal[i];
		DataLength *= 3;
	}

	ChangeDecimationParameters();
	ChangeAgcParams();

	DesignEarFilters();

#ifdef	MINUSG
	CPUs = 1;
#endif	/* MINUSG */

#ifdef CRAY
	if (CPUs > 1 && (UseUltra || cfn) ){	
		printf("Getting %d cpus.\n", CPUs);
		GETCPUS(&CPUs);
	}
#endif	/* CRAY */

/*
	AgcStage1Tau = AgcStage2Tau = AgcStage3Tau = AgcStage4Tau = 1000000;
 */

	if (printflag){
		PrintStats();
	}

	Sound = (float *)malloc(DataLength*sizeof(*Data));

	for (i=0;i<DataLength;i++){
		SystemCursor();
		SampleNumber = i;

		starttimer(10);
		EARSTEP(&Data[i], Output);
		endtimer(10);
		Channel = (i*EarLength)/DataLength+1;
		Sound[i] = Output[Channel] ;
	}
	WriteADCFile(ofn, sample_rate, DataLength, Sound);

	printtimers();
}

#define	Between(a,b,c)	((a > b && a < c) || (a < b && a > c))

Median(Data,Length)
float	*Data;
int	Length;
{
	register int	i;
	register float a, b, c, save;

	save = Data[0];
	for (i=1;i<Length-1;i++){
		a = Data[i-1];
		b = Data[i];
		c = Data[i+1];
		Data[i-1] = save;
		if (Between(a,b,c))
			save = a;
		else if (Between(c,a,b))
			save = c;
		else
			save = b;
	}
}

Animation2()
{
	int	LineCount = 0;
	int	CorrelationLineCount = 0, CorrelationPictureCount = 0;
	int	CorrelationOutputSize;
	float	*CorrelationOutput = NULL, *CorrelationZero = NULL;
	float	*Data, *ReadInputFile();
	register int	i;
	float	Output[MaxN], SamplesPerFrame, SamplesToNextFrame;

	cfn = ofn = "";
	UseUltra = 1;
	TauFactor= 1;
	DecimationFactor= 1;
	CorrelationLags = 512;
	Normalization = .5;
	EarQ = 4;
	EarStepFactor = .125;

	if (ImpulseInput){
		DataLength = 512;
		Data = (float *)calloc(DataLength, sizeof(*Data));
		Data[0] = IMPULSE_HEIGHT;
	} else {
		Data = ReadInputFile(ifn, &sample_rate, &DataLength);
	}

	ChangeDecimationParameters();
	ChangeAgcParams();

	DesignEarFilters();

#ifdef	MINUSG
	CPUs = 1;
#endif	/* MINUSG */

#ifdef CRAY
	if (CPUs > 1 && (UseUltra || cfn) ){	
		printf("Getting %d cpus.\n", CPUs);
		GETCPUS(&CPUs);
	}
#endif	/* CRAY */

	if (printflag){
		PrintStats();
	}

	InitCorrelation(EarLength-2, 2*CorrelationLags);
	CorrelationOutputSize = (EarLength + 10) * 
				CorrelationLags;
	CorrelationOutput = (float *)
				malloc(CorrelationOutputSize*
				sizeof(CorrelationOutput[0]));
	if (!CorrelationOutput){
		fprintf(stderr, 
			"%s: Couldn't get %d bytes for correlation output.\n",
			"Correlation Animation", CorrelationOutputSize);
		exit(2);
	}

	SamplesPerFrame = sample_rate/29.985;
	SamplesToNextFrame = SamplesPerFrame;
	printf("sample_rate is %g, SamplesPerFrame is %g.\n",
		sample_rate, SamplesPerFrame);
	
	OpenConnection("dumbo");
	SendCommandToRemote("setenv WF_RECDEV diaq");

	for (i=0;i<DataLength;i++){
		SampleNumber = i;
		starttimer(10);
		EARSTEP(&Data[i], Output);
		endtimer(10);
		SendInputToCorrelation(Output+2);

		SamplesToNextFrame--;
		if (SamplesToNextFrame < 0){
			extern int	FrameNumber;
			SamplesToNextFrame += SamplesPerFrame;

			EarCorrelation(CorrelationOutput, 
					CorrelationLags);
			if (SampleNumber/sample_rate > -1.0)
				RecordVideo();
			else
				printf("Skipping frame %d.\n",FrameNumber++);
			CorrelationPictureCount++;
		}
	}
	printtimers();
}


int	FrameNumber = 0;

RecordVideo(){
	int	i;
	extern	int	errno;
	FILE	*fp;


#ifdef	OLD
	i = system("/usr/ucb/remsh dumbo 'setenv WF_RECDEV diaq;/usr/local/wave/bin/record'");
/*
	i = system("/usr/ucb/remsh dumbo 'date'");
	*/
	if (i){
		printf("%d: system returns %d (errno is %d).\n",FrameNumber,
			i,errno);
		exit(1);
	} else {
		printf("Finished frame %d.\n", FrameNumber);
	}
	WaitForFrameBuffer();
#endif	OLD

	SendCommandToRemote("/usr/local/wave/bin/record");
	
	fp = fopen(".frame","w");
	if (fp){
		fprintf(fp,"%d\n", FrameNumber);
		fclose(fp);
	}
	FrameNumber++;
/*
	char	Buffer[512];
	int	i;
	printf("Recording a frame.....");
	clearerr(stdin);
	i = fgets(Buffer,512,stdin);
	printf("fgets got *%d*\n",i);
 */
}

#define	WAITFILE	"/tmp/stopanimation"

WaitForFrameBuffer(){
	FILE	*fp;

	fp = fopen(WAITFILE,"r");
	if (fp){
		fclose(fp);
		printf("Found %s....waiting.\n", WAITFILE);
		while (fp = fopen(WAITFILE,"r")){
			fclose(fp);
			sleep(1);
		}
		printf("Resuming animation.\n");
	}
}
