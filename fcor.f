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
C  $Header: fcor.f,v 2.2 90/12/17 18:01:01 malcolm Exp $
C 
C  $Log:	fcor.f,v $
c Revision 2.2  90/12/17  18:01:01  malcolm
c Removed ALIMIT subroutine definition.
c 
c Revision 2.1  89/11/09  23:15:37  malcolm
c Fixed minor problem in the test for zero data in the correlation output.
c 
c Revision 2.0  89/07/25  18:58:32  malcolm
c Completely debugged and tested version on the following machines (roughly
c in order of performance):
c Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
c both MPW and LightSpeed C.
c 
c Revision 1.4  89/07/19  17:01:49  malcolm
c Put autotasking code into comment.  Added alimit routine.  Also changed
c check for 0 to check for less than 1e-10.
c 
c Revision 1.1  88/10/23  22:40:39  malcolm
c Initial revision
c 
C 
C 
      subroutine fcor(input, output, window, work, lags, channels,
     1          datalength, fftlength, index, normalize)
      integer lags, datalength, fftlength, channels
      real input(datalength,channels), output(lags, channels)
      real window(datalength), normalize
      complex work(3*fftlength)
       
      complex fftdata(fftlength+1)
      integer channel, i, j
      real temp, firstreal, secondreal, firstim, secondim
      real firstscale, secondscale
      complex workx(3*fftlength)

      do 1 i=1,3*fftlength
    1   workx(i) = work(i)

c     do 5 i=1,datalength
c       input(i,21) = 1+sin(i*3.14159265/10.0)
c       input(i,22) = 0.0
c       input(i,23) = 1+sin(i*3.14159265/20.0)
c       input(i,24) = 0.0
c   5 continue

C	The CMIC$ directives enable microtasking of the loop.
C CMIC$ DO ALL PRIVATE(workx, fftdata, channel, i, j, temp, firstreal,
C      1               secondreal, firstim, secondim, firstscale,
C      2               secondscale)
CMIC$ DO GLOBAL
      do 100 channel = 1,channels,2
C                                       Fill the data into the fft arrrays
          do 5 i=index+1,datalength
              fftdata(i-index) = cmplx(input(i,channel),
     1                             input(i,channel+1))
    5     continue
          do 10 i=1,index
              fftdata(datalength-index+i) = cmplx(input(i,channel), 
     1                            input(i,channel+1))
   10     continue
C                                       Multiply by the window
          do 15 i=1,datalength
              fftdata(i) = fftdata(i) * window(i)
   15     continue
C                                       Zero pad the data
          do 20 i = datalength+1,fftlength
              fftdata(i) = cmplx(0.0,0.0)
   20     continue

          call cfft2(0,1,fftlength,fftdata,workx,fftdata)

          fftdata(fftlength+1) = fftdata(1)

CDIR$ IVDEP
          do 30, i=1,fftlength/2+1
              j = fftlength - i + 2
              firstreal = (real(fftdata(i))+real(fftdata(j)))/2.0
              firstim = (aimag(fftdata(i))-aimag(fftdata(j)))/2.0
              secondreal = (aimag(fftdata(i))+aimag(fftdata(j)))/2.0
              secondim = (real(fftdata(i))-real(fftdata(j)))/2.0

              firstmag = firstreal*firstreal + firstim*firstim
              secondmag = secondreal*secondreal + secondim*secondim

              fftdata(i) = cmplx(firstmag,secondmag)
              fftdata(j) = cmplx(firstmag,secondmag)
  30      continue

          call cfft2(0,-1,fftlength,fftdata,workx,fftdata)

C
C	The extra factor of fftlength is necessary to compensate for the
C	extra factors of N that are in the FFT.  We want to divide it out
C	before we do the normalize.
C
	  do 35 i=1,fftlength
	      fftdata(i) = fftdata(i)/fftlength
  35	  continue
        
C
C	Now figure out the scaling factor to use.  If the zero lag case is
C	too small or if it is less than the lag=1 and lag=2 case than we
C	we were probably correlating noise so we scale by zero.  Otherwise
C	we divide all terms of the correlation output by the zero lag term
C	raised to the normalize parameter.  The default value of this parameter
C	is .75 so there is some small compression going on.
C
          temp = real(fftdata(1))
          if (temp .le. 1.0e-10 .or.
     1        temp .lt. real(fftdata(2)) .or.
     2        temp .lt. real(fftdata(3))) then
              firstscale = 0.0
          else
	      firstscale = 1.0/(temp**normalize)
          endif

          temp = aimag(fftdata(1))
          if (temp .le. 1.0e-10 .or.
     1        temp .lt. aimag(fftdata(2)) .or.
     2        temp .lt. aimag(fftdata(3))) then
              secondscale = 0.0
          else
	      secondscale = 1.0/(temp**normalize)
          endif

C	Just for testing....
C         firstscale = 1.0
C         secondscale = 1.0
C
          do 40 i=1,lags
              output(i,channel) = real(fftdata(i)) * firstscale
              output(i,channel+1) = aimag(fftdata(i)) * secondscale
   40     continue
  100 continue

      return
      end

      function amax(array,n)
      integer i, n
      real array(n), amax
      i = ismax(n,array,1)
      amax = array(i)
      return
      end

      function amin(array,n)
      integer i, n
      real array(n), amin
      i = ismin(n,array,1)
      amin = array(i)
      return
      end

C
C	Simple vector move subroutine in fortran so that it will vectorize
C	on the Cray.
C
      subroutine vmov(source,sstride,n,dest,dstride)
      real source(*), dest(*)
      integer sstride, n, dstride
      integer i

      do 10 i=0,n-1
	 dest(i*dstride+1) = source(i*sstride+1)
   10 continue

      return
      end


