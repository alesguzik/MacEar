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
 * $Header: output.c,v 2.4 91/01/22 14:59:15 malcolm Exp $
 *
 * $Log:	output.c,v $
 * Revision 2.4  91/01/22  14:59:15  malcolm
 * Fixed an extra factor of two in the SPHYNX cosine transform code.
 * 
 * Revision 2.3  90/12/17  18:03:57  malcolm
 * Changed max to Max to clear up preprocessor problem on some machines.
 * 
 * Revision 2.2  90/11/06  20:53:05  malcolm
 * Added support for Sphynx VQ format.  All we do here is compute the
 * first 13 coefficients of the Cosine transform.
 * 
 * Revision 2.1  89/11/09  23:09:29  malcolm
 * Cleaned up some unused variables.
 * 
 * Revision 2.0.1.2  89/08/14  11:12:09  malcolm
 * Added binary option to the fopen call for LightSpeed C.
 * 
 * Revision 2.0.1.1  89/07/31  16:15:05  malcolm
 * Fixed bug in the BYTEOUTPUT part of WriteOutputFile.  Seems like the cast
 * was being applied to early and all zeros was getting written out.
 * 
 * Revision 2.0  89/07/25  18:58:51  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.5  89/07/19  12:49:13  malcolm
 * Added option (compile time) for byte and ASCII output files.
 * 
 * Revision 1.4  89/04/09  17:06:24  malcolm
 * Added support for decimation factor of zero.
 * 
 * Revision 1.3  89/03/15  10:16:38  malcolm
 * Fixed the OGC syncrep format so that it outputs the channels from low
 * frequency to high.
 * 
 * Revision 1.2  89/02/26  15:55:33  malcolm
 * Added code to support the OGC (actually CMU) output file format.
 * 
 * Revision 1.1  89/02/26  14:40:41  malcolm
 * Initial revision
 * 

 */

static char	*RCSid = "$Header: output.c,v 2.4 91/01/22 14:59:15 malcolm Exp $";

#include	<stdio.h>
#include	<math.h>
#include	"ear.h"

#ifdef	NOHEADER

static FILE	*ofp = NULL;
static int	LineCount = 0;
static int	LineLength = 0;
static char	*FileName = "";

OpenOutputFile(ofn)
char	*ofn;
{
        if (ofn && *ofn){
		FileName = ofn;
#ifdef	THINK_C
#ifdef	TEXTOUTPUT				/* Stupid LSC can't handle right way */
		ofp = fopen(ofn, "w");
#else
		ofp = fopen(ofn, "wb");
#endif	/* TEXTOUTPUT */
#else
                ofp = fopen(ofn, "w");
#endif	/* THINK_C */
                if (!ofp){
                        fprintf(stderr,
			    "OpenOutputFile: Can't open %s for output data.\n",
                                ofn);
                        exit(1);
                }
        }
}

WriteOutputFile(Data,Length)
float	*Data;
int	Length;
{
	int	i, result;
	float	Max, scale;

	if (ofp) {
		if (LineLength && Length != LineLength){
		    fprintf(stderr,"WriteOutputFile: Uneven line lengths.\n");
		    fprintf(stderr," Expecting %d, got %d at line %d.\n",
			LineLength, Length, LineCount);
		    exit(1);
		}
		LineLength = Length;
				
#ifdef	TEXTOUTPUT
		for (i=0; i<Length; i++){
			fprintf(ofp, "%g\t", Data[i]);
			if ((i % 4) == 3)
				fprintf(ofp,"\n");
		}
		fprintf(ofp,"\n");
#else 
#ifdef BYTEOUTPUT
		Max = 2*AgcStage4Target;
		scale = 255.0/Max;
		for (i=0; i<Length; i++){
			if (Data[i] < 0)
				putc(0, ofp);
			else if (Data[i] > Max)
				putc(0xff, ofp);
			else
				putc((int) (Data[i] * scale + 0.5), ofp);
		}
#else	/* Not bytes or ascii.....must be floats. */
		result = fwrite(Data,sizeof(float),Length,ofp);
		if (result != Length){
			fprintf(stderr,"Couldn't write output data (%d).\n",
				result);
			CloseOutputFile();
			exit(1);
		}
#endif	/* BYTEOUTPUT */
#endif	/* TEXTOUTPUT */
		LineCount++;
	}
}

CloseOutputFile(){
	if (ofp) {
#ifdef	TEXTOUTPUT
		printf("Image (%s) is %dx%d (ascii text, no header).\n", 
			FileName, LineLength, LineCount);
#else
#ifdef	BYTEOUTPUT
		printf("Image (%s) is %dx%d (8 bit bytes, no header).\n", 
			FileName, LineLength, LineCount);
#else
		printf("Image (%s) is %dx%d (floating point, no header).\n", 
			FileName, LineLength, LineCount);
#endif	/* BYTEOUTPUT */
#endif	/* TEXTOUTPUT */
		fclose(ofp);
	}
}
 
#endif	/* NOHEADER */



#ifdef	OGC

				/* WARNING....this stuff is a real
				 * hack to see if things can work.
				 *	Malcolm@apple.com  1/12/89
				 */
#include	"syncstruct.h"

static	int	NumberOfFrames = 0;
static	int	StartADCIndex = 0;

struct DataStruct	{
	short	*Data;
	int	Length;
	int	Start;
	struct DataStruct *Next;
};
struct DataStruct  FirstBuffer = {NULL, 0, 0, NULL};
struct DataStruct  *LastBuffer = &FirstBuffer;

/* HACK---Just set up the first buffer.
 * Only used if OGC is defined.
 */

static FILE	*ofp;
static char	*FileName = "";
#define	MAX	(AgcStage4Target*1.5)

OpenOutputFile(ofn)
char	*ofn;
{

        if (ofn && *ofn){
		FileName = ofn;
                ofp = fopen(ofn, "w");
                if (!ofp){
                        fprintf(stderr,
			    "OpenOutputFile: Can't open %s for output data.\n",
                                ofn);
                        exit(1);
                }
		FirstBuffer.Data = NULL;
		FirstBuffer.Length = 0;
		FirstBuffer.Start = 0;
		FirstBuffer.Next = NULL;
        }
}

/* HACK --- Save the data so that we can output it all at once at
 * the end of the run.
 */
WriteOutputFile(Data,Length)
float	*Data;
int	Length;
{
	int	i;
	float	Max;
	struct DataStruct *NextData;
	extern	int	DecimationFactor;
	extern	float	AgcStage4Target;

	if (!ofp)
		return;

	NextData = (struct DataStruct *)malloc(sizeof(*NextData));
	if (!NextData){
		fprintf(stderr, "Can't allocate space for data struct.\n");
		exit(1);
	}

	NextData->Data = (short *) malloc(Length*
					sizeof(*NextData->Data));
	if (!NextData->Data){
		fprintf(stderr, "Can't allocate data space.\n");
		exit(1);
	}

	for (i=0;i<Length;i++){
#ifdef	TEST
		NextData->Data[i] = 100*sin(6.28/Length*i*NumberOfFrames/80);
		if (NextData->Data[i] < 0)
			NextData->Data[i] = 0;
#else	
		if (Data[i] > MAX)
			Data[i] = MAX;
		NextData->Data[i] = Data[i]/MAX * 100;
#endif	TEST
	}
	NextData->Length = Length;
	NextData->Start = StartADCIndex;
	NextData->Next = NULL;
	LastBuffer->Next = NextData;
	LastBuffer = NextData;

	NumberOfFrames++;
	StartADCIndex += (DecimationFactor < 1)? 1 : DecimationFactor;

}

/* HACK---OK, now write out the data to the file.  First write out
 * all the headers and then send out the data.
 */
CloseOutputFile()
{
	SYNC	FrameInfo;
	int	Header[2], i = 0;
	struct	DataStruct *dp;

	rewind(ofp);
	Header[0] = -1;
	Header[1] = NumberOfFrames;
	Write32BitsHighLow(ofp,Header[0]);
	Write32BitsHighLow(ofp,Header[1]);

	for (dp = FirstBuffer.Next;dp;dp=dp->Next){
		Write32BitsHighLow(ofp,dp->Start);
		Write16BitsHighLow(ofp,256);	/* Fake it. */
		Write16BitsHighLow(ofp,(DecimationFactor < 1)?
					1 : DecimationFactor);
		Write16BitsHighLow(ofp, 0);	/* Don't know if pitch*/
		Write16BitsHighLow(ofp, dp->Length);	/* Spectral Size */
		Write32BitsHighLow(ofp, 0);	/* Frame */
		Write16BitsHighLow(ofp, 0);	/* Number of Energy Points */
		Write16BitsHighLow(ofp, 0);	/* Padding */
		Write32BitsHighLow(ofp, 0);	/* Energy and Extra Space */
		i++;
	}
	printf("Number of frames in output file %s is  %d (OGC Format).\n",
		FileName, i);

	for (dp = FirstBuffer.Next;dp;dp=dp->Next){
		for (i=dp->Length-1;i>=0;i--){
			Write16BitsHighLow(ofp, dp->Data[i]);
		}
	}
}
#endif	/* OGC */

#ifdef	SPHYNX

static	int	NumberOfFrames = 0;
static	char	*FileName;
static	FILE	*ofp;

OpenOutputFile(ofn)
char	*ofn;
{

        if (ofn && *ofn){
		FileName = ofn;
                ofp = fopen(ofn, "w");
                if (!ofp){
                        fprintf(stderr,
			    "OpenOutputFile: Can't open %s for output data.\n",
                                ofn);
                        exit(1);
                }
		Write32BitsHighLow(ofp,0);	/* Save room for byte count */
        }
}

#define	COS_LENGTH	13
static	float	(*CosTransform)[][COS_LENGTH] = 0;

WriteOutputFile(Data,Length)
float	*Data;
int	Length;
{
	int	f, i;
	float	result;

	if (!ofp)
		return;

	if (CosTransform == 0){
		printf("Computing the Sphynx cosine transform.\n");
		CosTransform = (float (*)[][COS_LENGTH])
				NewFloatArray(COS_LENGTH*Length);

		for (f = 0;f<COS_LENGTH;f++){
			for (i=0;i<Length;i++){
				(*CosTransform)[i][f] = 
					cos(PI*f*i/(float)Length);
			}
		}
		for (i=0;i<Length;i++)
			(*CosTransform)[i][0] *= 800000;

		picout("cos.transform", CosTransform, COS_LENGTH*Length);
	}

	for (f=0;f<COS_LENGTH;f++){
		result = 0.0;
#include	"ivdep.h"
		for (i=0;i<Length;i++){
			result += (*CosTransform)[i][f]*Data[i];
		}
		WriteIeeeFloatHighLow(ofp,result);
	}
	
	NumberOfFrames++;
}

/* HACK---OK, now write out the frame count to the file.
 */
CloseOutputFile()
{
	rewind(ofp);
						/* Number of Bytes */
	Write32BitsHighLow(ofp,4*COS_LENGTH*NumberOfFrames);	
	fflush(ofp);
	printf("Number of frames in output file %s is  %d (Sphynx Format).\n",
		FileName, NumberOfFrames);
}
#endif	/* SPHYNX */

