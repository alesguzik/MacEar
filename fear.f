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
C  $Header: fear.f,v 2.2 90/12/17 18:01:20 malcolm Exp $
C 
C  $Log:	fear.f,v $
c Revision 2.2  90/12/17  18:01:20  malcolm
c Named the block data initialization so it wouldn't conflict with other
c block datas.
c 
c Revision 2.1  90/01/28  15:26:45  malcolm
c Moved the initialization of the "inited" flag into a Block Data
c subprogram so the Fortran is now legal.  Also increased the size
c of all the parameter arrays so that lower Q's and Stepfactor's can
c be used.
c 
c Revision 2.0  89/07/25  18:58:33  malcolm
c Completely debugged and tested version on the following machines (roughly
c in order of performance):
c Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
c both MPW and LightSpeed C.
c 
c Revision 1.6  89/04/09  17:00:51  malcolm
c Added function to return the normally hidden AGC state.  Also added
c support for decimation factor of 0 indicating no decimation and no
c filtering.
c 
c Revision 1.5  89/02/26  14:56:21  malcolm
c Fixed bugs at beginning and end of ear model.  Also made all ear 
c parameters saved variables.  Also added test if ear length is longer than
c the compiled constant.
c 
c Revision 1.4  88/12/06  21:13:22  malcolm
c Moved the vmov routine to the fcor.f file.
c 
c Revision 1.3  88/11/03  15:46:05  malcolm
c Speed hacks.  Moved all of the parameters to earstep into a common block.
c Moved the AGC parameter initialization code into the initialization 
c section of this routine.  Coalesced some loops to get the main loop down
c to 48us per iteration.
c 
c Revision 1.2  88/11/02  11:18:32  malcolm
c Added comments.  Note this version still has a constant size for the
c number of channels.  This is for efficiency but will fail if the 
c parameters of the ear model change.
c Also, added declarations for Sos3State and Sos4State.
c 
c Revision 1.1  88/10/23  22:40:48  malcolm
c Initial revision
c 
C 
C	This is a fortran version of the inner loop of the ear model.
C	This routine takes a single input value and produces an output
C	vector of length n.  
C	
C	The following flags are defined:
C	UseCascade - When non zero take the output of each stage and
C		use it as input to the next stage (i.e. make a cascade).
C		Otherwise each filter stage is independent and the same 
C		input value is applied to each stage in parallel.
C	UseAgc - When nonzero the normal AGC is used to adjust the gain
C		of each stage independently.  When zero there is no AGC
C		in the model.
C	ComputeFiltered - When non zero output the filtered version of the 
C		sound.  This is useful when trying to resynthesize the 
C		original sound based on what the output of the cochlea is.  
C		This flag is not available in the Fortran version of the ear 
C		model.
C	UseDifference - The response of this cochlea model is can be
C		sharpened by differentiating the output with respect to
C		cochlear position.  This is implemented by subtracting
C		adjacent channels when this flag is non zero.
C	DecimationFactor - This variable indicates the factor by which the
C		sample rate of the output of the cochlea model will be less
C		the sample rate of the incoming audio.  The EarStep routine 
C		uses this information to decide whether the output should be 
C		low pass filtered by a pair of first order filters.
C	a0, a1, a2, b1, b2 - These are arrays of parameter values for the
C		cascade of second order stages.  Each array contains a single
C		value of the appropriate parameter for each section of the 
C		cascade.  The use of each variable is shown in the figure
C		describing the implementation of a second order section in the
C		Mathematica notebook.
C	AgcEpsilon1-AgcEpsilon4 - Represent the decay times for each of the 
C		four AGC stages.
C	AgcStage1Target-AgcStage4Target - Are the target values for each of 
C		the four AGC stages.
C	DecimationEpsilon - This is the first order filter parameter used
C		to low pass filter the input before decimation.  Note this
C		routine doesn't actually perform the decimation.  This routine
C		low pass filters its output (if DecimationFactor is greater
C		than 1) and other code must select (sample) the output at the
C		appropriate times.
C 
      subroutine earstep(input, output)
C     parameter (n=88)
      parameter (n=120)
C	
C	The value of MaxN must be the same in this routine and again in
C	ear.h.  This sets up the definition of a common block that is shared
C	by C and Fortran.  If this number is wrong then data will be stuffed
C	into the wrong location.
C
      parameter (MaxN=180)
      integer UseCascade, UseAgc, ComputeFiltered, UseDifference
      integer DecimationFactor
      real input, output(MaxN)
      real a0(MaxN), a1(MaxN), a2(MaxN), b1(MaxN), b2(MaxN)
      real AgcEpsilon1, AgcEpsilon2, AgcEpsilon3, AgcEpsilon4
      real AgcStage1Target, AgcStage2Target
      real AgcStage3Target, AgcStage4Target
      real DecimationEpsilon

C
C	This common block must not change unless the corresponding
C	structure in utilities.c also changes.  We use this common
C	block to pass the global variables between C and Fortran
C	without doing it in a big function call or passing them with
C	every filter step.
C
      common /CBLOCK/ in, UseAgc, UseDifference, UseCascade, 
     1  ComputeFiltered, DecimationFactor, DecimationEpsilon,
     2  AgcEpsilon1, AgcEpsilon2, AgcEpsilon3, AgcEpsilon4,
     3  AgcStage1Target, AgcStage2Target, 
     4	AgcStage3Target, AgcStage4Target,
     5  a0, a1, a2, b1, b2


      integer i, inited
      real Sos1State(MaxN), Sos2State(MaxN)
      real Sos3State(MaxN), Sos4State(MaxN)
      real Agc1State(MaxN+2), Agc2State(MaxN+2), Agc3State(MaxN+2)
      real Agc4State(MaxN+2)
      real InputState(MaxN), DecimateState1(MaxN), DecimateState2(MaxN)
      real AgcTemp(n+2), oldinput(MaxN), temp(n+2)
      real OneMinusEpsOverThree1, EpsOverTarget1
      real OneMinusEpsOverThree2, EpsOverTarget2
      real OneMinusEpsOverThree3, EpsOverTarget3
      real OneMinusEpsOverThree4, EpsOverTarget4

      common /FSTATE/ inited, Sos1State, Sos2State, Agc1State, 
     1	Agc2State, Agc3State, Agc4State, InputState, DecimateState1, 
     2	DecimateState2, oldinput, OneMinusEpsOverThree1, EpsOverTarget1,
     3	OneMinusEpsOverThree2, EpsOverTarget2, OneMinusEpsOverThree3, 
     4	EpsOverTarget3, OneMinusEpsOverThree4, EpsOverTarget4

C
C	Vector Fortran Initializations.
C
      if (inited .eq. 0) then
	  Sos1State = 0.0
	  Sos2State = 0.0
	  Sos3State = 0.0
	  Sos4State = 0.0
	  Agc1State = 0.0
	  Agc2State = 0.0
	  Agc3State = 0.0
	  Agc4State = 0.0
	  DecimateState1 = 0.0
	  DecimateState2 = 0.0
	  InputState = 0.0
	  oldinput = 0.0
	  call INITCOM(in)

          if (in .gt. n) then
	    print *, 
     1       "Fortran routine can only compute ear model of length ", n
	    print *, "Need length ",in," to retain full accuracy."
	    endif

          OneMinusEpsOverThree1 = (1.0 - AgcEpsilon1)/3.0
          EpsOverTarget1 = AgcEpsilon1/AgcStage1Target
          OneMinusEpsOverThree2 = (1.0 - AgcEpsilon2)/3.0
          EpsOverTarget2 = AgcEpsilon2/AgcStage2Target
          OneMinusEpsOverThree3 = (1.0 - AgcEpsilon3)/3.0
          EpsOverTarget3 = AgcEpsilon3/AgcStage3Target
          OneMinusEpsOverThree4 = (1.0 - AgcEpsilon4)/3.0
          EpsOverTarget4 = AgcEpsilon4/AgcStage4Target

C         print *, in
C         print *, UseCascade, UseAgc, ComputeFiltered, UseDifference
C     	  print *, DecimationFactor
C     	  print *, input, output(1)
C     	  print *, "filters(1)", a0(1), a1(1), a2(1), b1(1), b2(1)
C     	  print *, "filters(2)", a0(2), a1(2), a2(2), b1(2), b2(2)
C     	  print *, AgcEpsilon1, AgcEpsilon2, AgcEpsilon3, AgcEpsilon4
C     	  print *, AgcStage1Target, AgcStage2Target
C     	  print *, AgcStage3Target, AgcStage4Target
C     	  print *, DecimationEpsilon
      endif
      inited = inited+1

      if (UseCascade .ne. 0) then
          do 10 i=n,2,-1
   10         InputState(i) = InputState(i-1)
          InputState(1) = input
      else
          do 20 i=1,n
   20         InputState(i) = input
      endif

      do 30 i=1,n
          t = InputState(i)
          InputState(i) = a0(i)*t                       + Sos1State(i)
          Sos1State(i) =  a1(i)*t - b1(i)*InputState(i) + Sos2State(i)
          Sos2State(i) =  a2(i)*t - b2(i)*InputState(i)
   30 continue

      if (UseAgc .ne. 0) then
          do 40 i=1,n
               output(i) = max(0.0,InputState(i))
   40     continue

C 	print *,inited,": output of sos ", output(42)

C	Note each AGC stage consists of three steps.  First we multiply
C	(attenuate) the signal by one minus the current state.  Then we
C	compute a new state by applying a first order filter to the weighted
C	average of the nearby states, then we limit the state to 1.  Note that
C	the actual limit step is done in a seperate loop (in the next stage)
C	because we don't want to overwrite the previous values until we have
C	done all of its neighbors.
C
C		AGC Stage 1
C
          Agc1State(1) = Agc1State(2)
          Agc1State(n+2) = Agc1State(n+1)
          do 150 i=1,n
              output(i) = (1.0 - Agc1State(i+1)) * output(i)
              temp(i+1) = output(i) * EpsOverTarget1 + 
     1                OneMinusEpsOverThree1 * 
     2                (Agc1State(i)+Agc1State(i+1)+Agc1State(i+2))
  150     continue

C 	print *,"output of first agc ", output(42)
C 	print *,"state of first agc ", temp(43)
C
C		AGC Stage 2
C
          Agc2State(1) = Agc2State(2)
          Agc2State(n+2) = Agc2State(n+1)
          do 250 i=1,n
              Agc1State(i+1) = min(1.0,temp(i+1))
              output(i) = (1.0 - Agc2State(i+1)) * output(i)
              temp(i+1) = output(i) * EpsOverTarget2 + 
     1                OneMinusEpsOverThree2 * 
     2                (Agc2State(i)+Agc2State(i+1)+Agc2State(i+2))
  250     continue

C 	print *,"second target2 is ",EpsOverTarget2
C 	print *,"second oneminuseps/3 is ", OneMinusEpsOverThree2
C 	print *,"output of second agc ", output(42)
C 	print *,"state of second agc ", temp(43)

C
C		AGC Stage 3
C
          Agc3State(1) = Agc3State(2)
          Agc3State(n+2) = Agc3State(n+1)
          do 350 i=1,n
              Agc2State(i+1) = min(1.0,temp(i+1))
              output(i) = (1.0 - Agc3State(i+1)) * output(i)
              temp(i+1) = output(i) * EpsOverTarget3 + 
     1                OneMinusEpsOverThree3 * 
     2                (Agc3State(i)+Agc3State(i+1)+Agc3State(i+2))
  350     continue

C 	print *,"output of third agc ", output(42)
C 	print *,"state of third agc ", temp(43)

C
C		AGC Stage 4
C
          Agc4State(1) = Agc4State(2)
          Agc4State(n+2) = Agc4State(n+1)
          do 450 i=1,n
              Agc3State(i+1) = min(1.0,temp(i+1))
              output(i) = (1.0 - Agc4State(i+1)) * output(i)
              temp(i+1) = output(i) * EpsOverTarget4 + 
     1                OneMinusEpsOverThree4 * 
     2                (Agc4State(i)+Agc4State(i+1)+Agc4State(i+2))
  450     continue

C 	print *,"output of fourth agc ", output(42)
C 	print *,"state of fourth agc ", temp(43)

          do 460 i=2,n+1
             Agc4State(i) = min(1.0,temp(i))
  460     continue
      else
          do 70 i=1,n
   70        output(i) = InputState(i)
      endif

      if (UseDifference .ne. 0) then
          do 80 i=2,n
             temp(i) = max(0.0,output(i-1) - output(i))
   80     continue
	  do 85 i=2,n
C	     oldinput(i) = output(i)
	     output(i) = temp(i)
   85     continue
          output(1) = output(2)
      endif

C
C	Low pass filter output before decimation.  This is implemented as two
C	first order filters.
C
      if (DecimationFactor .gt. 0) then
          OneMinusGain = 1.0 - DecimationEpsilon
          do 100 i=1,n+2
              output(i) = DecimationEpsilon*output(i) + 
     1                    OneMinusGain*DecimateState1(i)
              DecimateState1(i) = output(i)
              output(i) = DecimationEpsilon*output(i) + 
     1                    OneMinusGain*DecimateState2(i)
              DecimateState2(i) = output(i)
  100     continue
      endif

      return
      end

      function getagc(agc,channel)
      integer agc, channel

      parameter (MaxN=128)
      integer inited
      real Sos1State(MaxN), Sos2State(MaxN)
      real Sos3State(MaxN), Sos4State(MaxN)
      real Agc1State(MaxN+2), Agc2State(MaxN+2), Agc3State(MaxN+2)
      real Agc4State(MaxN+2)
      real InputState(MaxN), DecimateState1(MaxN), DecimateState2(MaxN)
      real oldinput(MaxN)
      real OneMinusEpsOverThree1, EpsOverTarget1
      real OneMinusEpsOverThree2, EpsOverTarget2
      real OneMinusEpsOverThree3, EpsOverTarget3
      real OneMinusEpsOverThree4, EpsOverTarget4

      common /FSTATE/ inited, Sos1State, Sos2State, Agc1State, 
     1	Agc2State, Agc3State, Agc4State, InputState, DecimateState1, 
     2	DecimateState2, oldinput, OneMinusEpsOverThree1, EpsOverTarget1,
     3	OneMinusEpsOverThree2, EpsOverTarget2, OneMinusEpsOverThree3, 
     4	EpsOverTarget3, OneMinusEpsOverThree4, EpsOverTarget4

      if (agc .eq. 1) then
	  getagc = Agc1State(channel+2)
      else if (agc .eq. 2) then
	  getagc = Agc2State(channel+2)
      else if (agc .eq. 3) then
	  getagc = Agc3State(channel+2)
      else if (agc .eq. 4) then
	  getagc = Agc4State(channel+2)
      else
	  getagc = 0
      endif
      return
      end

      block data EarState
      common /FSTATE/ inited, Sos1State, Sos2State, Agc1State, 
     1	Agc2State, Agc3State, Agc4State, InputState, DecimateState1, 
     2	DecimateState2, oldinput, OneMinusEpsOverThree1, EpsOverTarget1,
     3	OneMinusEpsOverThree2, EpsOverTarget2, OneMinusEpsOverThree3, 
     4	EpsOverTarget3, OneMinusEpsOverThree4, EpsOverTarget4

      data inited/0/
      end
