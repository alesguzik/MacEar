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
 *
 * $Header: filter.h,v 2.2 90/12/17 18:02:08 malcolm Exp $
 *
 * $Log:	filter.h,v $
 * Revision 2.2  90/12/17  18:02:08  malcolm
 * Added preprocessor check to make sure include file was only included once.
 * 
 * Revision 2.1  90/01/28  15:20:24  malcolm
 * Added definitions for new Group Delay functions.
 * 
 * Revision 2.0  89/07/25  18:58:39  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.2  88/12/06  21:14:10  malcolm
 * Added comments and cleaned up declarations.
 * 
 * Revision 1.1  88/10/23  22:41:26  malcolm
 * Initial revision
 * 
 *
 */

#ifndef	_FILTER_H
#define	_FILTER_H

/*
 *	A polynomial structure is used to store the coefficients to a 
 *	polynomial in the z variable.  A second degree polynomial will have
 *	the number 2 stored for the order and have an array of three (1,z,z^2)
 *	coefficients stored in the coeff array.
 */
struct polynomial {
	int	order;
	float	*coeff;
};

/*
 *	A filter is defined as the ratio of two polynomials.  In addition
 *	there is a scalar gain term.
 *
 *	The MakeFilter function allows the user to specify a target gain at
 *	any frequency.  The function MakeFilter multiplies the gain into all
 *	terms in the numerator.  Finally, the original sample_rate is kept 
 *	around to make it easier to evaluate the gain at any frequency.
 */
struct filter {
	struct polynomial *forward, *feedback;
	float	freq, gain;		/* Desired gain and frequency */
	float	sample_rate;		/* Sampling rate of filter */
};

float	FilterNumeratorCoeff(), FilterDenominatorCoeff();
float	EarBandwidth(), CascadeZeroCF(), CascadeZeroQ(), CascadePoleCF();
float	CascadePoleQ(), MaximumEarCF(), EarChannelCF();
float	EpsilonFromTauFS(), FilterGroupDelay(), PolyGroupDelay();

struct polynomial *AllocatePolynomial(), *CoeffsFromTauFS();
struct polynomial *CoeffsFromFCFS(), *CoeffsFromCFQFS();
complex	PolyEval(), RationalTransferFunction(), FilterEval(), FilterGain();

struct filter *MakeFilter(), *OuterMiddleEarFilter();
struct filter *EarFrontFilter(), *EarStageFilter();

#endif	/* _FILTER_H */
