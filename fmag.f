C 
C 			Lyon's Cochlear Model, The Program
C 	   			   Malcolm Slaney
C 			     Advanced Technology Group
C 				Apple Computer, Inc.
C 				 malcolm@apple.com
C 				   November 1988
C 
C 	This program implements a model of acoustic propagation and detection
C 	in the human cochlea.  This model was first described by Richard F.
C 	Lyon.  Please see 
C 		Malcolm Slaney, "Lyon's Cochlear Model, the Mathematica 
C 		Notebook," Apple Technical Report #13, 1988
C 	for more information.  This report is available from the Apple 
C 	Corporate Library.
C 
C 	Warranty Information
C 	Even though Apple has reviewed this software, Apple makes no warranty
C 	or representation, either express or implied, with respect to this
C 	software, its quality, accuracy, merchantability, or fitness for a 
C 	particular purpose.  As a result, this software is provided "as is,"
C 	and you, its user, are assuming the entire risk as to its quality
C 	and accuracy.
C 
C 	Copyright (c) 1988-1989 by Apple Computer, Inc
C 
C  $Header: fmag.f,v 2.0 89/07/25 18:58:40 malcolm Exp $
C 
C  $Log:	fmag.f,v $
c Revision 2.0  89/07/25  18:58:40  malcolm
c Completely debugged and tested version on the following machines (roughly
c in order of performance):
c Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
c both MPW and LightSpeed C.
c 
c Revision 1.1  88/10/23  22:42:00  malcolm
c Initial revision
c 
C 
C 
       subroutine fmag(real,im,n)
       integer n
       real real(0:n), im(0:n)

CDIR$ IVDEP
       do 10, i=0,n/2
		j = n - i
		firstreal =  ( real(i) + real(j) ) / 2.0
		firstim =    ( im(i)   - im(j)   ) / 2.0
		secondreal = ( im(i)   + im(j)   ) / 2.0
		secondim =   ( real(i) - real(j) ) / 2.0

		firstmag = firstreal*firstreal + firstim*firstim
		secondmag = secondreal*secondreal + secondim*secondim

		real(i) = firstmag
		real(j) = firstmag
		im(i) = secondmag
		im(j) = secondmag
  10   continue
       return
       end
