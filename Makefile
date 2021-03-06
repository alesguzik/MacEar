# This makefile is for the Cray.  See the Makefile.unix file for other
# Unix machines.  Feel free to move Makefile.unix to Makefile if you don't
# have a Cray and are tired of typing make -f Makefile.unix.  Most of
# development is done on the Cray so I keep the names so it is most convenient
# for me.
#
# This makefile can either build the all C version (ear) or the C/Fortran
# version (fear).  The Fortran version has all the main loops coded in Fortran
# so they run as fast as possible.
#

SHELL = /bin/sh

CFILES = correlate.c \
	animate.c comm.c complex.c ear.c eardesign.c \
	earfilters.c fft.c file.c oneframe.c output.c picout.c script.c \
	timer.c utilities.c shamma.c patterson.c alloc.c \
	sosdesign.c sosfilters.c \
	hydesign.c hyfilters.c

FFILES = fcor.f fear.f fmag.f ftopix.f

FortranObjs = ftopix.o fcor.o fmag.o
GenObjs = $(FortranObjs) correlate.o shamma.o patterson.o \
	ear.o eardesign.o comm.o file.o complex.o fft.o \
	timer.o animate.o picout.o utilities.o output.o alloc.o  

FearObjs = $(GenObjs) sosdesign.o fear.o
SosObjs = $(GenObjs) sosdesign.o sosfilters.o earfilters.o
HydroObjs = $(GenObjs) hydesign.o hyfilters.o earfilters.o

CC=	scc
CFT=	cf77

#
#	CFLAGS - Here are some of the interesting options you might want to
#	use.
#		-DULTRA - Compile the ear model to include support for the 
#			Ultra Frame Buffer (at Apple).
#		-DPLOT3D - Compile the ear model to generate 3d plots using 
#			the Purdue Plot3d program.
#	Some machines also need some help with the floating point.  On Suns
#	you probably want to include -ffpa or -fweitek.  On Sequents you'll
#	either want -f1167 or -f387.
CFLAGS = -O -DULTRA $(COMMANDCCFLAGS)

#
#	Fortran Flags 
#	Use the following if you want debugging enabled on the Fortran stuff.
# FFLAGS= -g -a stack -c -m 2 -Zp 
#
FFLAGS= -a stack -c -m 2 -Zp  

LDFLAGS=

ULTRALIBS = ../ub/libub.a -lugraf 
CRAYLIBS = -lf -lsci -lu -lio -lnet 
LIBS=	$(ULTRALIBS) $(CRAYLIBS) -lm

#
#	The following is three different versions of the ear model.  
#	Fear is a C/Fortran implementation of the second order cascade model
#	Ear is a C implementation of the second order cascade model
#	Hear is a C implementation of the hydrodynamics model
#
fear:		$(FearObjs) Makefile 
		$(CC) $(LDFLAGS) $(CFLAGS) $(FearObjs) \
			$(LIBS) -o fear -lc

ear:		$(SosObjs) Makefile 
		$(CC) $(LDFLAGS) $(SosObjs) \
			$(LIBS) -o ear 

hear:		$(HydroObjs) Makefile 
		$(CC) $(LDFLAGS) $(HydroObjs) \
			$(LIBS) -o hear 

#
#	Most of the routines that do real work include stand alone test
#	code that can be used to verify the routines are working.
#	In all cases definining -DMAIN causes this test code to be included.
#
sosdesign:	sosdesign.c complex.o utilities.o comm.o eardesign.o
		$(CC) $(LDFLAGS) sosdesign.c -DMAIN complex.o \
			eardesign.o comm.o utilities.o \
			-lm -o sosdesign
		rm -f sosdesign.o

sosfilters:	sosfilters.c earfilters.o
		$(CC) $(LDFLAGS) sosfilters.c -DMAIN \
			earfilters.o -lm -o sosfilters
		rm -f sosfilters.o

hydesign:	hydesign.c complex.o utilities.o comm.o eardesign.o
		$(CC) $(LDFLAGS) hydesign.c -DMAIN complex.o \
			eardesign.o comm.o utilities.o \
			-lm -o hydesign
		rm -f hydesign.o

hyfilters:	hyfilters.c complex.o utilities.o comm.o eardesign.o \
			hydesign.o earfilters.o
		$(CC) hyfilters.c -DMAIN hydesign.o \
			complex.o eardesign.o comm.o utilities.o earfilters.o \
			-lm -o hyfilters
		rm -f hyfilters.o

complex:	complex.c 
		$(CC) $(LDFLAGS) -DMAIN complex.c -o complex
		rm -f complex.o

fft:		fft.c alloc.o timer.o
		$(CC) $(LDFLAGS) -DMAIN fft.c alloc.o timer.o $(LIBS) -o fft
		rm -f fft.o

correlate:	correlate.c fcor.o animate.o fft.o timer.o picout.o animate.o \
			ftopix.o alloc.o fmag.o
		$(CC) correlate.c -DMAIN animate.o fft.o timer.o fcor.o \
			picout.o animate.o ftopix.o alloc.o fmag.o \
			$(LIBS) -lm -o correlate
		rm -f correlate.o

oneframe:	oneframe.c
		$(CC) -DMAIN $(CFLAGS) oneframe.c -o oneframe -DDEBUG

animate:	animate.c ftopix.o timer.o 
		$(CC) -DMAIN $(CFLAGS) animate.c ftopix.o timer.o \
			-o animate -DDEBUG $(LIBS)
		-rm -f animate.o

ivq:		ivq.c file.o alloc.o
		$(CC) ivq.c file.o alloc.o -lm -o ivq

icos:		icos.c file.o alloc.o
		$(CC) icos.c file.o alloc.o -lm -o icos

#
#	Miscellaneous code that does useful work.
#
catsound:	catsound.o file.o
		$(CC) $(LDFLAGS) catsound.o file.o -o catsound $(LIBS)

all:		ear fear hear catsound

test:		sosdesign sosfilters hydesign hyfilters complex correlate \
		animate fft oneframe

clean:
		-rm *.o core ear fear hear sosdesign sosfilters hydesign \
			hyfilters complex correlate animate fft oneframe

SourceFiles = READ* *.[chf] Make* *.man ear.make .timers

shar:
		rm -f ear.shar
		shar ear.shar $(SourceFiles)

distribute:
		rcp $(SourceFiles) ceres:Speech/Ear
		rcp $(SourceFiles) medusa:Speech/Ear
		rcp $(SourceFiles) goofy:Speech/Ear
		rcp $(SourceFiles) jumbo:Speech/Ear
		rcp $(SourceFiles) blorb:Speech/Ear
		rcp $(SourceFiles) nucleus:Speech/Ear
		rcp $(SourceFiles) dataio:Speech/Ear

# depend:
#	Generates list of #include dependencies, also including nested
#	#includes.
#
# The for-loop generates a list of "file.o: header.h" dependencies, using
# cpp to expand nested includes.  The egrep picks out those lines that
# reference an include file.  The sed does: extract only the header-file,
# insert the object file name, changes redundant occurances of
# "../something/.." to ".." and "something/../somethingelse" to
# "somethingelse" up to two times.  This whole mess is sorted (uniq'd),
# and handed to awk to compress them on to 78-char lines.  The uniq is
# needed due to cpp output redundantly listing the same header file.
#
depend:
	echo '# DO NOT DELETE THIS LINE -- make depend uses it' > makedep
	-for file in ${CFILES} ; \
	do \
		obj=`basename $$file .c`.o; \
		${CC} ${COPTS} -E $$file | \
		egrep '^# line[ 	]+[0-9]+[ 	]+".*\.h"' | \
		sed -e 's/^.*"\(.*\)".*$$/\1/' \
		    -e "s/^/$$obj: /" \
		    -e 's;\.\./[^./]*/\.\.;..;' \
		    -e 's;[^./][^./]*/\.\./\([^./]*\);\1;' \
		    -e 's;[^./][^./]*/\.\./\([^./]*\);\1;' \
		    -e 's;\.\/\.\.;..;'; \
	done | sort -u | \
	awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' >> makedep
	echo '/^# DO NOT DELETE THIS LINE/,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >> eddep
	rm -f Makefile.bak
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep

# DO NOT DELETE THIS LINE -- make depend uses it

alloc.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/math.h
alloc.o: /usr/include/stdc/stdio.h /usr/include/stdc/stdlib.h
animate.o: ./timer.h ./ub.h /usr/include/stdc/math.h /usr/include/stdc/stdio.h
comm.o: /usr/include/stdc/stdio.h
complex.o: ./complex.h /usr/include/stdc/math.h
correlate.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/math.h
correlate.o: /usr/include/stdc/stdio.h
ear.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/errno.h
ear.o: /usr/include/stdc/math.h /usr/include/stdc/stdio.h
ear.o: /usr/include/sys/cpu.h
eardesign.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/math.h
eardesign.o: /usr/include/stdc/stdio.h
earfilters.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/math.h
earfilters.o: /usr/include/stdc/stdio.h
fft.o: ./timer.h /usr/include/stdc/math.h
file.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/math.h
file.o: /usr/include/stdc/stdio.h
hydesign.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/math.h
hydesign.o: /usr/include/stdc/stdio.h
hyfilters.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/math.h
hyfilters.o: /usr/include/stdc/stdio.h
oneframe.o: /usr/include/stdc/ctype.h /usr/include/stdc/locstruct.h
oneframe.o: /usr/include/stdc/stdio.h
output.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/math.h
output.o: /usr/include/stdc/stdio.h
patterson.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/math.h
patterson.o: /usr/include/stdc/stdio.h
picout.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/math.h
picout.o: /usr/include/stdc/stdio.h /usr/include/sys/fcntl.h
script.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/math.h
script.o: /usr/include/stdc/stdio.h
shamma.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/math.h
shamma.o: /usr/include/stdc/stdio.h
sosdesign.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/math.h
sosdesign.o: /usr/include/stdc/stdio.h
sosfilters.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/math.h
sosfilters.o: /usr/include/stdc/stdio.h
timer.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/ctype.h
timer.o: /usr/include/stdc/locstruct.h /usr/include/stdc/math.h
timer.o: /usr/include/stdc/stdio.h /usr/include/sys/types.h
utilities.o: ./complex.h ./ear.h ./filter.h ./timer.h /usr/include/stdc/math.h
utilities.o: /usr/include/stdc/stdio.h /usr/include/stdc/stdlib.h
