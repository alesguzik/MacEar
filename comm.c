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
 * $Header: comm.c,v 2.1 90/11/06 20:43:21 malcolm Exp $
 *
 * $Log:	comm.c,v $
 * Revision 2.1  90/11/06  20:43:21  malcolm
 * Changed copyright notice.
 * 
 * Revision 2.0  89/07/25  18:58:09  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.2  88/10/27  23:13:11  malcolm
 * Changed so that it was legal to specify a null value to a parameter.
 * An example of this is "if=".
 * 
 * Revision 1.1  88/10/23  22:38:49  malcolm
 * Initial revision
 * 
 *
 */

static char	*RCSid = "$Header: comm.c,v 2.1 90/11/06 20:43:21 malcolm Exp $";

/*
 *	This routine provides a simple means for searching for an argument
 *	name in a table.argument parsing.
 */
#include	<stdio.h>

comm(s,table)
char	*s;
char	*table[];
{
	register	int	i,j,r;

	for(i=0;table[i];i++){
		for (j=0;(r=table[i][j]) == s[j] && r;j++);
		if (r == 0 && s[j] == '=' )
			return(i+1);
	}
	fprintf(stderr,"bad option: %s\n",s);
	syntax();
	exit(1);
}
