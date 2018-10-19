/** 
	\file       visualizer.c
    \author     Francesco Cariaggi
    \matricola  503146
    
  Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  originale dell' autore. 
 */

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include "wator.h"
#include "util.h"

cell_t * planetRow = NULL;
FILE * dumpfilePtr = NULL; /* Stream relativo al file su cui effettuare il dump */
int socketRemoved = 0;     /* Flag che permette di evitare unlink multipli della socket */

/* Procedure di cleanup */
void visual_cleanup_1();
void visual_cleanup_2();

void term_handler(int sig); /* Handler per il segnale SIGTERM */

volatile sig_atomic_t need_to_stop;

int main(int argc, char * argv[])
{
	/* Maschero SIGINT */
	sigset_t sigset;
	SYSCALL_LIKE_NR(sigemptyset(&sigset), "sigemptyset");
	SYSCALL_LIKE_NR(sigaddset(&sigset, SIGINT), "sigaddset");
	PTHREAD(pthread_sigmask(SIG_BLOCK, &sigset, NULL), "pthread_sigmask");
	
	/* Definisco l'handler per il segnale SIGTERM (spedito da wator) */
	struct sigaction sigact;
	memset(&sigact, 0, sizeof(sigact));
	sigact.sa_handler = term_handler;
	SYSCALL_NR(sigaction(SIGTERM, &sigact, NULL), "sigaction");
	
	unlink(SOCK_PATH);
	socketRemoved = 1;
	
	dumpfilePtr = stdout;
	
	/* Registro la prima procedura di pulizia */
	if (atexit(visual_cleanup_1) != 0)
	{
		fprintf(stderr, "Could not register function via atexit.\n");
		exit(EXIT_FAILURE);
	}
	
	/* argv[1] contiene il nome del file su cui effettuare il dump, oppure '\0' */
	if (*(argv[1]) != '\0') 
	{
		dumpfilePtr = fopen(argv[1], "w");
		if (dumpfilePtr == NULL)
		{
			exit(EXIT_FAILURE);
		}
	}
	
	/* Prendo numero di righe e colonne del pianeta da stampare periodicamente */
	int rows, cols;
	char * errChar;
	
	rows = strtol(argv[2], &errChar, 10);
	if (*errChar != '\0')
	{	
		exit(EXIT_FAILURE);
	}
	cols = strtol(argv[3], &errChar, 10);
	if (*errChar != '\0')
	{
		exit(EXIT_FAILURE);
	}
	
	/* Definisco la socket per la comunicazione con collector */
	struct sockaddr_un sun;
	strncpy(sun.sun_path, SOCK_PATH, UNIX_PATH_MAX);
	sun.sun_family = AF_UNIX;
	
	int vsockFd; /* Socket privata di visualizer */
	SYSCALL(vsockFd, socket(AF_UNIX, SOCK_STREAM, 0), "socket");
	
	SYSCALL_NR(bind(vsockFd, (struct sockaddr*) &sun, sizeof(sun)), "bind");
	socketRemoved = 0;
	SYSCALL_NR(listen(vsockFd, SOMAXCONN), "listen");
	
	/* Avverto collector, che può adesso effettuare 'connect' */
	SYSCALL_NR(kill(getppid(), SIGUSR2), "kill");
	
	/* Registro la seconda procedura di pulizia */
	if (atexit(visual_cleanup_2) != 0)
	{
		fprintf(stderr, "Could not register function via atexit.\n");
		exit(EXIT_FAILURE);
	}
	
	planetRow = (cell_t*) malloc(cols * sizeof(cell_t));
	if (planetRow == NULL)
	{			
		exit(EXIT_FAILURE);
	}
	
	int wsockFd; /* Socket per la comunicazione con wator (collector in particolare) */
	while (1)
	{	
		/* Accolgo nuove connessioni sulla socket */
		while ((wsockFd = accept(vsockFd, NULL, NULL)) == -1)
		{
			if (errno != EINTR)
			{
				perror("accept");
				exit(errno);
			}
		}
		
		/* === Connessione stabilita === */
		
		/* Mi posiziono all'inizio del dumpfile */
		rewind(dumpfilePtr);
		
		fprintf(dumpfilePtr, "%d\n", rows);
		fprintf(dumpfilePtr, "%d\n", cols);
		
		for (int i = 0; i < rows; i++)
		{
			/* Leggo dalla socket (riga per riga)... */
			while (read(wsockFd, planetRow, cols * sizeof(cell_t)) == -1)
			{
				if (errno != EINTR)
				{
					perror("read");
					exit(errno);
				}
			}
			/* ...quindi scrivo sul dumpfile */
			for (int j = 0; j < cols; j++)
			{
				if (j == cols - 1)
				{
					fprintf(dumpfilePtr, "%c\n", cell_to_char(planetRow[j]));
				}
				else
				{
					fprintf(dumpfilePtr, "%c ", cell_to_char(planetRow[j]));
				}
			}
		}
		SYSCALL_NR(close(wsockFd), "close");
		
		/* C'è in sospeso un segnale di terminazione? */
		if (need_to_stop)
		{
			SYSCALL_NR(close(vsockFd), "close");
			return 0;
		}
	}
	
	return 0;
}

void visual_cleanup_1()
{	
	if (dumpfilePtr != NULL)
	{
		SYSCALL_LIKE_NR(fclose(dumpfilePtr), "fclose");
	}
	if (!socketRemoved)
	{
		unlink(SOCK_PATH);
	}
}

void visual_cleanup_2()
{
/*
	free(planetRow);
	planetRow = NULL;
	*/
}

void term_handler(int sig)
{
	need_to_stop = 1;
}
