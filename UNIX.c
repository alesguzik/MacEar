/*
 * This is a Macintosh-to-UNIX interface module.  It does the following:
 * (1) runs a dialog to obtain the command-line arguments to the UNIX program
 * (2) invokes the UNIX program (must change "main" to "UNIX_main")
 * (3) displays a scrolling text window for terminal I/O and supports:
 *     printf
 *     getchar
 *     putchar
 *     exit
 *
 * Written by: Steven M. Rubin, Apple Computer, 1989.
 * Compiles in either LightspeedC or MPW
 *
 *	Warranty Information
 *	Even though Apple has reviewed this software, Apple makes no warranty
 *	or representation, either express or implied, with respect to this
 *	software, its quality, accuracy, merchantability, or fitness for a 
 *	particular purpose.  As a result, this software is provided "as is,"
 *	and you, its user, are assuming the entire risk as to its quality
 *	and accuracy.
 *
 *	Copyright (c) 1988-1991 by Apple Computer, Inc
 *		All Rights Reserved.
 *
 * $Header: UNIX.c,v 2.4 91/01/23 11:23:50 malcolm Exp $
 *
 * $Log:	UNIX.c,v $
 * Revision 2.4  91/01/23  11:23:50  malcolm
 * Changed initial parameter dialog so strings are shorter (and window isn't
 * as wide).  Also, changed the pixels per parameter line from 25 to 21.  Both
 * of these were done so that the dialog would fit on the screen on Macs with
 * small screens.  Also changed some indentations in the source and made a
 * few attempts to get the highlights to be drawn correctly when the program
 * starts up (Things like the highlighting of the run button and the boxes
 * around the radio boxes aren't drawn now until some other even causes 
 * ModalDialog to return and force an update.)
 * 
 * Revision 2.3  90/12/18  09:45:22  malcolm
 * Added support for Roy Patterson's Model.
 * 
 * Revision 2.1  90/11/06  20:41:50  malcolm
 * Miscellaneous Fixes
 * 
 * 
 * Revision 2.0.1.1  89/07/28  21:38:57  malcolm
 * Added taufactor parameter to the command line interface.  Also changed
 * dialog text boxes so that they would automatically check their check-box
 * if their parameters changed.
 * 
 * Revision 2.0  89/07/25  18:56:16  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.2  89/07/21  14:37:06  malcolm
 * Changed calls to printf to rubin_printf so that LightSpeed C won't have
 * to put up a console window.
 * 
 * Revision 1.1  89/07/19  12:31:54  malcolm
 * Initial revision
 * 
 *
 * The dialog which obtains the command-line arguments is specified by an array of
 * strings called "parselist", declared below.  The format is as follows:
 *   parselist  := PARAMETERS
 *   PARAMETERS := PARAMETER
 *   PARAMETERS := PARAMETER  PARAMETERS
 *   PARAMETER  := "[" CHOICES "]"
 *   PARAMETER  := "(" CHOICES ")"
 *   PARAMETER  := "INPUTFILE"  [ ":" MACNAME ]
 *   PARAMETER  := "OUTPUTFILE" [ ":" MACNAME ]
 *   CHOICES    := CHOICE
 *   CHOICES    := CHOICE "|" CHOICES
 *   CHOICE     := 'string'     [ ":" MACNAME ]  [ "ON" ]
 *   CHOICE     := 'string'     [ ":" MACNAME ]  [ [ "=" ]  "INPUTFILE" ]
 *   CHOICE     := 'string'     [ ":" MACNAME ]  [ [ "=" ]  "OUTPUTFILE" ]
 *   CHOICE     := 'string1'    [ ":" MACNAME ]  [ [ "=" ]  'string2' ]
 *
 * Less formally, the strings describe a set of parameters to the UNIX program.
 * The parameters may be constant strings, choices from a list, input or output
 * file names.  Optional parameters are surrounded by square brackets.  Here are
 * some examples:
 * (1) The first parameter to a UNIX program is typically the program name.  Here,
 *     a constant string is specified with a quoted parameter (always use single
 *     quotes) and is optionally followed by a colon and another quoted string that
 *     will be displayed in the dialog.  For example, the UNIX program "test"
 *     might begin with:
 *         'test' : 'My test program'
 * (2) A required input file is described with the keyword INPUTFILE, followed by
 *     the optional Macintosh prompt.  For example:
 *         INPUTFILE : 'Input to the program'
 * (3) An optional output file is described with the keyword OUTPUTFILE, and all
 *     surrounded in square brackets:
 *         [OUTPUTFILE : 'Output (default is standard output)']
 * (4) A list of possible keywords is separated with "|" bars, and enclosed in
 *     parenthesis or, if they are optional, enclosed in square brackets.  For
 *     example, if the test program can do one-column or two-column output with
 *     the "-o" and "-t" switches, you might include the following string:
 *         ( '-o' : 'One column output' | '-t' : 'Two column output' )
 * (5) An option may be parameterized if it appears in a required or in an optional
 *     list.  Simply add an additional keyword after the parameter declaration.
 *     For example, the UNIX option "-t FONT" would be described with:
 *         [ '-t' : 'Optional font' 'Font Name' ]
 *     and, if there is no space between the switch and its argument (i.e. if
 *     the UNIX option is "-tFONT", use an "=" before the parameter keyword:
 *         [ '-t' : 'Optional font' = 'Font Name' ]
 * (6) An option may also take an input or output file name.  The UNIX parameter
 *     "-oFILENAME" would be described with:
 *         [ '-o' : 'Output File' = OUTPUTFILE ]
 * (7) An option may be checked by default.  The UNIX parameter
 *     "-warn" will be generated UNLESS it is checked with:
 *         [ '-warn' : 'Print warnings' ON ]
 */


/******************* CHANGE THIS DECLARATION TO FIT YOUR PROGRAM *******************/
char *steves_parselist[] =
{
    "'ear'       : 'The Lyon Cochlear Model 2.2'",
    "INPUTFILE    : 'Filename to be compiled'",
    "('QUISC':'Compile' | 'QUAIL':'Simulate' | 'NETLISP':'Make netlist' |",
    " 'RSIM':'RSIM simulation' | 'NOTARGET':'Ignore')",
    "['/extern'   : 'Assume undefined entities are external' ON]",
    "['/nowarn'   : 'Supress warning messages']",
    "['/lib='     : 'File of QUAIL gates' = INPUTFILE]",
    "['/outfile=' : 'Output file'         = OUTPUTFILE]",
    0
};

char	*parselist[] =
{
	"'ear'     : 'Lyons Cochlear Model 2.2'",
	"['if='     : 'Input File' = INPUTFILE ]",
	"['of='     : 'Output File' = OUTPUTFILE ]",
	"['cf='     : 'Correlation Output File' = OUTPUTFILE ]",
	"('-i'     : 'Turn off Impulse Input' |",
	" '+i'     : 'Turn on Impulse Input' )",
	"('cor=lick'     : 'Licklider Correlogram' |",
	" 'cor=pat'     : 'Patterson Correlogram' )",
/*	"('+a'     : 'Turn on AGC' |",
	" '-a'     : 'Turn off AGC' )",
	"('+c'     : 'Turn on Cascade' |",
	" '-c'     : 'Turn off Cascade' )",
	"('+m'     : 'Turn on Subtraction between Channels' |",
	" '-m'     : 'Turn off Subtraction between Channels' )",		*/
	"['df='  : 'Filter Decimation Factor (df)' = '20']",
	"['taufactor='  : 'Decimation Low Pass Filter (taufactor)' = '3']",
	"['earq='  : 'Filter Width (earq)' = '8']",
	"['stepfactor='  : 'Channel Separation (stepfactor)' = '.25']",
/*	"['breakf='  : 'Bark Scale Break Frequency (breakf)' = '1000']",
	"['sharpness='  : 'Sharpness of zeros compared to poles (sharpness)' = '5']",
	"['offset='  : 'Zero Offset (offset)' = '1.5']",
	"['preemph='  : 'Preemphasis Corner Frequency (preemph)' = '300']",	*/
/*	"['tau1='  : 'AGC #1 Time Constant (tau1)' = '.640']",
	"['tau2='  : 'AGC #2 Time Constant (tau2)' = '.160']",
	"['tau3='  : 'AGC #3 Time Constant (tau3)' = '.080']",
	"['tau4='  : 'AGC #4 Time Constant (tau4)' = '.004']",
	"['target1='  : 'AGC #1 Target Value (target1)' = '.0032']",
	"['target2='  : 'AGC #2 Target Value (target2)' = '.0016']",
	"['target3='  : 'AGC #3 Target Value (target3)' = '.0008']",
	"['target4='  : 'AGC #4 Target Value (target4)' = '.0004']",		*/
	"['clag='  : 'Lags in Correlation (clag)' = '256']",
	"['cstep='  : 'Steps between Correlations (cstep)' = '128']",
/*	"['normalize='  : 'Correlation Normalization (normalize)' = '.75']",	*/
	0
};
/***********************************************************************************/


#ifdef THINK_C
#define	SCREENBITS screenBits
#define	THEPORT    thePort
#define	hfileInfo hFileInfo
#else
#include <Types.h>
#include <Quickdraw.h>
#include <ToolUtils.h>
#include <Fonts.h>
#include <Events.h>
#include <Windows.h>
#include <Dialogs.h>
#include <Menus.h>
#include <Desk.h>
#include <SegLoad.h>
#include <TextEdit.h>
#include <OSUtils.h>
#include <Packages.h>
#include <Memory.h>
#include <OSEvents.h>
#include <Files.h>
#include <Scrap.h>
#define	SCREENBITS qd.screenBits
#define	THEPORT    qd.thePort
#endif

/* definitions for the graphics */
#define	SFONT	   monaco
#define	MENUSIZE   19
#define	SBARWIDTH  15

/* menu and item definitions */
enum {aboutMeCommand = 1};
enum {quitCommand = 1};
enum {cutCommand = 1, copyCommand, pasteCommand};
MenuHandle gra_appleMenu, gra_fileMenu, gra_editMenu;
enum {appleMENU = 128, fileMENU, editMENU};

/* definitions for parsing command-line/dialog description */
#define	EOP       -1		/* end of parse */
#define	UNKNOWN    0		/* unrecognized */
#define	WORD       1		/* a keyword */
#define	SEP        2		/* space or tab */
#define	OPENPAR    3		/* ( */
#define	CLOSEPAR   4		/* ) */
#define	OPENBRAC   5		/* [ */
#define	CLOSEBRAC  6		/* ] */
#define	ORBAR      7		/* | */
#define	QUOTE      8		/* ' */
#define	COLON      9		/* : */
#define	EQUAL     10		/* = */

/* definitions for command-line dialog */
#define	CONCOMMENT    1		/* a "comment" for display only */
#define	CONRADIO      2		/* a radio button */
#define	CONCHECK      3		/* a check box */
#define	CONTYPE      07		/* all above types */
#define	CONGETPAR   010		/* set if it gets a parameter */
#define	CONGETPARI  020		/* set if it gets an input file parameter */
#define	CONGETPARO  040		/* set if it gets an output file parameter */
#define	CONGETNS   0100		/* set if no space before parameter */
#define	CONOPTION  0200		/* set if parameter is optional */

#define	ACTIONSIZE   21		/* height of each line in command-line dialog */
#define	ANSWERWIDTH 150		/* answer width for parameterized command-line entries */

typedef struct
{
    short         control;	/* the control type (see above list) */
    short         checkit;	/* nonzero to check this box by default */
    short         item;		/* actual item in dialog */
    short         teitem;	/* text edit item in dialog */
    char         *display;	/* string to display in dialog */
    char         *string;	/* string to send to UNIX program */
    char         *defaultinput;	/* default input for CONGETPAR */
    short         radiogroup;	/* for grouping radio buttons */
} PARSE;

WindowPtr     gra_textwindow;	/* the standard I/O text window */
DialogPtr     gra_controldialog;/* the controls dialog */
ControlHandle gra_vScroll;	/* vertical scroll control in status window */
TEHandle      gra_TEH;		/* text editing handle in status window */
Rect          gra_dialogrect;	/* rectangle of dialog box */
static short  gra_linesInFolder;/* lines in text folder */
static char **gra_parselist;	/* the list of commands being parsed */
static short  gra_parseline;	/* the line number being parsed */
static char  *gra_parsepos;	/* the position in the line being parsed */
static short  gra_actioncount;	/* number of actions */
static PARSE  gra_action[30];	/* dialogs for command-line */
static int    gra_argc;
static char  *gra_argv[30];

short bufsize = 0, buflen = 0;
Handle buffer;
#define	INCREMENT 256

/*
 * The real "main" program that initializes graphics, runs the command-line dialog,
 * and then calls "_main" as if it was in UNIX
 */
main()
{
    short i, chr, gra_nextevent();

    /* initialize the Macintosh */
#ifndef	THINK_C
    _DATAINIT();
#endif
    MaxApplZone();
    InitGraf(&THEPORT);
    InitFonts();
    FlushEvents(everyEvent, 0);
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(0L);
    InitCursor();

    /* do the dialog to get command-line arguments */
    if (gra_getarguments(&gra_argc, gra_argv, parselist) != 0) ExitToShell();

    /* make a text window for interaction */
    gra_initgraphics(gra_action[0].display, gra_action[0].string);

    /* run the main program */
    for(i=0; i<gra_argc; i++) rubin_printf("%s ", gra_argv[i]);
    rubin_printf("\n");
    _main(gra_argc, gra_argv);
    rubin_printf("\n************ PROGRAM RETURNED ************\n");

    /* loop until user exits */
    for(;;) (void)gra_nextevent(&chr);
}

/*
 * Routine to create the scrolling text window for interaction
 */
gra_initgraphics(header, about)
char *header, *about;
{
    Rect r;
    char head[200];

    /* initialize menus */
    gra_appleMenu = NewMenu(appleMENU, "\p\024");
    strcpy(head, " About ");
    strcat(head, about);
    strcat(head, "...");
    head[0] = strlen(&head[1]);
    AppendMenu(gra_appleMenu, head);
    AppendMenu(gra_appleMenu, "\p-");
    AddResMenu(gra_appleMenu, 'DRVR');
    gra_fileMenu = NewMenu(fileMENU, "\pFile");
    gra_editMenu = NewMenu(editMENU, "\pEdit");
    InsertMenu(gra_appleMenu, 0);
    InsertMenu(gra_fileMenu, 0);
    InsertMenu(gra_editMenu, 0);
    DrawMenuBar();
    AppendMenu(gra_fileMenu, "\pQuit/Q");
    AppendMenu(gra_editMenu, "\pCut/X;Copy/C;Paste/V");

    /* create the scrolling status window */
    r.top = SCREENBITS.bounds.top + MENUSIZE*2;
    r.bottom = SCREENBITS.bounds.bottom;
    r.left = SCREENBITS.bounds.left;
    r.right = SCREENBITS.bounds.right;
    head[0] = strlen(header);
    strcpy(&head[1], header);
    gra_textwindow = NewWindow(0L, &r, head, 1, documentProc,
	(WindowPtr)(-1L), 1, 0L);
    SetPort(gra_textwindow);
    EraseRect(&gra_textwindow->portRect);
    DrawGrowIcon(gra_textwindow);
    TextFont(SFONT);
    TextSize(9);
    r = gra_textwindow->portRect;
    r.left = r.right-SBARWIDTH;
    r.right += 1;
    r.bottom -= 14;
    r.top -= 1;
    gra_vScroll = NewControl(gra_textwindow, &r, "\p", 1, 0, 0, 0, scrollBarProc,
	0L);
    r = gra_textwindow->portRect;
    r.right -= SBARWIDTH;
    InsetRect(&r, 4, 4);
    gra_TEH = TENew(&r, &r);
    gra_setview(gra_textwindow);
    TEActivate(gra_TEH);
    ShowControl(gra_vScroll);
}

DoMenu(menu)
long menu;
{
    char name[256];
    GrafPtr savePort;

    switch (HiWord(menu))
    {
	case appleMENU:
	    GetPort(&savePort);
	    if (LoWord(menu) == aboutMeCommand)
	    {
		strcpy(name, gra_action[0].display);
		strcat(name, ".  Macintosh implementation with Quick-shell, by Steven M. Rubin");
		MyAlert(name);
	    } else
	    {
		GetItem(gra_appleMenu, LoWord(menu), name);
		(void)OpenDeskAcc(name);
	    }
	    SetPort(savePort);
	    break;
	case fileMENU:
	    switch (LoWord(menu))
	    {
		case quitCommand: ExitToShell();
	    }
	    break;
	case editMENU:
	    switch (LoWord(menu))
	    {
		case cutCommand:
		    TECut(gra_TEH);
		    ZeroScrap();
		    TEToScrap();
		    break;
		case copyCommand:
		    TECopy(gra_TEH);
		    ZeroScrap();
		    TEToScrap();
		    break;
		case pasteCommand:
		    TEFromScrap();
		    TEPaste(gra_TEH);
		    break;
	    }
	    break;
    }
    HiliteMenu(0);
}

/****************** CODE TO CONTROL THE TEXT WINDOW DURING EXECUTION ******************/

/*
 * Routine to handle events in the text window.  Returns nonzero if a character
 * was read (and placed in "chr").
 */
short gra_nextevent(chr)
short *chr;
{
    EventRecord theEvent;
    short oak, cntlCode;
    long key;
    WindowPtr theWindow, win;
    Rect dragRect;
    pascal void ScrollProc();
    ControlHandle theControl;
    static int firstupdate = 1;

    HiliteMenu(0);
    SystemTask();	/* Handle desk accessories */
    TEIdle(gra_TEH);
    oak = GetNextEvent(everyEvent, &theEvent);
    if (oak != 0) switch (theEvent.what)
    {
	case mouseDown:
	    switch (FindWindow(theEvent.where, &theWindow))
	    {
		case inMenuBar:
		    DoMenu(MenuSelect(theEvent.where));
		    break;
		case inSysWindow:
		    SystemClick(&theEvent, theWindow);
		    break;
		case inContent:
		    if (theWindow != FrontWindow())
		    {
			SelectWindow(theWindow);
			break;
		    }
		    SetPort(theWindow);
		    if (theWindow == gra_textwindow)
		    {
			GlobalToLocal(&theEvent.where);
			cntlCode = FindControl(theEvent.where, theWindow,
			    &theControl);
			if (cntlCode == inThumb)
			{
			    TrackControl(theControl, theEvent.where, 0L);
			    gra_adjusttext();
			    break;
			}
			if (cntlCode == inUpButton || cntlCode == inDownButton ||
			    cntlCode == inPageUp || cntlCode == inPageDown)
			{
			    TrackControl(theControl, theEvent.where, (ProcPtr)&ScrollProc);
			    break;
			}
			TEClick(theEvent.where,
			    (theEvent.modifiers & shiftKey) != 0, gra_TEH);
		    }
		    break;
		case inDrag:
		    if (theWindow == gra_textwindow)
		    {
			SetPort(theWindow);
			dragRect = SCREENBITS.bounds;
			dragRect.top += MENUSIZE;
			DragWindow(theWindow, theEvent.where, &dragRect);
			InsetRect(&dragRect, 4, 4);
		    }
		    break;
		case inGrow:
		    if (theWindow == gra_textwindow)
			gra_MyGrowWindow(theWindow, theEvent.where);
		    break;
		case inGoAway:
		    if (theWindow == gra_textwindow)
		    {
			SetPort(theWindow);
			if (TrackGoAway(theWindow, theEvent.where) != 0)
			    ExitToShell();
		    }
		    break;
	    }
	    break;
	case keyDown: 
	case autoKey:
	    if ((theEvent.modifiers & cmdKey) != 0)
	    {
		key = MenuKey((char)(theEvent.message & charCodeMask));
		DoMenu(key);
		break;
	    }
	    *chr = theEvent.message & charCodeMask;
	    return(1);    
	case activateEvt:
	    if ((WindowPtr)theEvent.message == gra_textwindow)
	    {
		if (theEvent.modifiers&activeFlag) TEFromScrap(); else
		{
		    ZeroScrap();
		    TEToScrap();
		}
	    }
	    break;
	case updateEvt:
	    win = (WindowPtr)theEvent.message;
	    SetPort(win);
	    BeginUpdate(win);
	    if (win == gra_textwindow)
	    {
		if (firstupdate == 0)
		{
		    EraseRect(&win->portRect);
		    DrawControls(win);
		    DrawGrowIcon(win);
		    TEUpdate(&win->portRect, gra_TEH);
		}
		firstupdate = 0;
	    }
	    EndUpdate(win);
	    break;
    }
    return(0);
}

gra_setview(w)
WindowPtr w;
{
    (*gra_TEH)->viewRect = w->portRect;
    (*gra_TEH)->viewRect.right -= SBARWIDTH;
    (*gra_TEH)->viewRect.bottom -= SBARWIDTH;
    InsetRect(&(*gra_TEH)->viewRect, 4, 4);
    gra_linesInFolder = ((*gra_TEH)->viewRect.bottom - (*gra_TEH)->viewRect.top) /
	(*gra_TEH)->lineHeight;
    (*gra_TEH)->viewRect.bottom = (*gra_TEH)->viewRect.top + (*gra_TEH)->lineHeight *
	gra_linesInFolder;
    (*gra_TEH)->destRect.right = (*gra_TEH)->viewRect.right;
    TECalText(gra_TEH);
}

pascal void ScrollProc(theControl, theCode)
ControlHandle theControl;
short theCode;
{
    int pageSize, scrollAmt;

    if (theCode == 0) return;
    pageSize = ((*gra_TEH)->viewRect.bottom-(*gra_TEH)->viewRect.top) / 
	(*gra_TEH)->lineHeight - 1;
    switch (theCode)
    {
	case inUpButton:   scrollAmt = -1;          break;
	case inDownButton: scrollAmt = 1;           break;
	case inPageUp:     scrollAmt = -pageSize;   break;
	case inPageDown:   scrollAmt = pageSize;    break;
    }
    SetCtlValue(theControl, GetCtlValue(theControl)+scrollAmt);
    gra_adjusttext();
}

gra_adjusttext()
{
    int	oldScroll, newScroll, delta;

    oldScroll = (*gra_TEH)->viewRect.top - (*gra_TEH)->destRect.top;
    newScroll = GetCtlValue(gra_vScroll) * (*gra_TEH)->lineHeight;
    delta = oldScroll - newScroll;
    if (delta != 0)
	TEScroll(0, delta, gra_TEH);
}

gra_SetVScroll()
{
    int n;

    n = (*gra_TEH)->nLines - gra_linesInFolder + 1;
    SetCtlMax(gra_vScroll, n > 0 ? n : 0);
}

gra_showselect()
{
    register int topLine, bottomLine, theLine;

    gra_SetVScroll();
    gra_adjusttext();

    topLine = GetCtlValue(gra_vScroll);
    bottomLine = topLine + gra_linesInFolder;

    if ((*gra_TEH)->selStart < (*gra_TEH)->lineStarts[topLine] ||
	    (*gra_TEH)->selStart >= (*gra_TEH)->lineStarts[bottomLine])
    {
	for (theLine = 0; ; theLine++)
	    if ((*gra_TEH)->selStart <= (*gra_TEH)->lineStarts[theLine]) break;
	SetCtlValue(gra_vScroll, theLine - gra_linesInFolder / 2);
	gra_adjusttext();
    }
}

gra_MyGrowWindow(w, p)
WindowPtr w;
Point p;
{
    long theResult;
    short oScroll;
    Rect r, oView;

    SetPort(w);
    SetRect(&r, 80, 80, SCREENBITS.bounds.right, SCREENBITS.bounds.bottom);
    theResult = GrowWindow(w, p, &r);
    if (theResult == 0) return;
    SizeWindow(w, LoWord(theResult), HiWord(theResult), 1);

    InvalRect(&w->portRect);
    oView = (*gra_TEH)->viewRect;
    oScroll = GetCtlValue(gra_vScroll);

    gra_setview(w);
    HidePen();
    MoveControl(gra_vScroll, w->portRect.right - SBARWIDTH, w->portRect.top-1);
    SizeControl(gra_vScroll, SBARWIDTH+1,
	w->portRect.bottom - w->portRect.top-(SBARWIDTH-2));
    ShowPen();

    gra_SetVScroll();
    gra_adjusttext();
}

/****************************** UNIX-LIKE I/O ******************************/

#ifndef	THINK_C

putchar(cmd)
short cmd;
{
    TEKey(cmd, gra_TEH);
    TESetSelect(32767, 32767, gra_TEH);
    gra_showselect();
}

#endif

rubin_printf(s, p1, p2, p3, p4, p5, p6, p7, p8, p9)
char *s, *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9;
{
    long len;
    short i;
    char tline[100];

    sprintf(tline, s, p1, p2, p3, p4, p5, p6, p7, p8, p9);
    len = strlen(tline);
#ifdef	THINK_C
    for(i=0; i<len; i++) if (tline[i] == '\n') tline[i] = '\r';
#else
    for(i=0; i<len; i++) if (tline[i] == '\r') tline[i] = '\n';
#endif
    TEInsert(tline, len, gra_TEH);
    len = 32767;
    TESetSelect(len, len, gra_TEH);
    gra_showselect();
}

#ifndef	THINK_C

getchar()
{
    short chr, gra_nextevent();

    for(;;) if (gra_nextevent(&chr) != 0) break;
    return(chr);
}

#endif	THINK_C

rubin_exit(val)
{
    short chr, gra_nextevent();

    printf("\n************ PROGRAM EXITED WITH CODE %d ************\n", val);
    for(;;) (void)gra_nextevent(&chr);
    exit();
}

/***************************** DIALOG FOR COMMAND-LINE *****************************/

/*
 * Routine to parse the structure in "parselist", run a dialog on it, and build a UNIX
 * command line in "argc" and "argv".  Returns nonzero to cancel the dialog
 */
gra_getarguments(argc, argv, parselist)
int *argc;
char *argv[], *parselist[];
{
    char temp[256];
    short i, j, k, len, w, bigw, descent, thistype, len1, itemHit, itemType;
    Rect itemRect;
    Handle itemHdl;
    static Point SFGwhere = {90, 82};
    static Point SFPwhere = {106, 104};
    SFReply reply;
    SFTypeList myTypes;
    char *gra_makefullname();
    FontInfo fontinfo;

    /* parse the argument list, quit if bad */
    gra_parsearguments(parselist);
    if (gra_actioncount < 0)
    {
		*argc = 0;
		return;
    }

    /* determine width of window */
/*  TextFont(courier);
    TextSize(10);		*/
    GetFontInfo(&fontinfo);
    descent = fontinfo.descent;
    bigw = 0;
    for(i=0; i<gra_actioncount; i++)
    {
		len = strlen(gra_action[i].display);
		w = TextWidth(gra_action[i].display, 0, len);
		if ((gra_action[i].control & CONGETPAR) != 0) w += ANSWERWIDTH;
		if ((gra_action[i].control & (CONGETPARI|CONGETPARO)) != 0)
		    w += StringWidth("\pSET") - 6;
		if (w > bigw) bigw = w;
    }
    gra_dialogrect.top = SCREENBITS.bounds.top + 3*MENUSIZE/2;
    gra_dialogrect.bottom = gra_dialogrect.top + (gra_actioncount+1) * ACTIONSIZE +
		ACTIONSIZE/2;
    gra_dialogrect.left = SCREENBITS.bounds.left + 30;
    gra_dialogrect.right = gra_dialogrect.left + 40 + bigw;

    TextFont(systemFont);
    TextSize(12);
    /* build an item list that describes the controls, then make the dialog */
    gra_makecontrols();
    gra_controldialog = NewDialog(0L, &gra_dialogrect, "\p", 1, dBoxProc,
		(WindowPtr)(-1L), 0, 0L, buffer);
    SetPort(gra_controldialog);
/*  TextFont(courier); */
    TextSize(10);	
    gra_setextrainfo();

    /* preset buttons and check boxes */
    for(i=0; i<gra_actioncount; i++)
    {
		thistype = gra_action[i].control & CONTYPE;
		if (thistype == CONCHECK && gra_action[i].checkit != 0)
		{
		    GetDItem(gra_controldialog, gra_action[i].item, &itemType, &itemHdl,
			&itemRect);
		    SetCtlValue((ControlHandle)itemHdl, 1);
		}
		if (thistype == CONRADIO)
		{
		    if (i != 0 && gra_action[i-1].radiogroup == gra_action[i].radiogroup)
			continue;
		    if ((gra_action[i].control&CONOPTION) != 0) continue;
		    GetDItem(gra_controldialog, gra_action[i].item, &itemType, &itemHdl,
			&itemRect);
		    SetCtlValue((ControlHandle)itemHdl, 1);
		}
    }
    DrawDialog(gra_controldialog);
    gra_setextrainfo();

    for(;;){
		ModalDialog(0L, &itemHit);
		if (itemHit == 2) break;
		if (itemHit == 1)
		{
		    /* ensure that all required parameters are given */
		    if (gra_gotitall() != 0) break;
		}
		for(i=0; i<gra_actioncount; i++)
		{
		    thistype = gra_action[i].control & CONTYPE;
		    if (thistype == CONCOMMENT) continue;
		    if (thistype == CONCHECK && itemHit == gra_action[i].teitem)
			  {
			    GetDItem(gra_controldialog, itemHit, &itemType, &itemHdl,
			      &itemRect);
			    GetIText(itemHdl, temp);
			    temp[temp[0]+1] = 0;
			    GetDItem(gra_controldialog, gra_action[i].item, &itemType,
			      &itemHdl, &itemRect);
			    if (strcmp(&temp[1], gra_action[i].defaultinput) != 0)
			      SetCtlValue((ControlHandle)itemHdl, 1);
			    continue;
			  }
			if (gra_action[i].item != itemHit) {
				gra_setextrainfo();				
				continue;
			}
		    if (thistype == CONRADIO)
		    {
				for(k=0; k<gra_actioncount; k++) 
				    if (gra_action[k].radiogroup == gra_action[i].radiogroup)
				{
				    GetDItem(gra_controldialog, gra_action[k].item, &itemType, &itemHdl,
					&itemRect);
				    SetCtlValue((ControlHandle)itemHdl, 0);
				}
				GetDItem(gra_controldialog, itemHit, &itemType, &itemHdl, &itemRect);
				SetCtlValue((ControlHandle)itemHdl, 1);
		    } else if (thistype == CONCHECK)
		    {
				GetDItem(gra_controldialog, itemHit, &itemType, &itemHdl, &itemRect);
				SetCtlValue((ControlHandle)itemHdl, 1-GetCtlValue((ControlHandle)itemHdl));
		    }
	
		    /* "SET" button hit, get file names */
		    if ((gra_action[i].control&CONGETPARI) != 0)
		    {
				SFGetFile(SFGwhere, "\p", 0L, -1, myTypes, 0L, &reply);
				if (reply.good)
				{
				    for(j=0; j<reply.fName[0]; j++) temp[j] = reply.fName[j+1];
				    temp[j] = 0;
				    GetDItem(gra_controldialog, gra_action[i].teitem, &itemType,
					&itemHdl, &itemRect);
				    SetIText(itemHdl, gra_makefullname(temp, reply.vRefNum));
				}
				gra_setextrainfo();
		    } else if ((gra_action[i].control&CONGETPARO) != 0)
		    {
				SFPutFile(SFPwhere, "\p", "\p", 0L, &reply);
				if (reply.good)
				{
				    for(j=0; j<reply.fName[0]; j++) temp[j] = reply.fName[j+1];
				    temp[j] = 0;
				    GetDItem(gra_controldialog, gra_action[i].teitem, &itemType,
					&itemHdl, &itemRect);
				    SetIText(itemHdl, gra_makefullname(temp, reply.vRefNum));
				}
				gra_setextrainfo();
		    }
		}
    }

    /* compose the UNIX command line */
    *argc = 0;
    for(i=0; i<gra_actioncount; i++)
    {
	thistype = gra_action[i].control & CONTYPE;
	if (thistype == CONRADIO || thistype == CONCHECK)
	{
	    GetDItem(gra_controldialog, gra_action[i].item, &itemType, &itemHdl,
		&itemRect);
	    if (GetCtlValue((ControlHandle)itemHdl) == 0) continue;
	}
	len1 = strlen(gra_action[i].string);
	if ((gra_action[i].control & (CONGETPAR|CONGETNS)) ==
	    (CONGETPAR|CONGETNS))
	{
	    GetDItem(gra_controldialog, gra_action[i].teitem, &itemType, &itemHdl,
		&itemRect);
	    GetIText(itemHdl, temp);
	    argv[*argc] = NewPtr(len1+temp[0]+1);
	    strcpy(argv[*argc], gra_action[i].string);
	    for(j=0; j<temp[0]; j++) argv[*argc][len1+j] = temp[j+1];
	    argv[*argc][len1+temp[0]] = 0;
	} else
	{
	    argv[*argc] = NewPtr(len1+1);
	    strcpy(argv[*argc], gra_action[i].string);
	}
	(*argc)++;
	if ((gra_action[i].control & (CONGETPAR|CONGETNS)) == CONGETPAR)
	{
	    GetDItem(gra_controldialog, gra_action[i].teitem, &itemType, &itemHdl,
		&itemRect);
	    GetIText(itemHdl, temp);
	    argv[*argc] = NewPtr(temp[0]+1);
	    for(j=0; j<temp[0]; j++) argv[*argc][j] = temp[j+1];
	    argv[*argc][temp[0]] = 0;
	    (*argc)++;
	}
    }

    /* terminate the window */
    DisposDialog(gra_controldialog);
    if (itemHit == 2) return(1);
    return(0);
}

/* Routine to fill in extra graphics in the dialog */
gra_setextrainfo()
{
    short itemType, i, k, bigw, w, thistype;
    Handle itemHdl;
    Rect itemRect;

    /* highlight the RUN button */
    SetPort(gra_controldialog);
    GetDItem(gra_controldialog, 1, &itemType, &itemHdl, &itemRect);
    PenSize(3, 3);
    InsetRect(&itemRect, -4, -4);
    FrameRoundRect(&itemRect, 16, 16);
    PenSize(1, 1);

    for(i=0; i<gra_actioncount; i++)
    {
	thistype = gra_action[i].control & CONTYPE;
	if (thistype == CONRADIO)
	{
	    if (i != 0 && gra_action[i-1].radiogroup == gra_action[i].radiogroup)
		continue;
	    bigw = 0;
	    for(k=i; k<gra_actioncount; k++)
	    {
		if (gra_action[k].radiogroup != gra_action[i].radiogroup) break;
		w = TextWidth(gra_action[k].display, 0, strlen(gra_action[k].display));
		if (w > bigw) bigw = w;
	    }
	    itemRect.left = gra_controldialog->portRect.left + 8;
	    itemRect.right = itemRect.left + 24 + bigw;
	    itemRect.top = (i+1)*ACTIONSIZE-18;
	    itemRect.bottom = k*ACTIONSIZE+2;
	    PenSize(2, 2);
	    FrameRect(&itemRect);
	    PenSize(1, 1);
	    continue;
	}

	/* draw an arrow if not optional */
	if ((gra_action[i].control&CONOPTION) == 0 && thistype != CONRADIO)
	{
	    MoveTo(gra_controldialog->portRect.left, (i+1)*ACTIONSIZE-9);
	    PenSize(2, 2);
	    Line(6, 0);   Line(-2, 2);   Move(0, -4);   Line(2, 2);
	    PenSize(1, 1);
	}
    }
}

needinbuf(amt)
short amt;
{
    Handle newbuf;
    short i;

    if (buflen + amt > bufsize)
    {
	newbuf = NewHandle(buflen+amt+INCREMENT);
	if (newbuf == 0L) ExitToShell();
	if (buflen != 0)
	{
	    for(i=0; i<buflen; i++) (*newbuf)[i] = (*buffer)[i];
	    DisposHandle(buffer);
	}
	bufsize = buflen + amt + INCREMENT;
	buffer = newbuf;
    }
}

addchtobuffer(ch)
char ch;
{
    needinbuf(1);
    (*buffer)[buflen++] = ch;
}

addwdtobuffer(w)
short w;
{
    needinbuf(2);
    (*buffer)[buflen++] = w >> 8;
    (*buffer)[buflen++] = w & 0377;
}

addstrtobuffer(str)
char *str;
{
    short len, i, olen;

    olen = len = strlen(str);
    if ((len&1) != 0) len++;
    needinbuf(len+1);
    (*buffer)[buflen++] = olen;
    for(i=0; i<len; i++) (*buffer)[buflen++] = str[i];
}

/*
 * Routine to make a handle to an item list that describes the controls in the dialog
 * window.
 */
gra_makecontrols()
{
    short i, k, len, w, thistype, thisitem, dwid, sw, nonoptions;

    /* compute number of items */
    k = gra_actioncount + 2;
    nonoptions = 0;
    for(i=0; i<gra_actioncount; i++)
    {
	if ((gra_action[i].control&CONGETPAR) != 0) k++;
	if ((gra_action[i].control&(CONGETPARI|CONGETPARO)) != 0) k++;
	if ((gra_action[i].control&CONOPTION) == 0 &&
	    (gra_action[i].control&CONTYPE) != CONRADIO) nonoptions++;
    }
    if (nonoptions != 0) k++;

    /* first word is number of items minus 1 */
    addwdtobuffer(k-1);

    /* add "RUN" button */
    addwdtobuffer(0);   addwdtobuffer(0);
    addwdtobuffer((gra_actioncount+1)*ACTIONSIZE+4 - 20);
    addwdtobuffer(20);
    addwdtobuffer((gra_actioncount+1)*ACTIONSIZE+4);
    addwdtobuffer(60);
    addchtobuffer(ctrlItem + btnCtrl);
    addstrtobuffer("RUN");

    /* add "cancel" button */
    dwid = gra_dialogrect.right - gra_dialogrect.left;
    addwdtobuffer(0);   addwdtobuffer(0);
    addwdtobuffer((gra_actioncount+1)*ACTIONSIZE+4 - 20);
    addwdtobuffer(dwid - 80);
    addwdtobuffer((gra_actioncount+1)*ACTIONSIZE+4);
    addwdtobuffer(dwid - 20);
    addchtobuffer(ctrlItem + btnCtrl);
    addstrtobuffer("CANCEL");

    thisitem = 3;
    for(i=0; i<gra_actioncount; i++)
    {
	thistype = gra_action[i].control & CONTYPE;
	len = strlen(gra_action[i].display);
	w = TextWidth(gra_action[i].display, 0, len);
	if (thistype == CONCOMMENT)
	{
	    /* placeholder for handle */
	    addwdtobuffer(0);   addwdtobuffer(0);
	    addwdtobuffer((i+1)*ACTIONSIZE - 16);
	    addwdtobuffer((dwid - w) / 2);
	    addwdtobuffer((i+1)*ACTIONSIZE);
	    addwdtobuffer(dwid);
	    addchtobuffer(statText);
	    addstrtobuffer(gra_action[i].display);
	    thisitem++;
	    continue;
	}

	/* setup for radio button or check box */
	if ((gra_action[i].control&(CONGETPARI|CONGETPARO)) != 0)
	{
	    sw = StringWidth("\pSET") + 8;
	    addwdtobuffer(0);   addwdtobuffer(0);
	    addwdtobuffer((i+1)*ACTIONSIZE - 16);
	    addwdtobuffer(14+sw);
	    addwdtobuffer((i+1)*ACTIONSIZE);
	    addwdtobuffer(15+sw+w);
	    addchtobuffer(statText);
	    addstrtobuffer(gra_action[i].display);
	    thisitem++;

	    addwdtobuffer(0);   addwdtobuffer(0);
	    addwdtobuffer((i+1)*ACTIONSIZE - 16);
	    addwdtobuffer(10);
	    addwdtobuffer((i+1)*ACTIONSIZE);
	    addwdtobuffer(10+sw);
	    addchtobuffer(ctrlItem + btnCtrl);
	    addstrtobuffer("SET");
	} else
	{
	    addwdtobuffer(0);   addwdtobuffer(0);
	    addwdtobuffer((i+1)*ACTIONSIZE - 16);
	    addwdtobuffer(10);
	    addwdtobuffer((i+1)*ACTIONSIZE);
	    addwdtobuffer(30+w);
	    if (thistype == CONCHECK) addchtobuffer(ctrlItem + chkCtrl); else
		if (thistype == CONRADIO) addchtobuffer(ctrlItem + radCtrl); else
		    addchtobuffer(statText);
	    addstrtobuffer(gra_action[i].display);
	}
	gra_action[i].item = thisitem++;

	/* add input box if there is any */
	if ((gra_action[i].control&CONGETPAR) != 0)
	{
	    /* placeholder for handle */
	    addwdtobuffer(0);   addwdtobuffer(0);
	    addwdtobuffer((i+1)*ACTIONSIZE - 16);	/* TOP */
	    addwdtobuffer(dwid - 6 - ANSWERWIDTH);	/* LEFT */
	    addwdtobuffer((i+1)*ACTIONSIZE);		/* BOTTOM */
	    addwdtobuffer(dwid - 10);			/* RIGHT */
	    addchtobuffer(editText);
	    addstrtobuffer(gra_action[i].defaultinput);
	    gra_action[i].teitem = thisitem++;
	}
    }

    /* explain the arrow */
    if (nonoptions != 0)
    {
		addwdtobuffer(0);   addwdtobuffer(0);
		addwdtobuffer((gra_actioncount+1)*ACTIONSIZE+4 - 20);
		w = StringWidth("Arrows show required items");
		i = (dwid-20 - w)/2 + 60;
		addwdtobuffer(60+i);
		addwdtobuffer((gra_actioncount+1)*ACTIONSIZE+4);
		addwdtobuffer(dwid - 80);
		addchtobuffer(statText);
		addstrtobuffer("Arrows show required items");
    }
}

/*
 * Routine to convert a partial name with reference number into a full path name
 */
char *gra_makefullname(thisname, refnum)
char *thisname;
short refnum;
{
    short err;
    CInfoPBRec cpb;
    char line[256];
    static char sofar[256];

    strcpy(&sofar[1], thisname);
    cpb.hfileInfo.ioVRefNum = refnum;
    cpb.hfileInfo.ioDirID = 0;
    cpb.hfileInfo.ioCompletion = 0L;
    cpb.hfileInfo.ioNamePtr = (StringPtr)line;
    cpb.hfileInfo.ioFDirIndex = -1;
    for(;;)
    {
	err = PBGetCatInfo(&cpb, 0);
	if (err != noErr) break;
	line[line[0]+1] = 0;
	strcat(line, ":");
	strcat(line, &sofar[1]);
	strcpy(&sofar[1], &line[1]);
	if (cpb.hfileInfo.ioFlParID == 0) break;
	cpb.hfileInfo.ioDirID = cpb.hfileInfo.ioFlParID;
    }
    sofar[0] = strlen(&sofar[1]);
    return(sofar);
}

/*
 * Routine to parse "parselist" into a series of dialog actions in "gra_action".
 * Returns -1 on error.
 */
gra_parsearguments(parselist)
char *parselist[];
{
    int i, type, radiopackage, optional, inlist;
    char *line, *tempstring, *macstring;

    gra_parseline = -1;
    gra_parselist = parselist;

    /* first parse the parameters */
    radiopackage = 1;
    gra_actioncount = 0;
    inlist = optional = 0;
    type = UNKNOWN;
    for(;;)
    {
	/* skip leading blanks */
	if (type == UNKNOWN)
	{
	    do { type = gra_gettoken(&line); } while (type == SEP);
	    if (type == EOP) break;
	}

	/* handle beginning of aggregations */
	if (type == OPENPAR)
	{
	    if (optional != 0)
	    {
		gra_error("Cannot have parenthetical list inside of square brackets");
		return(-1);
	    }
	    if (inlist != 0)
	    {
		gra_error("Cannot have nested parenthetical lists");
		return(-1);
	    }
	    inlist = 1;
	    type = UNKNOWN;
	    continue;
	}
	if (type == OPENBRAC)
	{
	    if (inlist != 0)
	    {
		gra_error("Cannot have square brackets inside of parenthetical list");
		return(-1);
	    }
	    if (optional != 0)
	    {
		gra_error("Cannot have nested square brackets");
		return(-1);
	    }
	    optional = CONOPTION;
	    type = UNKNOWN;
	    continue;
	}

	/* handle end of aggregation */
	if (type == CLOSEPAR)
	{
	    if (inlist == 0)
	    {
		gra_error("No open bracket to match the close bracket");
		return(-1);
	    }
	    inlist = 0;
	    radiopackage++;
	    type = UNKNOWN;
	    continue;
	}
	if (type == CLOSEBRAC)
	{
	    if (optional == 0)
	    {
		gra_error("No open parenthesis to match the close parenthesis");
		return(-1);
	    }
	    optional = 0;
	    radiopackage++;
	    type = UNKNOWN;
	    continue;
	}

	/* handle lists */
	if (type == ORBAR)
	{
	    radiopackage--;
	    type = UNKNOWN;
	    continue;
	}

	/* handle parameter */
	if (type == QUOTE)
	{
	    if (inlist == 0 && optional == 0)
		gra_action[gra_actioncount].control = CONCOMMENT | CONOPTION; else
		    gra_action[gra_actioncount].control = CONCHECK | optional;
	    gra_action[gra_actioncount].checkit = 0;
	    gra_action[gra_actioncount].string = NewPtr(strlen(line)+1);
	    strcpy(gra_action[gra_actioncount].string, line);
	    macstring = gra_action[gra_actioncount].string;
	} else if (type == WORD)
	{
	    gra_action[gra_actioncount].control = CONCHECK | optional;
	    gra_action[gra_actioncount].checkit = 0;
	    if (strcmp(line, "INPUTFILE") == 0)
	    {
		macstring = "Input file";
		gra_action[gra_actioncount].control |= CONGETPAR | CONGETPARI | CONGETNS;
	    } else if (strcmp(line, "OUTPUTFILE") == 0)
	    {
		macstring = "Output file";
		gra_action[gra_actioncount].control |= CONGETPAR | CONGETPARO | CONGETNS;
	    } else
	    {
		gra_error("Use single quotes around keywords");
		return(-1);
	    }
	    gra_action[gra_actioncount].string = NewPtr(1);
	    strcpy(gra_action[gra_actioncount].string, "");
	} else
	{
	    gra_error("Expected INPUTFILE, OUTPUTFILE, or quoted string");
	    return(-1);
	}

	/* get the true Macintosh string if specified */
	do { type = gra_gettoken(&line); } while (type == SEP);
	if (type == COLON)
	{
	    do { type = gra_gettoken(&line); } while (type == SEP);
	    if (type != QUOTE)
	    {
		gra_error("Must have quoted string after ':'");
		return(-1);
	    }
	    macstring = line;
	    type = UNKNOWN;
	}
	gra_action[gra_actioncount].display = NewPtr(strlen(macstring)+1);
	strcpy(gra_action[gra_actioncount].display, macstring);

	/* get optional parameters (if in list) */
	if (inlist != 0 || optional != 0)
	{
	    /* first see if the optional "=" is specified */
	    if (type == UNKNOWN)
		do { type = gra_gettoken(&line); } while (type == SEP);
	    if (type == EQUAL)
	    {
		do { type = gra_gettoken(&line); } while (type == SEP);
		gra_action[gra_actioncount].control |= CONGETNS;
	    }

	    /* now get the parameter */
	    if (type == QUOTE)
	    {
		gra_action[gra_actioncount].control |= CONGETPAR;
		tempstring = line;
		type = UNKNOWN;
	    } else if (type == WORD)
	    {
		if (strcmp(line, "INPUTFILE") == 0)
		{
		    gra_action[gra_actioncount].control |= CONGETPAR | CONGETPARI;
		    tempstring = "Input file";
		} else if (strcmp(line, "OUTPUTFILE") == 0)
		{
		    gra_action[gra_actioncount].control |= CONGETPAR | CONGETPARO;
		    tempstring = "Output file";
		} else if (strcmp(line, "ON") == 0)
		{
		    gra_action[gra_actioncount].checkit = 1;
		} else
		{
		    gra_error("Use single quotes around parameter keyword");
		    return(-1);
		}
		type = UNKNOWN;
	    }
	} else tempstring = "";
	gra_action[gra_actioncount].defaultinput = NewPtr(strlen(tempstring)+1);
	strcpy(gra_action[gra_actioncount].defaultinput, tempstring);

	/* end of keyword specification */
	gra_action[gra_actioncount].radiogroup = radiopackage++;
	gra_actioncount++;
    }

    /* postprocess items in the same radio group to be radio buttons */
    for(i=1; i<gra_actioncount; i++)
    {
	if (gra_action[i-1].radiogroup == gra_action[i].radiogroup)
	{
	    gra_action[i-1].control = (gra_action[i-1].control & ~CONTYPE) | CONRADIO;
	    gra_action[i].control = (gra_action[i].control & ~CONTYPE) | CONRADIO;
	}
    }
    return(0);
}

gra_gettoken(line)
char **line;
{
    static char single[2] = " ";
    short c, firstc, wordpos, i;
    static char *word, *newword;
    static short wordlen = 0;

    /* get the next character */
    c = gra_getnxtchr();
    if (c == 0) return(EOP);

    /* if alpha, gather word */
    if (c == '\'' || gra_isalnum(c))
    {
	firstc = c;
	wordpos = 0;
	for(;;)
	{
	    if (wordpos >= wordlen-1)
	    {
		newword = NewPtr(wordlen+50);
		if (newword == 0) break;
		for(i=0; i<wordpos; i++) newword[i] = word[i];
		if (wordlen != 0) DisposPtr(word);
		wordlen += 50;
		word = newword;
	    }
	    word[wordpos++] = c;
	    c = gra_getnxtchr();
	    if (c == 0) break;
	    if (firstc == '\'')
	    {
		if (c == '\'') break;
	    } else
	    {
		if (!gra_isalnum(c))
		{
		    gra_backupchr();
		    break;
		}
	    }
	}
	word[wordpos] = 0;
	if (firstc == '\'')
	{
	    *line = &word[1];
	    return(QUOTE);
	} else
	{
	    *line = word;
	    return(WORD);
	}
    }

    /* single important character */
    single[0] = c;
    *line = single;

    /* recognize special characters */
    if (c == ' ' || c == '\t') return(SEP);
    if (c == '(') return(OPENPAR);
    if (c == ')') return(CLOSEPAR);
    if (c == '[') return(OPENBRAC);
    if (c == ']') return(CLOSEBRAC);
    if (c == '|') return(ORBAR);
    if (c == '\'') return(QUOTE);
    if (c == ':') return(COLON);
    if (c == '=') return(EQUAL);
    return(UNKNOWN);
}

gra_isalnum(c)
{
    if (c >= 'a' && c <= 'z') return(1);
    if (c >= 'A' && c <= 'Z') return(1);
    if (c >= '0' && c <= '9') return(1);
    return(0);
}

gra_getnxtchr()
{
    if (gra_parseline < 0 || *gra_parsepos == 0)
    {
	if (gra_parselist[gra_parseline+1] == 0) return(0);
	gra_parseline++;
	gra_parsepos = gra_parselist[gra_parseline];
	return(' ');
    }
    return(*gra_parsepos++);
}

gra_backupchr()
{
    if (gra_parselist[gra_parseline] == 0 ||
	gra_parsepos == gra_parselist[gra_parseline])
    {
	if (gra_parseline == 0) return;
	gra_parseline--;
	gra_parsepos = gra_parselist[gra_parseline];
	while (*gra_parsepos != 0) gra_parsepos++;
	return;
    }
    gra_parsepos--;
}

gra_error(msg)
char *msg;
{
    char whichline[100];

    sprintf(whichline, "On line %d: %s", gra_parseline+1, msg);
    MyAlert(whichline);
}

/*
 * Routine to see if all required parameters have been supplied.  Returns zero
 * if so.
 */
gra_gotitall()
{
    short i, j, itemType;
    Rect itemRect;
    Handle itemHdl;
    char sofar[256];

    for(i=0; i<gra_actioncount; i++)
    {
	if ((gra_action[i].control&CONOPTION) != 0) continue;
	if ((gra_action[i].control&CONTYPE) == CONRADIO)
	{
	    for(j=0; j<gra_actioncount; j++)
		if (gra_action[j].radiogroup == gra_action[i].radiogroup)
	    {
		GetDItem(gra_controldialog, gra_action[j].item, &itemType,
		    &itemHdl, &itemRect);
		if (GetCtlValue((ControlHandle)itemHdl) != 0) break;
	    }
	    if (j < gra_actioncount) continue;
	} else if ((gra_action[i].control&(CONGETPARI|CONGETPARO)) != 0)
	{
	    GetDItem(gra_controldialog, gra_action[i].teitem, &itemType,
		&itemHdl, &itemRect);
	    GetIText(itemHdl, sofar);
	    if (sofar[0] != 0) continue;
	} else
	{
	    GetDItem(gra_controldialog, gra_action[i].item, &itemType,
		&itemHdl, &itemRect);
	    if (GetCtlValue((ControlHandle)itemHdl) != 0) continue;
	}
	MyAlert("Must set options with arrows before running");
	gra_setextrainfo();
	return(0);
    }
    return(1);
}

/*
 * A resource-free Alert routine that displays a message
 */
MyAlert(msg)
char *msg;
{
    Rect r;
    WindowPtr w, theWindow;
    ControlHandle c, theControl;
    short oak, cntlCode, chr, wid, len, winwid, line;
    EventRecord theEvent;

    r.left = 42;
    r.right = 330;
    r.top = 44;
    r.bottom = 176;
    w = NewWindow(0L, &r, "\p", 1, dBoxProc, (WindowPtr)(-1L), 1, 0L);
    SetPort(w);
    EraseRect(&w->portRect);
    TextFont(systemFont);
    TextSize(12);
    winwid = r.right - r.left - 6;

    r.left = 202;
    r.right = 262;
    r.top = 102;
    r.bottom = 122;
    c = NewControl(w, &r, "\pOK", 1, 0, 0, 1, pushButProc, 0L);
    PenSize(3, 3);
    InsetRect(&r, -4, -4);
    FrameRoundRect(&r, 16, 16);

    for(line = 20; ; line += 20)
    {
	/* find longest string */
	for(len = strlen(msg); len > 0; len--)
	{
	    if (msg[len] != 0 && msg[len] != ' ') continue;
	    wid = TextWidth(msg, 0, len);
	    if (wid <= winwid) break;
	}
	if (len <= 0) len = strlen(msg);
	MoveTo(3, line);
	DrawText(msg, 0, len);
	if (msg[len] == 0) break;
	msg += len + 1;
    }

    /* now get options */
    for(;;)
    {
	SystemTask();	/* Handle desk accessories */
	oak = GetNextEvent(everyEvent, &theEvent);
	if (oak == 0) continue;

	/* handle RETURN and ENTER keys */
	if (theEvent.what == keyDown)
	{
	    if ((theEvent.modifiers & cmdKey) == 0)
	    {
		chr = theEvent.message & charCodeMask;
		if (chr == '\r' || chr == 03) break;
	    }
	    continue;
	}

	/* handle mouse clicks */
	if (theEvent.what != mouseDown) continue;
	if (FindWindow(theEvent.where, &theWindow) != inContent) continue;
	if (theWindow != w) continue;
	GlobalToLocal(&theEvent.where);
	cntlCode = FindControl(theEvent.where, theWindow, &theControl);
	if (cntlCode == 0) continue;
	if (theControl == c) break;
    }

    /* terminate the window */
    DisposeWindow(w);
}
