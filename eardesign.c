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
 * $Header: eardesign.c,v 2.1 90/11/06 20:45:34 malcolm Exp $
 *
 * $Log:	eardesign.c,v $
 * Revision 2.1  90/11/06  20:45:34  malcolm
 * Added calculation of the group delay.  Also moved center frequency
 * calculations into the filter specific files (sos or hydro).
 * 
 * Revision 2.0  89/07/25  18:58:26  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.7  89/07/19  12:41:46  malcolm
 * Changed MaxN to MaxChannels.  Also moved the ear filter bank parameter
 * storage to the utilities.c file.
 * 
 * Revision 1.6  89/04/09  17:06:47  malcolm
 * Added debugging code.
 * 
 * Revision 1.5  89/02/24  22:57:08  malcolm
 * Changed definition of atof() from float (wrong) to double.
 * 
 * Revision 1.4  88/12/06  21:11:56  malcolm
 * Fixed up declarations for the Macintosh.
 * 
 * Revision 1.3  88/10/27  23:15:08  malcolm
 * Changed most function names to agree with the Mathematica notebook.
 * Cleaned up some code, added lots of comments and made sure that results
 * agreed with Mathematica results.
 * 
 * Revision 1.2  88/10/23  23:08:19  malcolm
 * Made compatible with ANSI C (removed pragma's and endif comments).
 * 
 * Revision 1.1  88/10/23  22:39:59  malcolm
 * Initial revision
 * 
 *
 */

static char	*RCSid = "$Header: eardesign.c,v 2.1 90/11/06 20:45:34 malcolm Exp $";

/*
 *	This file designs the filters used to model the cochlea.
 *	Compile with -DMAIN to test these functions.
 */

#include	<stdio.h>
#include	"complex.h"
#include	"filter.h"
#include	"ear.h"

/*
 *	AllocatePolynomial - Create a polynomial structure of the given order
 *	(leave room for order+1 coefficients.)
 */
struct polynomial *AllocatePolynomial(order)
int	order;
{
	struct polynomial *pp;

	pp = (struct polynomial *)calloc(1,sizeof(*pp));
	if (!pp){
		fprintf(stderr, 
		"AllocatePolynomial: Couldn't allocate polynomial structure.\n"
			);
		exit(1);
	}
	pp->order = order;
	pp->coeff = (float *)calloc(order+1, sizeof(float));
	if (!pp->coeff){
	    fprintf(stderr, 
	    "AllocatePolynomial: Couldn't allocate space for polynomial list.\n"
		);
	    exit(1);
	}
	pp->coeff[order] = 1.0;
	return(pp);
}

/*
 *	PrintPolynomial - Pretty print a polynomial structure.
 */
PrintPolynomial(pp)
struct polynomial *pp;
{
	register int	i;

	if (pp) {
		for (i=0;i<pp->order+1;i++){
			if (i == 0 || pp->coeff[i] != 1.0)
				printf("%g", pp->coeff[i]);
			if (i == 1)
				printf("z");
			else if (i > 1)
				printf("z**%d",i);
			if (i != pp->order)
				printf(" + ");
		}
	} else
		printf("1");
}

/*
 *	Evaluate a polynomial (in Z) for any complex value of z.  If the
 *	polynomial is null then return 1.0.
 */
complex PolyEval(pp, x)
struct polynomial *pp;
complex	x;
{
	register int	i;
	complex xpow, y;

	if (!pp)
		return (cmplx(1.0,0.0));

	xpow = cmplx(1.0,0.0);
	y = cmplx(0.0,0.0);

	for (i=0;i<pp->order+1;i++){
		y = cadd(y,cmul(xpow,cmplx(pp->coeff[i],0.0)));
		xpow = cmul(xpow,x);
	}
	return(y);
}

/*
 *	MakeFilter - Create a filter from feed forward (zeros) and feedback
 *	(poles) polynomials.  The user specifies the sampling rate (after all
 *	we are in the z domain) and a frequency and desired gain at that 
 *	frequency.  The coefficients of all numerator terms are adjusted so
 *	that the filter has the proper gain.
 */
struct filter *MakeFilter(forward, feedback, fsamp, freq, gain)
struct polynomial *forward, *feedback;
float	fsamp, freq, gain;
{
	struct filter *fp;
	int	i;


#ifdef  DEBUG
	printf("MakeFilter called with (");
	PrintPolynomial(forward);
	printf(", ");
	PrintPolynomial(feedback);
	printf(", %g, %g, %g\n", fsamp, freq, gain);
#endif  /* DEBUG */


	fp = (struct filter *)calloc(1,sizeof(*fp));
	if (!fp){
		fprintf(stderr, 
		"MakeFilter: Couldn't allocate coefficient structure.\n");
		exit(1);
	}
	if (forward)
		fp->forward = forward;
	else
		fp->forward = forward = AllocatePolynomial(0);
	if (feedback)
		fp->feedback = feedback;
	else
		fp->feedback = feedback = AllocatePolynomial(0);
	fp->sample_rate = fsamp;
	fp->gain = gain;
	fp->freq = freq;
	gain = gain / cmag(FilterGain(fp,fp->freq));
	for (i=0;i<forward->order+1;i++)
		forward->coeff[i] *= gain;
	return(fp);
}

/*
 *	FilterEval - Evaluate the value of a filter function (ratio of 
 *	polynomials) at a given z.
 *
 *	We use the temporary variables a and b to make it simpler on the
 *	compiler.  At least two compilers I have tried this on (Cray and
 *	Mac MPW) can't handle the more direct approach.
 */
complex FilterEval(fp, z)
struct filter *fp;
complex	z;
{
	complex a,b;
	a = PolyEval(fp->forward,z);
	b = PolyEval(fp->feedback,z);
	return(cdiv(a,b));
}

/*	FilterGain - Evaluate the gain of a filter function (ratio of 
 *	polynomials in z) at a given frequency.
 */
complex FilterGain(fp, freq)
struct filter *fp;
float	freq;
{
	return(FilterEval(fp, cis(2*PI*freq/fp->sample_rate)));
}

complex	EvalZPolyDerivative(polyp, freq, sample_rate)
struct	polynomial *polyp;
float	freq, sample_rate;
{
	int	i;
	complex 	z, ZtotheN, result;

	if (polyp->order == 0){
		return(cmplx(0.0,0.0));
	} else {

		ZtotheN = cmplx(1.0,0.0);
		z = cis(freq/sample_rate*2*PI);
		result = cmplx(0.0,0.0);
		for (i=1;i<=polyp->order; i++){
			result = cadd(result,
				      cmul(cmplx(polyp->coeff[i]*i,0.0),
					   ZtotheN));
			ZtotheN = cmul(ZtotheN,z);
		}
		return(result);
	}
}

/*
 * 	FilterGroupDelay - This function computes the group delay of a 
 *		digital filter using the following expression from
 *		page 210 of Rabiner and Gold.  See page 112 of my Apple 
 *		1989/1990 log book for a more complete derivation.
 *						 |
 *			          [ z dH(z)/dz ] |
 *			tau = - Re[------------] |
 *				  [    H(z)    ] |    jw
 *						 | z=e
 *		Remember that the derivative of u/v is given by
 *				v du - u dv
 *				-----------
 *				    v v
 */

float	FilterGroupDelay(fp, freq, sample_rate)
struct filter *fp;
float	freq, sample_rate;
{
	complex	u, v, du, dv, z, h, hprime;

	z = cis(freq/sample_rate*2*PI);
	u = PolyEval(fp->forward,z);
	v = PolyEval(fp->feedback,z);
	du = EvalZPolyDerivative(fp->forward,freq,sample_rate);
	dv = EvalZPolyDerivative(fp->feedback,freq,sample_rate);

	h = cdiv(u,v);
	hprime = cdiv(csub(cmul(v,du),
			   cmul(u,dv)),
		      cmul(v,v));
	return -real(cdiv(cmul(z,hprime),
			 h));
}

/*
 *	FilterNumeratorCoeff - Return the i'th coefficient of the numerator
 *	of a filter.
 */
float
FilterNumeratorCoeff(fp,i)
struct filter *fp;
int	i;
{
	struct polynomial *pp;

	pp = fp->forward;
	if (i <= pp->order)
		return(pp->coeff[i]);
	else
		return(0.0);
}

/*
 *	FilterDenominatorCoeff - Return the i'th coefficient of the denominator
 *	of a filter.
 */
float
FilterDenominatorCoeff(fp,i)
struct filter *fp;
int	i;
{
	struct polynomial *pp;

	pp = fp->feedback;
	if (i <= pp->order)
		return(pp->coeff[i]);
	else
		return(0.0);
}

/*
 *	PrintFilter - Pretty print a filter structure.
 */
PrintFilter(fp)
struct filter *fp;
{
	float gain;

	printf("\t");
	PrintPolynomial(fp->forward);
	printf(" / ");
	PrintPolynomial(fp->feedback);
	printf("\n");
	printf("\tSampling rate of %g, desired gain is %g at %g hz.\n",
		fp->sample_rate, fp->gain, fp->freq);
	gain = cmag(FilterGain(fp, fp->freq));
	printf("\tActual gain is %g at %g.\n", gain, fp->freq);
}

/*
 *	The following is a bunch of parameters that describe the ear model.
 *	See the technical report for details and explanations.
 */
extern float EarBreakFreq;
extern float EarQ;
extern float EarStepFactor;
extern float EarZeroOffset;
extern float EarSharpness;
extern float EarPreemphCorner;

/*
 *	EarBandwidth - How wide are the poles of a stage in the ear filter.
 *	The result is in Hz.
 */
float EarBandwidth(cf)
float	cf;
{
	return(sqrt(cf*cf + EarBreakFreq*EarBreakFreq)/EarQ);
}
	
/*
 *	CascadeZeroCF - What frequency are the zeros at in a stage of the
 *	ear filter.  The result is a small factor of the step size between 
 *	channels (EarBandwidth * EarStepFactor)
 */
float CascadeZeroCF(cf)
float	cf;
{
	return( cf + EarBandwidth(cf) * EarStepFactor * EarZeroOffset);
}

/*
 *	CascadeZeroQ - What is the quality factor of a zero in a stage
 *	of the ear filter.
 */
float CascadeZeroQ(cf)
float	cf;
{
	return(EarSharpness * CascadeZeroCF(cf) / EarBandwidth(cf));
}

/*
 *	CascadePoleCF - What is the center frequency of the poles of each
 *	stage of the cascade filter bank?
 */
float CascadePoleCF(cf)
float	cf;
{
	return(cf);
}

/*
 *	CascadePoleQ - What is the quality factor of the poles?
 */
float CascadePoleQ(cf)
float	cf;
{
	return(cf / EarBandwidth(cf));
}

/* EpsilonFromTauFS
 *	Compute coefficient in follower-integrator filter (one minus pole)
 */

float EpsilonFromTauFS(tau, fs)
float	tau, fs;
{
	return (1.0 - exp(-1.0/tau/fs));
}

/* 
 *	FirstOrderFromTau - Polynomial coefficient list for first-order 
 *	pole or zero with time constant tau.
 */
struct polynomial *FirstOrderFromTau(tau, fs)
float	tau, fs;
{
	struct polynomial *pp = AllocatePolynomial(1);

	pp->coeff[0] = EpsilonFromTauFS(tau, fs) - 1.0;
	pp->coeff[1] = 1.0;
	return pp;
}

/* 
 *	FirstOrderFromCorner - Polynomial coefficient list for first-order 
 *	pole or zero with corner frequency fc.
 */
struct polynomial *FirstOrderFromCorner(fc, fs)
float	fc, fs;
{
	float fct, rho;
	struct polynomial *pp = AllocatePolynomial(1);

	fct = fc / fs;
	rho = exp(-2.0*PI*fct);
	pp->coeff[0] = - rho;
	pp->coeff[1] = 1.0;
	return(pp);
}

/* 
 *	SecondOrderFromCenterQ - Polynomail coefficient list for second-order 
 *	complex poles or zeros
 */
struct polynomial *SecondOrderFromCenterQ(NatFreq, q, fs)
float	NatFreq, q, fs;
{
	float cft, rho, theta;
	struct polynomial *pp = AllocatePolynomial(2);

	cft = NatFreq/fs;
	rho = exp(-PI * cft / q);
	theta = 2 * PI * cft * sqrt(1 - 0.25/q/q);
	pp->coeff[0] = rho*rho;
	pp->coeff[1] = -2.0 * rho * cos(theta);
	pp->coeff[2] = 1.0;
	return(pp);
}

/* 
 *	OuterMiddleEar - Create a filter that roughly models the effect of the
 *	outer and middle ears.  In this case it is just a simple high pass
 *	filter with a corner frequency given by EarPreemphCorner.
 */
struct filter *OuterMiddleEarFilter(fs)
float	fs;
{
	struct filter *fp;

	fp = MakeFilter(FirstOrderFromCorner(EarPreemphCorner, fs),
		        NULL,
		        fs,
		        fs/4.0,
		        1.0);
	return(fp);
}

/*
 *	EarFrontFilter - Create a filter to handle the initial processing
 *	before the cascade of filters.  This filter has the following terms
 *	1)	A zero at DC (differentiator) to model the conversion of 
 *		acoustic pressure into basilar membrane motion.
 *	2)	A zero at the Nyquist rate to compensate for the narrow
 *		spacing of channels at the high frequencies.
 *	3)	The first pair of poles from the cascade of ear filters.
 */
struct filter *EarFrontFilter(fs)
float	fs;
{
	struct filter *fp;
	struct polynomial *pp;

	pp = AllocatePolynomial(2);	/* Differentiator and Compensator */
	pp->coeff[0] = -1.0;
	pp->coeff[1] = 0.0;
	pp->coeff[2] = 1.0;

	fp = MakeFilter(pp,
			SecondOrderFromCenterQ(MaximumEarCF(fs),
					CascadePoleQ(MaximumEarCF(fs)),
					fs),
			fs,
			fs/4.0,
			1.0);
	return(fp);
}

/*
 *	EarStageFilter - OK, here it is....the definition of each stage of 
 *	the cascade-only filter bank.  Each stage is just a pair of poles
 *	and a pair of zeros.
 */
struct filter *EarStageFilter(cf,fs,DCGain)
float	fs, cf, DCGain;
{
	struct filter *fp;

	fp = MakeFilter(SecondOrderFromCenterQ(CascadeZeroCF(cf),
					CascadeZeroQ(cf),
					fs),
			SecondOrderFromCenterQ(CascadePoleCF(cf),
					CascadePoleQ(cf),
					fs),
		        fs,
		        0.0,
		        DCGain);
	return(fp);
}

