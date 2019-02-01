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
C 	Copyright (c) 1988-1990 by Apple Computer, Inc
C 
C  $Header: ftopix.f,v 2.2 90/12/17 18:02:27 malcolm Exp $
C 
C  $Log:	ftopix.f,v $
c Revision 2.2  90/12/17  18:02:27  malcolm
c Cleaned up and moved ALIMIT subroutine to this file.
c 
c Revision 2.1  90/01/28  15:37:01  malcolm
c Added capability (MRKPXL subroutine) so that an arbitrary pixel in the
c image can be marked with red.
c 
c Revision 2.0  89/07/25  18:58:42  malcolm
c Completely debugged and tested version on the following machines (roughly
c in order of performance):
c Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
c both MPW and LightSpeed C.
c 
c Revision 1.4  89/07/19  17:04:47  malcolm
c Added code to expand picture by different amounts in x and y.
c 
c Revision 1.3  89/03/24  15:02:39  malcolm
c Everything turned upside down.  Top and bottom of the picture were
c flipped so that high frequencies (low channel numbers) are at the top.
c Also, high energy points in the picture are now black like they are in
c a conventional spectrogram.
c 
c Revision 1.2  88/12/06  21:14:54  malcolm
c Added comments and rearranged declarations
c 
c Revision 1.1  88/10/23  22:42:47  malcolm
c Initial revision
c 
C 
C 
C***********************************************************************
C	FTOPIX - Routine to convert an array of floats into pixels for 
C	the ultra frame buffer.  This routine takes as arguments an
C	incore frame buffer (dest), an array of floating point numbers
C	(source), the width and height of the data and the min and max
C	of the data.  No checks are made that the data doesn't exceed
C	the limits passed as arguments.
C
C	The incore frame buffer is an array of integers (64 bits each).
C	Each word of the frame buffer holds two pixels.  The format of the
C	data in the word is as follows
C		|			64 bit word			     |
C		| Left Pixel (32 bits)	    |   Right Pixel (32 bits)	     |
C		| Empty|Blue |  Green | Red |	Empty | Blue |  Green | Red  |
C	Each of the RGB values is one byte.  The magic number 0x10101 is 
C	used to take a single gray scale level (between 0 and 255) and to
C	fill in the Red, Green and Blue values in the word.
C
C	Finally, one more performance hack.  It is necessary to shift the
C	left pixel into place instead of incorporating the shift into the
C	multiplication by 0x10101 because Cray integer arithmetic is only
C	48 bits long.
C
C	The UltraBuffer is represented in memory as a array of words in TV
C	raster order (top left pixel first).  This routine puts the image
C	centered on the screen with the first byte of the input array at the
C	lower left of the screen.
C
C	The CMIC$ directives enable microtasking of the loop.
C
C	The magic numbers that are used here are:
C	255 - Maximum value for a pixel
C	1280 - Width in pixels of the ultra display
C	1024 - Height in pixels of the ultra display
C	65793 - Equal to 0x10101 - Needed to convert gray scale level into RGB
C

CMIC$ MICRO
      subroutine FTOPIX(dest, source, width, height, minimum, maximum)
      integer dest(1280/2,*), i, j, width, height, expand
      integer xexpand, yexpand, oldpixel, mask, newpixel
      real source(width,height), minimum, maximum, scale, minvalue
      common /PIXEL/ markedx, markedy

C	For high resolution monitor
C     parameter (nwidth=1280, nheight=1024)	
C	For NTSC resolution monitor
C     parameter (nwidth=750, nheight=484)

      parameter (nwidth=780, nheight=480)
C     parameter (nwidth=1280, nheight=1024)	

C
C	There is a special hack installed in this version of ftopix that
C	allows the user to mark a pixel in the image with a bright color.
C	This color is specified by the following parameter.  (It is just
C	jammed into the pixel after putting everything else in place.)
      parameter (color=255)

c     scale = maximum
c     maximum = minimum
c     minimum = scale
c     print *,maximum, minimum, scale

      scale = 255.0/(maximum - minimum)

      minvalue = maximum
      scale = 255.0/(minimum - maximum)

      xexpand = 1
      if (width*2 .lt. nwidth) xexpand = 2
      if (width*4 .lt. nwidth) xexpand = 4

      yexpand = 1
      if (height*2 .lt. nheight) yexpand = 2
      if (height*4 .lt. nheight) yexpand = 4

      expand = yexpand
      if (xexpand .lt. yexpand) expand = xexpand

      if (expand .eq. 1) then
        if (xexpand .eq. 1 .and. yexpand .ge. 2) then
CMIC$ DO GLOBAL
          do 300 j = 1, height
CDIR$ IVDEP
             do 400 i = 1, width, 2
              dest(nwidth/2/2-width/2/2+i/2, nheight/2-height+2*j) =
     +         SHIFTL(INT((source(i+1,j)-minvalue)*scale)*65793, 32).OR.
     +                INT((source(i  ,j)-minvalue)*scale)*65793
              dest(nwidth/2/2-width/2/2+i/2, nheight/2-height+2*j+1) =
     +         SHIFTL(INT((source(i+1,j)-minvalue)*scale)*65793, 32).OR.
     +                INT((source(i  ,j)-minvalue)*scale)*65793
  400        continue
  300      continue
	   if (markedx .gt. 0 .and. markedx .le. width .and. 
     +         markedy .gt. 0 .and. markedy .le. height) then
	     if (mod(markedx,2) .eq. 1) then
	       newpixel = SHIFTL(color,32)
	       mask = 16777215
	     else
	       newpixel = color
	       mask = SHIFTL(16777215,32)
	     endif

	     oldpixel = dest(nwidth/2/2-width/2/2+markedx/2,
     +                       nheight/2-height+2*markedy)
	     dest(nwidth/2/2-width/2/2+markedx/2, 
     +            nheight/2-height+2*markedy) = 
     +                  (mask .and. oldpixel) + color
	     oldpixel = dest(nwidth/2/2-width/2/2+markedx/2,
     +                       nheight/2-height+2*markedy+1)
	     dest(nwidth/2/2-width/2/2+markedx/2, 
     +            nheight/2-height+2*markedy+1) = 
     +                  (mask .and. oldpixel) + color
	   endif
         else
	   
CMIC$ DO GLOBAL
           do 1100 j = 1, height
CDIR$ IVDEP
            do 1200 i = 1, width, 2
              dest(nwidth/2/2-width/2/2+i/2, nheight/2-height/2+j) =
     +         SHIFTL(INT((source(i+1,j)-minvalue)*scale)*65793,32).OR.
     +                INT((source(i  ,j)-minvalue)*scale)*65793
 1200       continue
 1100      continue
	   if (markedx .gt. 0 .and. markedx .le. width .and. 
     +         markedy .gt. 0 .and. markedy .le. height) then
	     if (mod(markedx,2) .eq. 1) then
	       newpixel = SHIFTL(color,32)
	       mask = 16777215
	     else
	       newpixel = color
	       mask = SHIFTL(16777215,32)
	     endif

	     oldpixel = dest(nwidth/2/2-width/2/2+markedx/2,
     +                       nheight/2-height/2+markedy)
	     dest(nwidth/2/2-width/2/2+markedx/2, 
     +            nheight/2-height/2+markedy) = 
     +                  (mask .and. oldpixel) + color
	   endif
	 endif
       endif

       if (expand .eq. 2) then
CMIC$ DO GLOBAL
         do 2100 j = 1, height
CDIR$ IVDEP
            do 2200 i = 1, width
             dest(nwidth/2/2-width/2+i, nheight/2-height+2*j) =
     +          SHIFTL(INT((source(i,j)-minvalue)*scale)*65793, 32).OR.
     +                 INT((source(i,j)-minvalue)*scale)*65793
             dest(nwidth/2/2-width/2+i, nheight/2-height+2*j+1) =
     +          SHIFTL(INT((source(i,j)-minvalue)*scale)*65793, 32).OR.
     +                 INT((source(i,j)-minvalue)*scale)*65793
 2200       continue
 2100    continue
	 if (markedx .gt. 0 .and. markedx .le. width .and. 
     +       markedy .gt. 0 .and. markedy .le. height) then
	   dest(nwidth/2/2-width/2+markedx,
     +          nheight/2-height+2*markedy) =
     +         SHIFTL(color,32) + color
	   dest(nwidth/2/2-width/2+markedx,
     +          nheight/2-height+2*markedy+1) =
     +         SHIFTL(color,32) + color
	 endif
       endif

       if (expand .eq. 4) then
CMIC$ DO GLOBAL
         do 4100 j = 1, height
CDIR$ IVDEP
            do 4200 i = 1, width
             dest(nwidth/2/2-width+2*i, nheight/2-2*height+4*j) =
     +          SHIFTL(INT((source(i,j)-minvalue)*scale)*65793, 32).OR.
     +                 INT((source(i,j)-minvalue)*scale)*65793
             dest(nwidth/2/2-width+2*i, nheight/2-2*height+4*j+1) =
     +          SHIFTL(INT((source(i,j)-minvalue)*scale)*65793, 32).OR.
     +                 INT((source(i,j)-minvalue)*scale)*65793
             dest(nwidth/2/2-width+2*i, nheight/2-2*height+4*j+2) =
     +          SHIFTL(INT((source(i,j)-minvalue)*scale)*65793, 32).OR.
     +                 INT((source(i,j)-minvalue)*scale)*65793
             dest(nwidth/2/2-width+2*i, nheight/2-2*height+4*j+3) =
     +          SHIFTL(INT((source(i,j)-minvalue)*scale)*65793, 32).OR.
     +                 INT((source(i,j)-minvalue)*scale)*65793
             dest(nwidth/2/2-width+2*i+1, nheight/2-2*height+4*j) =
     +          SHIFTL(INT((source(i,j)-minvalue)*scale)*65793, 32).OR.
     +                 INT((source(i,j)-minvalue)*scale)*65793
             dest(nwidth/2/2-width+2*i+1, nheight/2-2*height+4*j+1) =
     +          SHIFTL(INT((source(i,j)-minvalue)*scale)*65793, 32).OR.
     +                 INT((source(i,j)-minvalue)*scale)*65793
             dest(nwidth/2/2-width+2*i+1, nheight/2-2*height+4*j+2) =
     +          SHIFTL(INT((source(i,j)-minvalue)*scale)*65793, 32).OR.
     +                 INT((source(i,j)-minvalue)*scale)*65793
             dest(nwidth/2/2-width+2*i+1, nheight/2-2*height+4*j+3) =
     +          SHIFTL(INT((source(i,j)-minvalue)*scale)*65793, 32).OR.
     +                 INT((source(i,j)-minvalue)*scale)*65793
 4200       continue
 4100    continue
	 if (markedx .gt. 0 .and. markedx .le. width .and. 
     +       markedy .gt. 0 .and. markedy .le. height) then
	   dest(nwidth/2/2-width+2*markedx,
     +          nheight/2-2*height+4*markedy) =
     +         SHIFTL(color,32) + color
	   dest(nwidth/2/2-width+2*markedx+1,
     +          nheight/2-2*height+4*markedy) =
     +         SHIFTL(color,32) + color
	   dest(nwidth/2/2-width+2*markedx,
     +          nheight/2-2*height+4*markedy+1) =
     +         SHIFTL(color,32) + color
	   dest(nwidth/2/2-width+2*markedx+1,
     +          nheight/2-2*height+4*markedy+1) =
     +         SHIFTL(color,32) + color
	   dest(nwidth/2/2-width+2*markedx,
     +          nheight/2-2*height+4*markedy+2) =
     +         SHIFTL(color,32) + color
	   dest(nwidth/2/2-width+2*markedx+1,
     +          nheight/2-2*height+4*markedy+2) =
     +         SHIFTL(color,32) + color
	   dest(nwidth/2/2-width+2*markedx,
     +          nheight/2-2*height+4*markedy+3) =
     +         SHIFTL(color,32) + color
	   dest(nwidth/2/2-width+2*markedx+1,
     +          nheight/2-2*height+4*markedy+3) =
     +         SHIFTL(color,32) + color
	 endif
       endif

      return
      end

      subroutine MRKPXL(x,y)
      common /PIXEL/ markedx, markedy
      integer x, y

      markedx = x
      markedy = y

      return
      end

      block data PixState
      common /PIXEL/ markedx, markedy
      data markedx/-1/, markedy/-1/
      end

      subroutine alimit(array,n,fmin,fmax)
      integer n, i
      real array(n), fmin, fmax

c     write *, "n is ",n,", fmin is ",fmin,", fmax is ",fmax
      do 10 i=1,n
	 array(i) = amax1(amin1(array(i),fmax),fmin)
c	 array(i) = array(i)*.5
   10 continue
      return
      end

