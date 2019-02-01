#
#	MPW Ear Application make file.
#
#	Note this makefile creates a full fledged Macintosh Application for a Mac-II.
#	This will application will not work without a 68881 (even if it did it would
#	be too slow to make sense.)
#
#							Malcolm Slaney
#							Apple Speech and Hearing
#							malcolm@apple.com
#
#   File:       ear.make
#   Target:     ear
#   Sources:    animate.c
#               comm.c
#               complex.c
#		correlate.c
#               ear.c
#               eardesign.c
#               fft.c
#               earfilters.c
#               file.c
#               output.c
#               picout.c
#               timer.c
#               UNIX.c
#               utilities.c
#   Created:    Sun, Jul 16, 1989 11:42:54 AM

CFLAGS = -mc68881 -mc68020 -d BUILDAPP -m

animate.c.o Ä ear.make animate.c
	 C {CFLAGS} animate.c -s animate
comm.c.o Ä ear.make comm.c
	 C {CFLAGS} comm.c -s comm 
complex.c.o Ä ear.make complex.c
	 C {CFLAGS} complex.c -s complex 
correlate.c.o Ä ear.make correlate.c
	 C {CFLAGS} correlate.c -s correlate
ear.c.o Ä ear.make ear.c
	 C {CFLAGS} ear.c -s ear
eardesign.c.o Ä ear.make eardesign.c
	 C {CFLAGS} eardesign.c -s eardesign
fft.c.o Ä ear.make fft.c
	 C {CFLAGS} fft.c -s fft
earfilters.c.o Ä ear.make earfilters.c
	 C {CFLAGS} earfilters.c -s earfilters
file.c.o Ä ear.make file.c
	 C {CFLAGS} file.c -s file
output.c.o Ä ear.make output.c
	 C {CFLAGS} output.c -s output
picout.c.o Ä ear.make picout.c
	 C {CFLAGS} picout.c -s picout
timer.c.o Ä ear.make timer.c
	 C {CFLAGS} timer.c -s timer
UNIX.c.o Ä ear.make UNIX.c
	 C {CFLAGS} UNIX.c -s UNIX
utilities.c.o Ä ear.make utilities.c
	 C {CFLAGS} utilities.c -s utilities

SOURCES = animate.c comm.c complex.c ear.c eardesign.c fft.c earfilters.c file.c output.c picout.c timer.c UNIX.c utilities.c
OBJECTS = ¶
		animate.c.o ¶
		comm.c.o ¶
		complex.c.o ¶
		correlate.c.o ¶
		ear.c.o ¶
		eardesign.c.o ¶
		fft.c.o ¶
		earfilters.c.o ¶
		file.c.o ¶
		output.c.o ¶
		picout.c.o ¶
		timer.c.o ¶
		UNIX.c.o ¶
		utilities.c.o

ear ÄÄ ear.make {OBJECTS}
	duplicate -y MacEar.¹.rsrc ear
	Link -w -t APPL -c Mear ¶
		{OBJECTS} ¶
		"{CLibraries}"CLib881.o ¶
		"{CLibraries}"CRuntime.o ¶
		"{Libraries}"Interface.o ¶
		"{CLibraries}"StdCLib.o ¶
		"{CLibraries}"CSANELib881.o ¶
		"{CLibraries}"Math881.o ¶
		"{CLibraries}"CInterface.o ¶
		-o ear
