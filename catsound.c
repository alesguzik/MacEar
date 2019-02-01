#ifdef	__STDC__
#include	<stdlib.h>
#endif	
#include	<stdio.h>
#include	<math.h>
#include	<ctype.h>

#define	PI		3.141592653589792434
#define	MAXSOUND	1000000			/* 1M should be enough */

int	CurrentSoundStart = 0, Length;
int	MaxSamples = -1;
float	InputGain = 1.0;
float	SamplingRate = 22254.5454545454;
float	Output[MAXSOUND];

main(argc, argv)
int	argc;
char	**argv;
{
	register int	i, j;
	float	*Buffer;

	for (i=1;i<argc;i++){
		if (strncmp(argv[i],"-bee",4) == 0){
			fprintf(stderr,"Adding a 0.25 second beep.\n");
			for (j=0;j<SamplingRate/4.0;j++)
				Output[CurrentSoundStart++] = 
					0.25*sin(j/SamplingRate*400*2*PI);
		}
		else if (strncmp(argv[i],"-sil",4) == 0){
			float	Length = 1.0;

			if (isdigit(argv[i+1][0])){
				Length = atof(argv[++i]);
			}
			fprintf(stderr, "Adding %g seconds of silence.\n",
						Length);
			CurrentSoundStart += Length*SamplingRate;
		}
		else {
			extern float	*ReadInputFile();
			fprintf(stderr,"Reading %s ", argv[i]);
			fflush(stderr);
			Buffer = ReadInputFile(argv[i], &SamplingRate, &Length);
			fprintf(stderr, "(%g seconds at %g samples/sec.)\n", 
				Length/SamplingRate, SamplingRate);
			for (j=0;j<Length;j++)
				Output[CurrentSoundStart++] = Buffer[j];
			free(Buffer);
		}
	}

	WriteAiffFile("-",SamplingRate, CurrentSoundStart, Output, 1);
/*
	WriteADCFile("-", SamplingRate, CurrentSoundStart, Output);
	WriteByteFile("-", CurrentSoundStart, Output);
*/
}

float	*
NewFloatArray(size,usage)
int	size;
char	*usage;
{
	float	*p;

	p = (float *)calloc(sizeof(*p),size);
	if (!p){
		fprintf(stderr,"%s: Can't allocate %ld floats.\n", size);
		exit(1);
	}
	return p;
}

