#include	<stdio.h>
#include	<math.h>

#ifndef	M_PI
#define	M_PI	3.141592653589792434
#endif

FILE	*ifp;				/* Input File Name */

#define	COS_LENGTH	13
#define	OUTPUT_LENGTH	85

float	CosTransform[OUTPUT_LENGTH][COS_LENGTH];
float	InputGain;
int	MaxSamples;
char	**progname;

main(argc, argv)
int	argc;
char	**argv;
{
	int	f, j, ByteCount, i, FrameCount;
	float	Data[COS_LENGTH], ReadIeeeFloatHighLow();
	float	*Output, max, min, scale, result;

	if (argc < 2){
		fprintf(stderr, "Syntax: %s input > output\n", 
			argv[0]);
		exit(1);
	}

	ifp = fopen(argv[1], "r");

	if (!ifp){
		fprintf(stderr, "Can't open %s for reading input.\n", argv[1]);
		exit(1);
	}

	for (f=0;f<COS_LENGTH;f++){
		float	gain = 1.0;
		if (f != 0)
			gain = 2.0;
		for (j=0;j<OUTPUT_LENGTH;j++)
			CosTransform[j][f] = gain*cos(M_PI*f*j/OUTPUT_LENGTH);
	}

	ByteCount = Read32BitsHighLow(ifp);

	Output = (float *)malloc(ByteCount/4/COS_LENGTH*OUTPUT_LENGTH*
					sizeof(*Output));

	FrameCount = 0;
	while (ByteCount > 0){
		for (i=0;i<COS_LENGTH;i++)
			Data[i] = ReadIeeeFloatHighLow(ifp);

		for (j=0;j<OUTPUT_LENGTH;j++){
			result = 0.0;
			for (i=0;i<COS_LENGTH;i++)
				result += Data[i] * CosTransform[j][i];
			
			Output[FrameCount*OUTPUT_LENGTH+j] = result;
		}
		ByteCount -= 4 * COS_LENGTH;
		FrameCount++;
	}

	max = min = Output[i];
	for (i=0;i<FrameCount*OUTPUT_LENGTH;i++){
		if (Output[i] > max)
			max = Output[i];
		if (Output[i] < min)
			min = Output[i];
	}

	scale = 255/(max-min);
	for (i=0;i<FrameCount*OUTPUT_LENGTH;i++){		
		putchar((int)floor((Output[i]-min)*scale));
	}
}

