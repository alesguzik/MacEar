#include	<stdio.h>
#include	<math.h>

#ifndef	M_PI
#define	M_PI	3.141592653589792434
#endif

FILE	*ifp;				/* Input File Name */

#define	COS_LENGTH	13
#define	OUTPUT_LENGTH	85

#define	VQ_SIZE		256

float	CosTransform[OUTPUT_LENGTH][COS_LENGTH];
float	InputGain;
int	MaxSamples;
char	*progname;

float	CodeBook[VQ_SIZE][COS_LENGTH];

main(argc, argv)
int	argc;
char	**argv;
{
	int	f, j, ByteCount, i, FrameCount;
	float	*Data, ReadIeeeFloatHighLow();
	float	*Output, max, min, scale, result;

	if (argc < 2){
		fprintf(stderr, "Syntax: %s input codebook > output\n", 
			argv[0]);
		exit(1);
	}

	progname = argv[0];
	ifp = fopen(argv[1], "r");
	ReadCodeBook(argv[2]);

	for (f=0;f<COS_LENGTH;f++){
		for (j=0;j<OUTPUT_LENGTH;j++){
			CosTransform[j][f] = cos(M_PI*f*j/OUTPUT_LENGTH);
		}
	}

	ByteCount = Read32BitsHighLow(ifp);

	Output = (float *)malloc(ByteCount*OUTPUT_LENGTH*sizeof(*Output));

	FrameCount = 0;
	while (ByteCount > 0){
		int	code;

		code = ReadByte(ifp) & 0xff;
		if (code < 0 || code >= VQ_SIZE){
			fprintf(stderr, 
			    "%s: Got a code (%d) out of the codebook (0->%d)\n",
			    progname, code, VQ_SIZE);
			exit(1);
		}

		Data = CodeBook[code];

		for (j=0;j<OUTPUT_LENGTH;j++){
			result = 0.0;
			for (i=0;i<COS_LENGTH;i++)
				result += Data[i] * CosTransform[j][i];
			
			Output[FrameCount*OUTPUT_LENGTH+j] = result;
		}
		ByteCount--;
		FrameCount++;
	}

	max = min = Output[0];
	for (i=0;i<FrameCount*OUTPUT_LENGTH;i++){
		if (Output[i] > max)
			max = Output[i];
		if (Output[i] < min)
			min = Output[i];
	}
	fprintf(stderr, "Min is %g, Max is %g.\n", min, max);

	scale = 255/(max-min);
	for (i=0;i<FrameCount*OUTPUT_LENGTH;i++){		
		putchar((int)floor((Output[i]-min)*scale));
	}
}

ReadCodeBook(name)
char	*name;
{
	int	i;
	FILE	*fp;

	fp = fopen(name, "r");
	if (!fp) {
		fprintf(stderr, "%s: Can't open codebook (%s).\n",
			progname, name);
		exit(1);
	}

					/* Note, unlike the ceptral files,
					 * the codebook files have a float
					 * count (instead of a byte count.)
					 */
	i = Read32BitsHighLow(fp);

	if (i != COS_LENGTH*VQ_SIZE){
		fprintf(stderr, "%s: Code is wrong size (%d), expected %d.\n",
			progname, i, COS_LENGTH*VQ_SIZE);
		exit(1);
	}

	for (i=0;i<VQ_SIZE;i++){
		int	j;
		for (j=0;j<COS_LENGTH;j++){
			CodeBook[i][j] = ReadIeeeFloatHighLow(fp);
		}
	}

	fclose(fp);
}

