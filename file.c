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
 *	Copyright (c) 1988-1991 by Apple Computer, Inc
 *		All Rights Reserved.
 *
 * $Header: file.c,v 2.8 91/02/20 17:32:14 malcolm Exp $
 *
 * $Log:	file.c,v $
 * Revision 2.8  91/02/20  17:32:14  malcolm
 * ARRRGGGHHH.....D*mn Lightspeed C compiler was converting 0x80 into
 * unsigned integers which then broke the ReadByteFile routine.  Changed
 * Hex constants to Decimal and now code works on LightSpeed (Macintosh).
 * 
 * Revision 2.7  91/01/09  15:44:38  malcolm
 * Added support (untested though) for GW Instruments MacSpeech format.
 * 
 * Revision 2.6  90/11/08  08:26:03  malcolm
 * Fixed code for ConvertToIeeeExtended and WriteAiffFile.  Neither routine
 * was anywhere close to being correct.
 * 
 * Revision 2.5  90/11/07  08:53:28  malcolm
 * Added IRCAM file support.  This is a machine dependent file format (ick).
 * Also changed fileAbort to take more arguments.
 * 
 * Revision 2.4  90/11/06  20:50:38  malcolm
 * Added binary float and IEEE floating point support.  Also added support
 * for the Entropic and the AIFF file formats.
 * 
 * Revision 2.3  90/08/25  15:39:28  malcolm
 * Added first attempt at supporting the Entropic/Waves file format.  This
 * code isn't working yet.
 * 
 * Also added better support of Ieee floating point format.  This code
 * should work for all numbers but those that are close to the minimum
 * and maximum that can be expressed with IEEE.
 * 
 * Revision 2.2  90/01/28  15:28:57  malcolm
 * Fixed some printfs.   Also use the new NewFloatArray routine to allocate
 * all array storage.  Fixed call to ReadWavFile to remove extra parameter
 * (Thanks to Robert E. Novak at MIPs/SPEC.)
 * 
 * Revision 2.1  89/11/09  23:10:50  malcolm
 * Fixed some error messages so they referenced the correct routine.
 * 
 * Revision 2.0.1.2  89/08/10  22:14:11  malcolm
 * David Mellinger's (CCRMA@Stanford) fixes.  Added support for Dyaxis and
 * NeXT input formats.  Also added MaxSamples parameter to limit input
 * data.
 * 
 * Revision 2.0.1.1  89/07/28  21:33:02  malcolm
 * Added a missing fclose after reading in an ADC file.
 * 
 * Revision 2.0  89/07/25  18:58:36  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.8  89/07/19  12:48:00  malcolm
 * Fixed infinite loop bug in the code that finishes reading the header
 * words.
 * 
 * Revision 1.7  89/06/21  11:08:09  malcolm
 * Just a couple of small bug fixes for ADC files and LightSpeed C.
 * 
 * Revision 1.6  89/06/20  22:45:28  malcolm
 * Added support (int32 type) for LightSpeed C.
 * 
 * Revision 1.5  89/04/09  16:59:47  malcolm
 * Added support for writing ADC files (used by other programs) and reading
 * data files (.fl suffix) with native floating point format.  Also changed
 * MacRecorder sample rate to be 22254.545454...as per the Macintosh spec.
 * 
 * Revision 1.4  89/02/24  22:57:40  malcolm
 * Made compatible with Lightspeed C for the Macintosh.
 * 
 * Revision 1.3  88/12/04  17:38:25  malcolm
 * Added support for ADC files and choosing the input style based on the 
 * file suffix.
 * 
 * Revision 1.2  88/11/29  00:42:48  malcolm
 * Added support for Macintosh MacRecorder 8 bit files.
 * 
 * Revision 1.1  88/10/23  22:41:12  malcolm
 * Initial revision
 * 
 *
 */

static char	*RCSid = "$Header: file.c,v 2.8 91/02/20 17:32:14 malcolm Exp $";


/*
 *	This file reads the input speech data in a number of different formats.
 *	
 *	Note: Motorola processors (Macintosh, Sun, Sparc, etc) places the
 *		bytes in the word from high to low (they are big-endian).
 *		Use the HighLow routines to match the native format of these
 *		machines.
 *	
 *	Note: Intel-like machines (PCs, Sequent) use little-endian format.
 *		Use the LowHigh routines for these machines.
 */

#include	<stdio.h>
#include	<math.h>
#ifdef	PLAY

int     MaxSamples = -1;
float   InputGain = 1;

#ifdef  LSC
typedef long int32;
#else
typedef int int32;
#endif

float	*
NewFloatArray(size,usage)
int32	size;
char	*usage;
{
	float	*p;

	p = (float *)calloc(sizeof(*p),size);
	if (!p){
		fprintf(stderr,"NewFloatArray: Can't allocate %ld floats.\n", 
			size);
		exit(1);
	}
	return p;
}

#else
#include	"ear.h"
#endif	/* PLAY */


ReadByte(fp)
FILE	*fp;
{
	int	result;
	
	result = getc(fp) & 0xff;
	if (result & 0x80)
		result = result - 0x100;
	return result;
}

Read16BitsLowHigh(fp)
FILE	*fp;
{
	int	first, second, result;

	first = 0xff & getc(fp);
	second = 0xff & getc(fp);

	result = (second << 8) + first;
#ifndef	THINK_C
	if (result & 0x8000)
		result = result - 0x10000;
#endif
	return(result);
}

Read16BitsHighLow(fp)
FILE	*fp;
{
	int	first, second, result;

	first = 0xff & getc(fp);
	second = 0xff & getc(fp);

	result = (first << 8) + second;
#ifndef	THINK_C
	if (result & 0x8000)
		result = result - 0x10000;
#endif
	return(result);
}

Write8Bits(fp,i)
FILE	*fp;
int	i;
{
	putc(i&0xff,fp);
}

Write16BitsLowHigh(fp,i)
FILE	*fp;
int	i;
{
	putc(i&0xff,fp);
	putc((i>>8)&0xff,fp);
}

Write16BitsHighLow(fp,i)
FILE	*fp;
int	i;
{
	putc((i>>8)&0xff,fp);
	putc(i&0xff,fp);
}

int32 Read24BitsHighLow(fp)
FILE	*fp;
{
	int	first, second, third;
	int32	result;

	first = 0xff & getc(fp);
	second = 0xff & getc(fp);
	third = 0xff & getc(fp);

	result = (first << 16) + (second << 8) + third;
	if (result & 0x800000)
		result = result - 0x1000000;
	return(result);
}

#define	Read32BitsLowHigh(f)	Read32Bits(f)

int32
Read32Bits(fp)
FILE	*fp;
{
	int32	first, second, result;

	first = 0xffff & Read16BitsLowHigh(fp);
	second = 0xffff & Read16BitsLowHigh(fp);

	result = (second << 16) + first;
#ifdef	CRAY
	if (result & 0x80000000)
		result = result - 0x100000000;
#endif
	return(result);
}
	
int32
Read32BitsHighLow(fp)
FILE	*fp;
{
	int32	first, second, result;

	first = 0xffff & Read16BitsHighLow(fp);
	second = 0xffff & Read16BitsHighLow(fp);

	result = (first << 16) + second;
#ifdef	CRAY
	if (result & 0x80000000)
		result = result - 0x100000000;
#endif
	return(result);
}
	
Write32Bits(fp,i)
long	i;
FILE	*fp;
{
	Write16BitsLowHigh(fp,(int)(i&0xffffL));
	Write16BitsLowHigh(fp,(int)((i>>16)&0xffffL));
}

Write32BitsLowHigh(fp,i)
long	i;
FILE	*fp;
{
	Write16BitsLowHigh(fp,(int32)(i&0xffffL));
	Write16BitsLowHigh(fp,(int32)((i>>16)&0xffffL));
}

Write32BitsHighLow(fp,i)
long	i;
FILE	*fp;
{
	Write16BitsHighLow(fp,(int32)((i>>16)&0xffffL));
	Write16BitsHighLow(fp,(int32)(i&0xffffL));
}

float *
ReadADCFile(FileName, SamplingRate, Length)
char	*FileName;
float	*SamplingRate;
int32	*Length;
{
	register int32	i, SampleTime, n, HeaderSize;
	float	*Data;
	FILE	*fp;

	fp = fopen(FileName, "r");
	if (!fp){
		fprintf(stderr, "ReadADCFile: Couldn't open %s for reading.\n",
			FileName);
		exit(1);
	}

	HeaderSize = Read16BitsLowHigh(fp);
	HeaderSize--;
	Read16BitsLowHigh(fp);				/* Version Number */
	HeaderSize--;
	Read16BitsLowHigh(fp);				/* Channels */
	HeaderSize--;
	SampleTime = Read16BitsLowHigh(fp);
	HeaderSize--;
	n = Read32Bits(fp);
	if (MaxSamples >= 0 && n > MaxSamples) n = MaxSamples;
	HeaderSize -= 2;
	while (HeaderSize > 0){
		Read16BitsLowHigh(fp);
		HeaderSize--;
	}
	*SamplingRate = 4.0e6 / SampleTime;

	fprintf(stderr,
		"Reading %ld samples from %s with sampling rate of %g.\n", 
		(long)n, FileName, *SamplingRate);
	Data = NewFloatArray((int32) n, "ReadADCFile");
			
	for (i=0;i<n;i++)
		Data[i] = Read16BitsLowHigh(fp)/65536.0*16*InputGain;
	
	*Length = i;
	fclose(fp);
	
	return(Data);
}

WriteADCFile(FileName, SamplingRate, Length, Data)
char	*FileName;
float	SamplingRate;
int32	Length;
float	*Data;
{
	register int32	i;
	FILE	*fp;
	float	Max;

	if (FileName[0] == '-' && FileName[1] == '\0')
		fp = stdout;
	else
		fp = fopen(FileName, "w");

	if (!fp){
		fprintf(stderr, "WriteADCFile: Couldn't open %s for writing.\n",
			FileName);
		exit(1);
	}

	Write16BitsLowHigh(fp,6);
	Write16BitsLowHigh(fp,0);
	Write16BitsLowHigh(fp,1);
	Write16BitsLowHigh(fp,(int) (4.0e6/SamplingRate));
	Write32Bits(fp,(long) Length);

	Max = Data[0];
	for (i=1;i<Length;i++)
		if (Data[i] > Max)
			Max = Data[i];
	Max *= 10.0;

	fprintf(stderr,"Writing %s with sample rate %f and length %ld.\n",
		FileName, SamplingRate, Length);
	fprintf(stderr," Maximum value of file is %g.\n", Max);

	Max = ((1<<12)-1)/Max;
	for (i=0;i<Length;i++)
		Write16BitsLowHigh(fp,(int)(Data[i]*Max));
	
	fclose(fp);
}

float *
ReadDACFile(FileName, SamplingRate, Length)
char	*FileName;
float	*SamplingRate;
int32	*Length;
{
	register int32	i, SampleTime, n, HeaderSize;
	float	*Data;
	FILE	*fp;

	fp = fopen(FileName, "r");
	if (!fp){
		fprintf(stderr, "ReadADCFile: Couldn't open %s for reading.\n",
			FileName);
		exit(1);
	}

	HeaderSize = Read16BitsHighLow(fp);
	HeaderSize--;
	Read16BitsHighLow(fp);				/* Version Number */
	HeaderSize--;
	Read16BitsHighLow(fp);				/* Channels */
	HeaderSize--;
	SampleTime = Read16BitsHighLow(fp);
	HeaderSize--;
	n = Read32BitsHighLow(fp);
	if (MaxSamples >= 0 && n > MaxSamples) n = MaxSamples;
	HeaderSize -= 2;
	while (HeaderSize > 0){
		Read16BitsHighLow(fp);
		HeaderSize--;
	}
	*SamplingRate = 4.0e6 / SampleTime;

	fprintf(stderr,
		"Reading %ld samples from %s with sampling rate of %g.\n", 
		(long)n, FileName, *SamplingRate);
	Data = NewFloatArray((int32) n, "ReadADCFile");
			
	for (i=0;i<n;i++)
		Data[i] = Read16BitsHighLow(fp)/65536.0*16*InputGain;
	
	*Length = i;
	fclose(fp);
	
	return(Data);
}

int32
FileLength(fp)
FILE	*fp;
{
	int	pos, end;

	pos = ftell(fp);
	fseek(fp, 0L, 2);
	end = ftell(fp);
	fseek(fp,pos,0);
	return(end);
}

float *
ReadWavFile(FileName, Length)
char	*FileName;
int32	*Length;
{
	register int32	i;
	float	*Data;
	FILE	*fp;

	fp = fopen(FileName, "r");
	if (!fp){
		fprintf(stderr, "ReadWavFile: Couldn't open %s for reading.\n",
			FileName);
		exit(1);
	}

	*Length = FileLength(fp)/2;
	if (MaxSamples >= 0 && *Length > MaxSamples) *Length = MaxSamples;

	Data = NewFloatArray((int32) *Length, "ReadWavFile");
	
	for (i=0;i<*Length;i++)
		Data[i] = Read16BitsLowHigh(fp)/32768.0*InputGain;
	
	return(Data);
}


float *
ReadByteFile(FileName, Length)
char	*FileName;
int32	*Length;
{
	register int	i;
	float	*Data;
	FILE	*fp;

	fp = fopen(FileName, "r");
	if (!fp){
		fprintf(stderr, "ReadByteFile: Couldn't open %s for reading.\n",
			FileName);
		exit(1);
	}

	*Length = FileLength(fp);
	if (MaxSamples >= 0 && *Length > MaxSamples) *Length = MaxSamples;

	Data = NewFloatArray((int32) *Length, "ReadByteFile");
	
	for (i=0;i<*Length;i++){
		Data[i] = (((int)getc(fp) & 255) - 128)/128.0*InputGain;
	}
	
	return(Data);
}

WriteByteFile(FileName, Length, Data)
char	*FileName;
int32	Length;
float	*Data;
{
	register int32	i;
	FILE	*fp;
	float	Max;

	fp = fopen(FileName, "w");
	if (!fp){
		fprintf(stderr, "WriteByteFile: Couldn't open %s for writing.\n",
			FileName);
		exit(1);
	}

	Max = Data[0];
	for (i=1;i<Length;i++)
		if (Data[i] > Max)
			Max = Data[i];

	fprintf(stderr,"Writing byte file %s with length %ld.\n",
		FileName, Length);
	fprintf(stderr," Maximum value of file is %g.\n", Max);

	Max = 127/Max;
	for (i=0;i<Length;i++)
		putc((int)(Data[i]*Max)+128,fp);
	
	fclose(fp);
}

/* Dyaxis file: 512-byte header, 16-bit stereo interleaved samples
 * with left channel first.  Each sample has high byte first.
 */
float *ReadDyaxisFile(FileName, Length)
char	*FileName;
int	*Length;
{
	register int	i;
	float	*Data;
	FILE	*fp;
	long len;

	fp = fopen(FileName, "r");
	if (!fp){
	    fprintf(stderr, "ReadDyaxisFile: Couldn't open %s for reading.\n",
		    FileName);
	    exit(1);
	}

	len = FileLength(fp);		/* length in bytes */
	*Length = (len ) / 2;		/* left-channel samples in file */
	if (MaxSamples >= 0 && *Length > MaxSamples) *Length = MaxSamples;
	
	Data = NewFloatArray((int32) *Length, "ReadDyaxisFile");
	
	for (i=0;i<*Length;i++) {
	    Data[i] = Read16BitsHighLow(fp)/32768.0*InputGain;	/* left */
/*	    Read16BitsHighLow(fp);			   right (ignored) */
	}
	
	return(Data);
}

/* NeXT file: Variable-length header, then 16-bit samples (interleaved
 * if stereo).
 */
float *ReadNeXTFile(FileName, Length, SamplingRate)
char *FileName;
int *Length;
float *SamplingRate;
{
	register int	i, stereo;
	float	*Data;
	FILE	*fp;
	struct {
		char suffix[4];		/* ".snd" */
		long hdr_len,		/* header length following ".snd" */
			data_len,		/* data len, in bytes */
			mode,			/* 3 ==> 16-bit linear */
			srate,
			nChannels;		/* 1 for mono, 2 for stereo */
	} header;

	fp = fopen(FileName, "r");
	if (!fp) 
		fileAbort("Couldn't open %s for reading.\n", FileName);
	if (fread(&header, sizeof(header), 1, fp) != 1)
		fileAbort("Can't read header of %s.\n", FileName);
	if (header.mode != 3) 
		    fileAbort("Sound file %s is not 16-bit linear (mode 3).\n", 
			FileName);
	fseek(fp, header.hdr_len + 4, 0);	/* skip comment */

	*SamplingRate = header.srate;
	stereo = (header.nChannels == 2);
	*Length = header.data_len / 2;	
	if (stereo) *Length /= 2;
	if (MaxSamples >= 0 && *Length > MaxSamples) *Length = MaxSamples;
	
	Data = NewFloatArray((int32) *Length, "ReadNeXTFile");
	
	for (i = 0; i < *Length; i++) {
	    Data[i] = Read16BitsHighLow(fp) / 32768.0*InputGain;/* left */
	    if (stereo) Read16BitsHighLow(fp);		/* right (ignored) */
	}
	
	return Data;
}

/* IRCAM file: 1024-byte  header, then 16-bit samples (interleaved
 * if stereo).
 */

#define IRCAM_HEADER_SIZE	1024
#define IRCAM_MAGIC_NUMBER	0x0001A364L

float *ReadIRCAMFile(FileName, Length, SamplingRate)
char *FileName;
int *Length;
float *SamplingRate;
{
        register int    i, stereo;
        float   *Data;
        FILE    *fp;
        struct {
                int32	sf_magic;     /* = 107364L */
                float	sf_srate;     /* float samrate in Hz */
                int32	sf_chans;     /* 1=mono, 2=stereo */
                int32	sf_packmode;  /* 2 = 16 bit, 4 = float */
        /*	char	sf_codes;     /* ignore */
        } header;

        fp = fopen(FileName, "r");
        if (!fp) 
	  fileAbort("Couldn't open %s for reading.\n", FileName);
	
	if (fread(&header, sizeof(header), 1, fp) != 1)
	  fileAbort("Could not read header bytes of file %s.", FileName);

	
	/* Check some parameters */

/*
	printf("magic = %lX\nrate = %f\nchans = %lu\npackmode = %lu\n",
	       header.sf_magic,
	       header.sf_srate,
	       header.sf_chans,
	       header.sf_packmode);
 */
	

	if (header.sf_magic != IRCAM_MAGIC_NUMBER)
	  fileAbort("Sound file %s has a magic number of %lX, but expected %lX\n",
		    FileName,
		    header.sf_magic,
		    IRCAM_MAGIC_NUMBER);
	if ((header.sf_srate < 4500) || (header.sf_srate > 100000))
	  fileAbort("Sound file %s has an unreasonable sample rate (%fHz).\n",
		    FileName,
		    header.sf_srate);
	if ((header.sf_chans != 1) && (header.sf_chans != 2))
	  fileAbort("Sound file %s does not have 1 channel (channels=%lu).\n",
		    FileName,
		    header.sf_chans);
        if ((header.sf_packmode != 2) && (header.sf_packmode != 4)) 
	  fileAbort("Sound file %s has an unexpected packmode (%ld).\n", 
                        FileName);


	/* skip header block */

        fseek(fp, IRCAM_HEADER_SIZE, 0);

	/* Set return values */

        *SamplingRate = header.sf_srate;

        *Length = FileLength(fp) / header.sf_packmode / header.sf_chans;   

        if (MaxSamples >= 0 && *Length > MaxSamples) *Length = MaxSamples;
        
        Data = NewFloatArray((int32) *Length * header.sf_chans, "ReadIRCAMFile");
        
	if (header.sf_packmode == 2)
	  {
	    register float factor;
	    int result;

	    factor = InputGain / 32768.0;

	    result = fread(&Data[0],
			   sizeof(short),
			   *Length * header.sf_chans,
			   fp);

	    for (i = (*Length * header.sf_chans) - 1; i >= 0; i--)
	      {
		Data[i] = ((short*)Data)[i] * factor;
	      }
	  }
	else
	  {
	    register float factor;
	    int result;

	    factor = InputGain / 32768.0;

	    result = fread(&Data[0],
			   sizeof(float),
			   *Length * header.sf_chans,
			   fp);
 
	    for (i = result-1; i >= 0; i--)
	      {
		Data[i] *= factor;
	      }
	  }
	    
        
        return Data;
}


fileAbort(str, arg1, arg2, arg3, arg4)
char *str, *arg1, *arg2, *arg3, *arg4;
{
	fprintf(stderr, "Reading data file: ");
	fprintf(stderr, str, arg1, arg2, arg3, arg4);
	exit(1);
}
	
float *
ReadNativeFloatFile(FileName, Length)
char	*FileName;
int32	*Length;
{
	float	*Data;
	FILE	*fp;

	fp = fopen(FileName, "r");
	if (!fp){
		fprintf(stderr, "ReadNativeFloatFile: Couldn't open %s for reading.\n",
			FileName);
		exit(1);
	}

	*Length = FileLength(fp)/sizeof(float);
	if (MaxSamples >= 0 && *Length > MaxSamples) *Length = MaxSamples;

	Data = NewFloatArray((int32) *Length, "ReadNativeFloatFile");
	
	fread(Data,sizeof(float),*Length,fp);
	
	return(Data);
}

float	ReadIeeeFloatLowHigh(fp)
FILE	*fp;
{
	int32	bits;
	float	ConvertIeeeToFloat();

	bits = Read32BitsLowHigh(fp);
	return ConvertIeeeToFloat(bits);
}

float	ReadIeeeFloatHighLow(fp)
FILE	*fp;
{
	int32	bits;
	float	ConvertIeeeToFloat();

	bits = Read32BitsHighLow(fp);
	return ConvertIeeeToFloat(bits);
}

float	ConvertIeeeToFloat(bits)
int32	bits;
{
	float	f;
	int32	mantissa, expon;

	if (bits == 0)
		return 0.0;

	mantissa = (bits & 0x7fffff) + 0x800000;
	expon = (bits & 0x7f800000) >> 23;
	expon -= 127;

	f = (float)mantissa/(float)0x8000000 * exp(expon*log(2.0));
	if (bits & 0x80000000)
		return -f;
	else
		return f;
}

float	ReadIeeeExtendedHighLow(fp)
FILE	*fp;
{
	unsigned int	first, second, third, fourth, fifth;
	float	ConvertIeeeExtendedToFloat();

	first = Read16BitsHighLow(fp);
	second = Read16BitsHighLow(fp);
	third = Read16BitsHighLow(fp);
	fourth = Read16BitsHighLow(fp);
	fifth = Read16BitsHighLow(fp);

	return ConvertIeeeExtendedToFloat(first, second, third, fourth, fifth);
}

float	ConvertIeeeExtendedToFloat(first, second, third, fourth, fifth)
unsigned int	first, second, third, fourth, fifth;
{
	float	f, expfactor;
	int32	expon;

	first &= 0xffff;
	second &= 0xffff;
	third &= 0xffff;
	fourth &= 0xffff;
	fifth &= 0xffff;

	if (!first && !second && !third && !fourth && !fifth)
		return 0.0;

	expon = (first & 0x7fff);
	expon -= 16383;

	expfactor = exp(expon*log(2.0));
	f = (float)second/(float)0x8000 * expfactor;
	f += (float)third/(float)0x10000 * (expfactor /= 0x10000);
	f += (float)fourth/(float)0x10000 * (expfactor /= 0x10000);
	f += (float)fifth/(float)0x10000 * (expfactor /= 0x10000);

	if (first & 0x8000)
		return -f;
	else
		return f;
}

int32 ConvertToIeee(num)
float	num;
{
	int32	sign, bits, mantissa, expon;

	if (num < 0) {
		sign = 0x80000000;
		num *= -1;
	} else {
		sign = 0;
	}

	if (num == 0){
		bits = 0;
	} else {
		expon = floor(log(num)/log(2.0));
		if (expon < -126) {
			expon = 1;
			mantissa = 0;
		} else if (expon > 128){
			expon = 255;
			mantissa = 0x7fffff;
		} else {
			mantissa = floor((num/pow(2.0,(float)expon)-1)
					*0x800000+.5);
			expon += 127;
		}
	}

	bits = sign | (expon << 23) | mantissa;
	return bits;
}

float	WriteIeeeFloatLowHigh(fp, num)
FILE	*fp;
float	num;
{
	int32	bits;
	
	bits = ConvertToIeee(num);
	Write32BitsLowHigh(fp,bits);
}

float	WriteIeeeFloatHighLow(fp, num)
FILE	*fp;
float	num;
{
	int32	bits;
	
	bits = ConvertToIeee(num);
	Write32BitsHighLow(fp,bits);
}

/*
 *	ConvertToIeeeExtended - It's really hard to do the multiple word
 *	conversion without knowing how many bits the machine has.  Thus
 *	this routine will only get the first 24 bits right.
 */

ConvertToIeeeExtended(num, first, second, third, fourth, fifth)
float	num;
int	*first, *second, *third, *fourth, *fifth;
{
	int32	sign, mantissa, expon;

	if (num < 0) {
		sign = 0x8000;
		num *= -1;
	} else {
		sign = 0;
	}

	if (num == 0){
		mantissa = 0;
	} else {
		expon = floor(log(num)/log(2.0));
		if (expon < -16383) {
			expon = 1;
			mantissa = 0;
		} else if (expon > 16383){
			expon = 32767;
			mantissa = 0x7fffff;
		} else {
			mantissa = floor(num/pow(2.0,(float)expon)*0x400000+.5);
			expon += 16383;
		}
	}
	if (expon < 0 || expon > 32767){
		fprintf(stderr,
		"ConvertToIeeeExtended Program Error: Illegal Exponent (%d).\n",
			expon);
		return;
	}

#ifdef	TESTEXTENDED
	printf("Partial result is expon is %d and mantissa is 0x%x.\n",
		expon, mantissa);
#endif
	*first = expon | sign;
	*second = (mantissa>>7)&0xffff;
	*third = (mantissa<<1)&0xffff;
	*fourth = 0;
	*fifth = 0;
}

float	WriteIeeeExtendedLowHigh(fp, num)
FILE	*fp;
float	num;
{
	int	first, second, third, fourth, fifth;
	
	ConvertToIeeeExtended(num, &first, &second, &third, &fourth, &fifth);
	Write16BitsLowHigh(fp,fifth);
	Write16BitsLowHigh(fp,fourth);
	Write16BitsLowHigh(fp,third);
	Write16BitsLowHigh(fp,second);
	Write16BitsLowHigh(fp,first);
}

float	WriteIeeeExtendedHighLow(fp, num)
FILE	*fp;
float	num;
{
	int	first, second, third, fourth, fifth;
	
	ConvertToIeeeExtended(num, &first, &second, &third, &fourth, &fifth);
	Write16BitsHighLow(fp,first);
	Write16BitsHighLow(fp,second);
	Write16BitsHighLow(fp,third);
	Write16BitsHighLow(fp,fourth);
	Write16BitsHighLow(fp,fifth);
}

#define	AiffFORM	0x464f524d		/* "FORM" */
#define	AiffAIFF	0x41494646		/* "AIFF" */
#define	AiffCOMM	0x434f4d4d		/* "COMM" */
#define	AiffSSND	0x53534e44		/* "SSND" */

float	*ReadAiffFile(FileName, Length, SampleRate)
char	*FileName;
int32	*Length;
float	*SampleRate;
{
	int32	i, ChunkSize, SubSize, SoundPosition, numSampleFrames, offset;
	int	sampleSize, numChannels, blockSize;
	float	*Data;
	FILE	*fp;

	fp = fopen(FileName, "r");
	if (!fp){
		fprintf(stderr, 
			"ReadAiffDataFile: Couldn't open %s for reading.\n",
			FileName);
		exit(1);
	}

	*SampleRate = 0;
	*Length = 0;

	if (Read32BitsHighLow(fp) != AiffFORM){
		fprintf(stderr,
			"ReadAiffDataFile: Couldn't find initial chunk.\n");
		exit(1);
	}

	ChunkSize = Read32BitsHighLow(fp);

	if (Read32BitsHighLow(fp) != AiffAIFF){
		fprintf(stderr,
			"ReadAiffDataFile: Couldn't find AIFF chunk.\n");
		exit(1);
	}

	while (ChunkSize > 0){
		ChunkSize -= 4;
		switch(Read32BitsHighLow(fp)){
		case	AiffCOMM:
			ChunkSize -= SubSize = Read32BitsHighLow(fp);
			numChannels = Read16BitsHighLow(fp);	SubSize -= 2;
			numSampleFrames = Read32BitsHighLow(fp);SubSize -= 4;
			sampleSize = Read16BitsHighLow(fp);	SubSize -= 2;
			*SampleRate = ReadIeeeExtendedHighLow(fp); SubSize-=10;
			while (SubSize > 0){
				getc(fp);
				SubSize--;
			}
			break;
		case	AiffSSND:
			ChunkSize -= SubSize = Read32BitsHighLow(fp);
			offset = Read32BitsHighLow(fp);		SubSize -= 4;
			blockSize = Read32BitsHighLow(fp);	SubSize -= 4;
			SoundPosition = ftell(fp) + offset;
			
			while (SubSize > 0){
				getc(fp);
				SubSize--;
			}
			break;
		default:
			ChunkSize -= SubSize = Read32BitsHighLow(fp);
			while (SubSize > 0){
				getc(fp);
				SubSize--;
			}
			break;
		}
	}

	if (!SoundPosition){
		fprintf(stderr,
			"ReadAiffDataFile: Didn't find a SSND chunk.\n"
			);
		exit(1);
	}

	fseek(fp, SoundPosition, 0);
	*Length = numSampleFrames*numChannels;

	if (MaxSamples >= 0 && *Length > MaxSamples) 
		*Length = MaxSamples;

	fprintf(stderr,
		"Reading %ld samples from %s with sampling rate of %g.\n", 
		(long)*Length, FileName, *SampleRate);
	Data = NewFloatArray((int32) *Length, "ReadAiffDataFile");
	
	switch ((sampleSize+7)/8){
	case 1:
		for (i=0;i<*Length;i++){
			Data[i] = ReadByte(fp)/127.0;
		}
		break;
	case 2:
		for (i=0;i<*Length;i++){
			Data[i] = Read16BitsHighLow(fp)/32768.0;
		}
		break;
	case 3:
		for (i=0;i<*Length;i++){
			Data[i] = Read24BitsHighLow(fp)/32768.0/255.0;
		}
		break;
	case 4:
		for (i=0;i<*Length;i++){
			Data[i] = Read32BitsHighLow(fp)/32768.0/65536.0;
		}
		break;
	}
	return Data;
}

#define	AiffWordSize	1

WriteAiffFile(FileName, SampleRate, Length, Data, Channels)
char	*FileName;
int32	Length;
int	Channels;
float	*Data;
float	SampleRate;
{
	register int32	i;
	int	ChunkSize = 0;
	FILE	*fp;
	float	Max;

	if (strcmp(FileName,"-") != 0) {
		fp = fopen(FileName, "w");
		if (!fp){
			fprintf(stderr, 
			"WriteAiffFile: Couldn't open %s for writing.\n",
				FileName);
			exit(1);
		}
	} else {
		fp = stdout;
	}

	Max = Data[0];
	for (i=1;i<Length * Channels;i++)
		if (Data[i] > Max)
			Max = Data[i];

	fprintf(stderr,"Writing AIFF file %s with %ld frames.\n",
		FileName, Length);
	fprintf(stderr," Maximum value of file is %g.\n", Max);

	ChunkSize = 30 + Length*Channels*AiffWordSize;

	Write32BitsHighLow(fp, AiffFORM);	/* Magic Number */
	Write32BitsHighLow(fp, ChunkSize);	/* Chunk Size */
	Write32BitsHighLow(fp, AiffAIFF);	/* Type of Chunk */

						/* AIFF Common Chunk */
	Write32BitsHighLow(fp, AiffCOMM);
	Write32BitsHighLow(fp, 18);		/* Chunk Size */
	Write16BitsHighLow(fp, Channels);
	Write32BitsHighLow(fp, Length);	
	Write16BitsHighLow(fp, 8*AiffWordSize);	/* Sample Size */
	WriteIeeeExtendedHighLow(fp, SampleRate);

	ChunkSize = 8 + Length*Channels*AiffWordSize;
	Write32BitsHighLow(fp, AiffSSND);
	Write32BitsHighLow(fp, ChunkSize);	/* Chunk Size */
	Write32BitsHighLow(fp, 0);		/* Offset */
	Write32BitsHighLow(fp, 0);		/* Block Size*/

#if	AiffWordSize == 1
	Max = 127/Max;
#endif
	Max = 32767/Max;
	for (i=0;i<Length*Channels;i++)
#if	AiffWordSize == 1
		Write8Bits(fp,(int)(Data[i]*Max));
#else
		Write16BitsHighLow(fp,(int)(Data[i]*Max));
#endif

	if (fp != stdin)
		fclose(fp);
}

#ifdef	ESPS

/*
 *	ReadSignalStoreFile - First attempt to read the Entropic (Waves)
 *	Signal Processing format.
 */
	
#include	<esps/esps.h>
#include	<esps/fea.h>
#include	<esps/feasd.h>

float	*ReadEntropicFile(FileName, Length, SampleRate)
char	*FileName;
int32	*Length;
float	*SampleRate;
{
	float	*Data = 0;
	int	Channels;
	FILE	*inputsd_strm;
	long	i, HeaderLength = 0;
	struct	header	*sd_ihd;
	struct	feasd	*sd_feasd;


	*SampleRate = 0;
	*Length = 0;

	(void)eopen("play", FileName, "r", FT_FEA, FEA_SD, &sd_ihd,
			&inputsd_strm);
	
	*SampleRate = get_genhd_val("record_freq", sd_ihd, -1.0);
	if (*SampleRate < 0){
		fprintf(stderr, "ReadEntropicFile: Sample rate (%g) < 0.\n",
			*SampleRate);
		free(sd_ihd);
		return (float *) 0;
	}

	Channels = get_fea_siz("samples", sd_ihd, (short *)0, (long **)0);
	if (Channels > 1){
		fprintf(stderr, 
	    "ReadEntropicFile: Number of channels (%d) > 0 not supported.\n",
			Channels);
		free(sd_ihd);
		return (float *) 0;
	}

	*Length = sd_ihd->common.ndrec;
	
	if (MaxSamples >= 0 && *Length > MaxSamples) *Length = MaxSamples;

	Data = NewFloatArray((int32) *Length, "ReadEntropicFile");

	sd_feasd = allo_feasd_recs(sd_ihd, FLOAT, *Length, Data, NO);
	
	get_feasd_recs(sd_feasd, 0, *Length, sd_ihd, inputsd_strm);

	free(sd_feasd);
	free(sd_ihd);
	return Data;
}
	
#else

float	*ReadEntropicFile(FileName, Length, SampleRate)
char	*FileName;
int32	*Length;
float	*SampleRate;
{
	fprintf(stderr, "ReadEntropicFile not supported on this machine.\n");
	*SampleRate = *Length = 0;
	return (float *)0;
}

#endif	/* ESPS	 */

/*
 *	Attempt to read the GW Instruments MacSpeech file format.
 *
 *	The data is stored as 12 bit unsigned numbers.  Appendix D of
 *	the MacSpeech Lab II manual states that 0 corresponds to -10V,
 *	2048 is DC and 4095 is 10V.  Since it is a Macintosh format
 *	then the bytes are stored HighLow (big endian).
 *
 *	The first word contains an integer that represents the sample 
 *	rate.  The "normal" sample rate is 5.208Khz.  Other sample rates
 *	are given by this first integer multiplied by 5.208Khz.
 */
float	*ReadMacSpeechFile(FileName, Length, SampleRate)
char	*FileName;
int32	*Length;
float	*SampleRate;
{
	register int32	i;
	float	*Data;
	FILE	*fp;
	long len;

	fp = fopen(FileName, "r");
	if (!fp){
	    fprintf(stderr,"ReadMacSpeechFile: Couldn't open %s for reading.\n",
		    FileName);
	    exit(1);
	}

	len = FileLength(fp);		/* length in bytes */
	*Length = len/2 - 1;		/* Subtract first word from length */
	if (MaxSamples >= 0 && *Length > MaxSamples) *Length = MaxSamples;
	
	Data = NewFloatArray((int32) *Length, "ReadMacSpeechFile");
	
	*SampleRate = 5208 * Read16BitsHighLow(fp);
	
	for (i=0;i<*Length;i++) {
	    Data[i] = (Read16BitsHighLow(fp)-2048.0)/2048.0*InputGain;
	}
	
	return(Data);
}

char	*
GetFileSuffix(FileName)
char	*FileName;
{
	char *p;

	for (p=FileName;*p;p++);

	for (;*p != '.' && p >= FileName;p--);

	if (*p == '.')
		return(p+1);
	else 
		return(FileName);
}

float *
ReadInputFile(FileName, SamplingRate, Length)
char	*FileName;
float	*SamplingRate;
int32	*Length;
{
	char	*Suffix;
	float	*Data = NULL;

	Suffix = GetFileSuffix(FileName);
	if (strncmp(Suffix,"adc",3) == 0 || strncmp(Suffix,"ADC",3) == 0)
		Data = ReadADCFile(FileName,SamplingRate,Length);
	else if (strncmp(Suffix,"dac",3) == 0 || strncmp(Suffix,"DAC",3) == 0)
		Data = ReadDACFile(FileName,SamplingRate,Length);
	else if (strncmp(Suffix,"wav",3) == 0 || strncmp(Suffix,"WAV",3) == 0){
		Data = ReadWavFile(FileName,Length);
		*SamplingRate = 16000;
	} else if (strncmp(Suffix,"m22",3) == 0||strncmp(Suffix,"M22",3) == 0){
		*SamplingRate = 22254.5454545454545454;
		Data = ReadByteFile(FileName,Length);
	} else if (strncmp(Suffix,"m11",3) == 0||strncmp(Suffix,"M11",3) == 0){
		*SamplingRate = 22254.5454545454545454/2;
		Data = ReadByteFile(FileName,Length);
	} else if (strncmp(Suffix,"m7",2) == 0 || strncmp(Suffix,"M7",2) == 0){
		*SamplingRate = 22254.5454545454545454/3;
		Data = ReadByteFile(FileName,Length);
	} else if (strncmp(Suffix,"dy22",4)==0||strncmp(Suffix,"DY22",4)==0){
		*SamplingRate = 22050;
		Data = ReadDyaxisFile(FileName,Length);
	} else if (strncmp(Suffix,"dy44",4)==0||strncmp(Suffix,"DY44",4)==0){
		*SamplingRate = 44100;
		Data = ReadDyaxisFile(FileName,Length);
	} else if (strncmp(Suffix,"snd",3) == 0) {
		/* sampling rate comes from the file */
		Data = ReadNeXTFile(FileName,Length,SamplingRate);
	} else if ((strncmp(Suffix,"irc",3) == 0) ||
		   (strncmp(Suffix,"IRC",3) == 0)) {
		/* sampling rate comes from the file */
		Data = ReadIRCAMFile(FileName,Length,SamplingRate);
	} else if (strncmp(Suffix,"fl",2) == 0 || strncmp(Suffix,"FL",2) == 0){
		*SamplingRate = 16000;
		Data = ReadNativeFloatFile(FileName, Length);
	} else if (strncmp(Suffix,"aif",3) == 0||strncmp(Suffix,"AIF",3) == 0){
		Data = ReadAiffFile(FileName, Length, SamplingRate);
	} else if (strncmp(Suffix,"sd",2) == 0 || strncmp(Suffix,"SD",2) == 0){
		Data = ReadEntropicFile(FileName, Length, 
			SamplingRate);
	} else if (strncmp(Suffix,"macspeech",9) == 0 || 
					strncmp(Suffix,"MACSPEECH",9) == 0){
		Data = ReadMacSpeechFile(FileName, Length, 
			SamplingRate);
	} else {
		fprintf(stderr, "Don't know format for input file '%s'\n", 
			FileName);
		exit(1);
	}
#ifdef	VERBOSE
	picout("Input.output",Data,sizeof(*Data)* *Length);
#endif	/* VERBOSE */
	return(Data);
}


#ifdef	TESTEXTENDED

float	InputGain;
int32	MaxSamples;

float	*NewFloatArray(size, message)
int	size;
char	*message;
{
	return 0;
}

main(){
	float	f, ConvertIeeeExtendedToFloat();
	int	a, b, c, d, e;
	
	while (1){
		printf("Please Enter a Number to Convert: ");
		scanf("%g", &f);

		ConvertToIeeeExtended(f, &a, &b, &c, &d, &e);
		printf("%g is 0x%04x, %04x, %04x, %04x, %04x,", 
			f, a, b, c, d, e);
		fflush(stdout);
		f = ConvertIeeeExtendedToFloat(a, b, c, d, e);
		printf(" and back to %g.\n", f);
	}
}

#endif	TESTEXTENDED

