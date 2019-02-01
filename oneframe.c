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
 * $Header: oneframe.c,v 2.2 90/11/06 20:52:51 malcolm Exp $
 *
 * $Log:	oneframe.c,v $
 * Revision 2.2  90/11/06  20:52:51  malcolm
 * Changed copyright message.
 * 
 * Revision 2.1  89/11/09  23:09:51  malcolm
 * Removed an unused Buffer.
 * 
 * Revision 2.0  89/07/25  18:58:48  malcolm
 * Completely debugged and tested version on the following machines (roughly
 * in order of performance):
 * Cray, Stellar, SGI, Sun-4, Sequent Balance, Sun-3, VAX, Macintosh under
 * both MPW and LightSpeed C.
 * 
 * Revision 1.1  89/07/19  12:48:39  malcolm
 * Initial revision
 * 
 */

#include	<stdio.h>
#include	<ctype.h>

FILE	*ToRemote, *FromRemote;
int	RemotePID;

OpenConnection(machine)
char	*machine;
{
	int	i;
	int	topipe[2], frompipe[2];

	i = pipe(topipe);
	if (i < 0){
		perror("OpenConnection: To Pipe failed");
		exit(1);
	}
	pipe(frompipe);
	if (i < 0){
		perror("OpenConnection: From Pipe failed");
		exit(1);
	}

	RemotePID = fork();
	if (RemotePID < 0){
		fprintf(stderr, "Couldn't fork to open connection to %s.\n",
			machine);
		fprintf(stderr, 
		    "Try setting NCPUS to one to disable multiprocessing.\n");
		exit(1);
	}

	if (RemotePID > 0){			/* Child */
		i = dup2(topipe[0],0);		/* Standard Input */
		if (i < 0){
			perror("OpenConnection: Dup2 ToPipe Failed");
			exit(1);
		}
		i = dup2(frompipe[1],1);	/* Standard Output */
		if (i < 0){
			perror("OpenConnection: Dup2 FromPipe Failed");
			exit(1);
		}
		i = dup2(frompipe[1],2);	/* Standard Error */
		if (i < 0){
			perror("OpenConnection: Dup2 FromPipe Failed");
			exit(1);
		}
		i = close(topipe[0]);
		if (i < 0){
			perror("OpenConnection: Close ToPipe[0] Failed");
			exit(1);
		}
		i = close(topipe[1]);
		if (i < 0){
			perror("OpenConnection: Close ToPipe[1] Failed");
			exit(1);
		}
		i = close(frompipe[0]);
		if (i < 0){
			perror("OpenConnection: Close FromPipe[0] Failed");
			exit(1);
		}
		i = close(frompipe[1]);
		if (i < 0){
			perror("OpenConnection: Close FromPipe[1] Failed");
			exit(1);
		}

		execl("/usr/ucb/remsh", "remsh", machine, "exec", 
						"/bin/csh", "-i", 0);
		fprintf(stderr, "Couldn't exec remsh from child.\n");
		exit(1);
	}

						/* Parent */
	ToRemote = fdopen(topipe[1],"a");
	if (ToRemote == 0){
		fprintf(stderr, "OpenConnection: FDOpen topipe failed.\n");
		exit(1);
	}
	FromRemote = fdopen(frompipe[0], "r");
	if (FromRemote == 0){
		fprintf(stderr, "OpenConnection: FDOpen frompipe failed.\n");
		exit(1);
	}
	i = close(topipe[0]);
	if (i < 0){
		perror("OpenConnection: Close ToPipe[0] Failed");
		exit(1);
	}
	close(frompipe[1]);
	if (i < 0){
		perror("OpenConnection: Close FromPipe[1] Failed");
		exit(1);
	}

	while((i = getc(FromRemote)) != '%')
		putchar(i);
	getc(FromRemote);			/* Get the trailing space */

	SendCommandToRemote("set prompt = \"ok\\\n\"\n");
}

SendCommandToRemote(command)
char	*command;
{
	int	i;
	char	Buffer[BUFSIZ];

	i = strchr(command, '\n');
	if (i == 0){
		command[strlen(command)] = '\n';
		command[strlen(command)+1] = 0;
	}
	i = fputs(command,ToRemote);
	fflush(ToRemote);
	do {
		fgets(Buffer, BUFSIZ, FromRemote);
#ifdef	DEBUG
		printf(" Got back %s from remote side.\n", Buffer);
#endif	DEBUG
	} while (!feof(FromRemote) && strcmp(Buffer,"ok\n") != 0);
	if (feof(FromRemote)){
		printf("Got an end of file from the remote command.\n");
		exit(1);
	}
}

CloseConnection(){
	if (ToRemote)
		fclose(ToRemote);
	if (FromRemote)
		fclose(FromRemote);
}

#ifdef	MAIN

main(){
	int	i;

	OpenConnection("dumbo");

	for (i=0;i<10;i++){
		SendCommandToRemote("date");
	}

	CloseConnection();
}

#endif	MAIN
