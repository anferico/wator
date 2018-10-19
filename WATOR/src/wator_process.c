/** 
	\file       wator_process.c
    \author     Francesco Cariaggi
    \matricola  503146
    
  Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  originale dell' autore. 
 */

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include "wator.h"
#include "util.h"
#include "jobsrepo.h"
#define NWORK_DEF 4
#define CHRON_DEF 5
#define DURATION 1000  /* Durata (in chronon) della simulazione */
#define N rheight      /* Altezza dei rettangoli elaborati da ciascun thread 'worker' */
#define K rwidth       /* Larghezza dei rettangoli elaborati da ciascun thread 'worker' */
#define CHECKPOINT_FILE "./wator.check"

/* Funzioni d'inizio dei thread dispatcher, collector e worker */
void * dispatcher(void * arg);
void * collector(void * arg);
void * worker(void * arg);

FILE * chkfilePtr = NULL;   /* Stream realtivo al file di checkpoint */
int dumpPeriod = CHRON_DEF; /* Indica ogni quanti chronon deve avvenire un dump del pianeta */
int lastComputation = 0;    /* Indica se è l'ultima computazione da eseguire */
int totalJobs = 0;          /* Rettangoli da processare in totale per ogni chronon */
int jobsCompleted = 0;      /* Rettangoli processati (relativamente al chronon corrente) */
int canDispatch = 0;        /* Indica se 'dispatcher' può distribuire nuovi job */
int rheight = 0;            /* Altezza dei rettangoli elaborati da ciascun thread 'worker' */
int rwidth = 0;             /* Larghezza dei rettangoli elaborati da ciascun thread 'worker' */

pthread_mutex_t dispatcherMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t jobsCompletedMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lastComputationMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t repositoryMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bordersMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fsCountMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t dispatcherCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t jobsCompletedCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t emptyRepositoryCond = PTHREAD_COND_INITIALIZER;

/* Variabili da allocare sullo heap */
wator_t * wator = NULL;        /* Struttura di simulazione */
int ** dirty_cells_mat = NULL; /* Matrice di appoggio per evitare di aggiornare pesci/squali appena nati */
char * planfile = NULL;        /* Nome del file contenente la definizione del pianeta */
char * dumpfile = "";          /* Nome del file su cui eventualmente 'visualizer' effettuerà il dump */
pid_t watorPid;                /* processID del processo 'wator' */
pthread_t dispatcherTid;       /* threadID del thread 'dispatcher' */
pthread_t collectorTid;        /* threadID del thread 'collector' */
pthread_t * workerTids = NULL; /* Array dei threadID dei thread 'worker' */
int * wids = NULL;             /* Array degli wid (workerID) dei thread 'worker' */

jobsrepo_t * jobsRepository = NULL; /* Deposito di job da cui gli worker prelevano */

/* Procedure di cleanup */
void cleanup_checkpoint_1();
void cleanup_checkpoint_2();
void cleanup_checkpoint_3();
void cleanup_checkpoint_4();
void cleanup_checkpoint_5();
void main_thread_cleanup(void * arg);
void cleanup_checkpoint_D();

void usr1_handler (int sig);        /* Handler per il segnale SIGUSR1 */
void usr2_handler (int sig);        /* Handler per il segnale SIGUSR2 */
void termination_handler (int sig); /* Handler per i segnali SIGINT e SIGTERM */

/* Variabili globali manipolabili all'interno dei signal handler */
volatile sig_atomic_t visualizerPid;
volatile sig_atomic_t checkpoint_requested;
volatile sig_atomic_t termination_requested;
volatile sig_atomic_t visualizer_listening;

int main(int argc, char * argv[])
{
	watorPid = getpid();
	
	/* Maschero per il momento SIGINT e SIGTERM */
	sigset_t watorSigSet;
	SYSCALL_LIKE_NR(sigemptyset(&watorSigSet), "sigemptyset");
	SYSCALL_LIKE_NR(sigaddset(&watorSigSet, SIGINT), "sigaddset");
	SYSCALL_LIKE_NR(sigaddset(&watorSigSet, SIGTERM), "sigaddset");
	
	PTHREAD(pthread_sigmask(SIG_BLOCK, &watorSigSet, NULL), "pthread_sigmask");
	
	/* Definisco handler per i segnali SIGINT e SIGTERM */
	struct sigaction sigactTerm;
	memset(&sigactTerm, 0, sizeof(sigactTerm));
	sigactTerm.sa_handler = termination_handler;
	sigactTerm.sa_mask = watorSigSet;
	SYSCALL_NR(sigaction(SIGINT, &sigactTerm, NULL), "sigaction");
	SYSCALL_NR(sigaction(SIGTERM, &sigactTerm, NULL), "sigaction");
	
	/* Poi li smaschero */
	PTHREAD(pthread_sigmask(SIG_UNBLOCK, &watorSigSet, NULL), "pthread_sigmask");
	
	/* Maschero per il momento SIGUSR1 (e permanentemente SIGUSR2) */
	SYSCALL_LIKE_NR(sigemptyset(&watorSigSet), "sigemptyset");
	SYSCALL_LIKE_NR(sigaddset(&watorSigSet, SIGUSR1), "sigaddset");
	SYSCALL_LIKE_NR(sigaddset(&watorSigSet, SIGUSR2), "sigaddset");
	
	PTHREAD(pthread_sigmask(SIG_BLOCK, &watorSigSet, NULL), "pthread_sigmask");
	
	/* Definisco l'handler per il segnale SIGUSR1 */
	struct sigaction sigactUsr;
	memset(&sigactUsr, 0, sizeof(sigactUsr));
	sigactUsr.sa_handler = usr1_handler;
	SYSCALL_NR(sigaction(SIGUSR1, &sigactUsr, NULL), "sigaction");
	
	/* Poi lo smaschero */
	SYSCALL_LIKE_NR(sigemptyset(&watorSigSet), "sigemptyset");
	SYSCALL_LIKE_NR(sigaddset(&watorSigSet, SIGUSR1), "sigaddset");
	
	PTHREAD(pthread_sigmask(SIG_UNBLOCK, &watorSigSet, NULL), "pthread_sigmask");
	
	if (argc < 2)
	{
		fprintf(stderr, "Usage: wator file [-n nwork] [-v chronon] [-f dumpfile]\n");
		exit(EXIT_FAILURE);
	}
	
	/* ASSUNZIONE: il nome del file da cui caricare il pianeta deve essere
	               specificato come PRIMO parametro di 'wator'. */
	/* Se il file passato come argomento non esiste (o non è di tipo regular) esco */
	if (!file_exists_regular(argv[1]))
	{
		fprintf(stderr, "\"%s\" is not a valid file.\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	
	/* Primo checkpoint di pulizia */
	if (atexit(cleanup_checkpoint_1) != 0)
	{
		fprintf(stderr, "Could not register function via atexit.\n");
		exit(EXIT_FAILURE);
	}
	
	planfile = strdup(argv[1]); /* Allocazione sullo heap, attenzione */
	if (planfile == NULL)
	{
		perror("strdup");
		exit(errno);
	}	
	
	/* ===== Utilizzo il parser 'getopt' per analizzare le opzioni ===== */
	
	optind = 2; /* i.e. Inizio ad analizzare gli argomenti a partire dal secondo */
	opterr = 0; /* i.e. La gestione degli errori è a mio carico */
	
	const char * optstring = ":n:v:f:";
	/* i.e. Le opzioni consentite sono n, v, f. Ognuna di esse richiede un argomento (obbligatorio) */
	
	int nFlag = 0,
		vFlag = 0,
		fFlag = 0;
	int nwork;
	int chronon;
	
	char option;
	char * errChar;
	
	do
	{
		switch (option = getopt(argc, argv, optstring))
		{
			case 'n':
				if (nFlag != 0)
				{
					fprintf(stderr, "Option '-n' was set more than once.\n");
					exit(EXIT_FAILURE);
				}
				nFlag = 1;
				/* Mi aspetto che l'argomento sia un intero positivo */
				nwork = strtol(optarg, &errChar, 10);
				if (*errChar != '\0' || nwork <= 0)
				{
					fprintf(stderr, "Argument for option '-n' was not in a correct format.\n");
					exit(EXIT_FAILURE);
				}
			break;
			
			case 'v':
				if (vFlag != 0)
				{
					fprintf(stderr, "Option '-v' was set more than once.\n");
					exit(EXIT_FAILURE);
				}
				vFlag = 1;
				/* Mi aspetto che l'argomento sia un intero positivo */
				chronon = strtol(optarg, &errChar, 10);
				if (*errChar != '\0' || chronon <= 0)
				{
					fprintf(stderr, "Argument for option '-v' was not in a correct format.\n");
					exit(EXIT_FAILURE);
				}
			break;
			
			case 'f':
				if (fFlag != 0)
				{
					fprintf(stderr, "Option '-f' was set more than once.\n");
					exit(EXIT_FAILURE);
				}
				fFlag = 1;

				/* Secondo checkpoint di pulizia */
				if (atexit(cleanup_checkpoint_2) != 0)
				{
					fprintf(stderr, "Could not register function via atexit.\n");
					exit(EXIT_FAILURE);
				}
				
				dumpfile = strdup(optarg); /* Allocazione sullo heap, attenzione */
				if (dumpfile == NULL)
				{
					perror("strdup");
					exit(errno);
				}
			break;
			
			case ':':
				fprintf(stderr, "Missing argument for option '%c'.\n", optopt);
				exit(EXIT_FAILURE);
			break;
			
			case '?':
				fprintf(stderr, "Unrecognized option '%c'.\n", optopt);
				exit(EXIT_FAILURE);
			break;
		}
		
	} while (option != -1);
	
	/* Terzo checkpoint di pulizia */
	if (atexit(cleanup_checkpoint_3) != 0)
	{
		fprintf(stderr, "Could not register function via atexit.\n");
		exit(EXIT_FAILURE);
	}
	
	/* Inizializzo la struttura di simulazione */
	wator = new_wator(planfile);
	if (wator == NULL)
	{
		perror("new_wator");
		exit(errno);
	}
	
	wator->nwork = NWORK_DEF;
	wator->chronon = DURATION;
	
	free(planfile); /* Non mi serve più */
	planfile = NULL;
	
	if (nFlag)
	{
		wator->nwork = nwork;
	}
	if (vFlag)
	{
		dumpPeriod = chronon;
	}
	
	/* Setto la dimensione dei rettangoli */
	rheight = (int) ceil((double) wator->plan->nrow / nwork);
	rwidth = (int) ceil((double) wator->plan->ncol / nwork);
	
	/* Setto il totale dei job da portare a compimento per ogni chronon */
	totalJobs = (int) (ceil((double) wator->plan->nrow / rheight) * ceil((double) wator->plan->ncol / rwidth));
	
	/* Con questo escamotage faccio funzionare sempre il ciclo di 'collector' */
	jobsCompleted = totalJobs;
	
	/* fork del processo visualizer */
	char rows[10];
	char cols[10];
	switch (visualizerPid = fork())
	{
		case 0:
			if (snprintf(rows, 10, "%d", wator->plan->nrow) < 0)
			{
				exit(EXIT_FAILURE);
			}
			if (snprintf(cols, 10, "%d", wator->plan->ncol) < 0)
			{
				exit(EXIT_FAILURE);
			}
			execl("./visualizer", "visualizer", dumpfile, rows, cols, (char*) NULL);
			perror("execl");
		break;
		
		case -1:
			perror("fork");
			exit(errno);
		break;
	}
	
	/* Funzione di cleanup per il thread principale (wator) */
	pthread_cleanup_push(main_thread_cleanup, NULL);
	
	/* Checkpoint di pulizia del deposito dei job */
	if (atexit(cleanup_checkpoint_D) != 0)
	{
		fprintf(stderr, "Could not register function via atexit.\n");
		exit(EXIT_FAILURE);
	}
	
	/* Lancio dispatcher (modalità detached) */
	PTHREAD(pthread_create(&dispatcherTid, NULL, dispatcher, NULL), "pthread_create(dispatcher)");
	PTHREAD(pthread_detach(dispatcherTid), "pthread_detach(dispatcher)");
	
	/* Quarto checkpoint di pulizia */
	if (atexit(cleanup_checkpoint_4) != 0)
	{
		fprintf(stderr, "Could not register function via atexit.\n");
		exit(EXIT_FAILURE);
	}
	
	wids = (int*) malloc(wator->nwork * sizeof(int));
	if (wids == NULL)
	{
		errno = ENOMEM;
		exit(errno);
	}
	workerTids = (pthread_t*) malloc(wator->nwork * sizeof(pthread_t));
	if (workerTids == NULL)
	{
		errno = ENOMEM;
		exit(errno);
	}
	
	/* Lancio collector (modalità detached) */
	PTHREAD(pthread_create(&collectorTid, NULL, collector, NULL), "pthread_create(collector)");
	PTHREAD(pthread_detach(collectorTid), "pthread_detach(collector)");
	
	/* Lancio 'nwork' thread worker (modalità detached) */
	for (int i = 0; i < wator->nwork; i++)
	{
		wids[i] = i;
		PTHREAD(pthread_create(&(workerTids[i]), NULL, worker, &(wids[i])), "pthread_create(worker)");
		PTHREAD(pthread_detach(workerTids[i]), "pthread_detach(worker)");
	}
	
	pthread_cleanup_pop(0);
	
	/* Quinto checkpoint di pulizia */
	if (atexit(cleanup_checkpoint_5) != 0)
	{
		fprintf(stderr, "Could not register function via atexit.\n");
		exit(EXIT_FAILURE);
	}
	
	/* Definisco il sigset per la sospensione */
	sigset_t suspset;
	SYSCALL_LIKE_NR(sigfillset(&suspset), "sigfillset");
	SYSCALL_LIKE_NR(sigdelset(&suspset, SIGUSR1), "sigdelset");
	SYSCALL_LIKE_NR(sigdelset(&suspset, SIGINT), "sigdelset");
	SYSCALL_LIKE_NR(sigdelset(&suspset, SIGTERM), "sigdelset");
	
	/* ===== Da adesso in poi posso dedicarmi ai segnali ===== */
	
	while (1)
	{
		if (checkpoint_requested)
		{
			/* Apro il file su cui devo effetutare il checkpoint */
			chkfilePtr = fopen(CHECKPOINT_FILE, "w");
			if (chkfilePtr == NULL)
			{
				exit(EXIT_FAILURE);
			}
			
			SYSCALL_LIKE_NR(print_planet(chkfilePtr, wator->plan), "print_planet");
			
			checkpoint_requested = 0;
			
			/* Chiudo il file di checkpoint */
			if (fclose(chkfilePtr) != 0)
			{
				perror("fclose");
				exit(errno);
			}
			chkfilePtr = NULL;
		}
		if (termination_requested)
		{
			/* Avverto visualizer */
			SYSCALL_NR(kill(visualizerPid, SIGTERM), "kill");
			
			/* Setto il flag che indica che questa è l'ultima computazione da eseguire */
			PTHREAD(pthread_mutex_lock(&lastComputationMutex), "pthread_mutex_lock");
			lastComputation = 1;
			PTHREAD(pthread_mutex_unlock(&lastComputationMutex), "pthread_mutex_unlock");
			
			/* Aspetto visualizer */
			SYSCALL_NR(wait(NULL), "wait(wator_process)");  
			exit(EXIT_FAILURE);
		}
		
		/* Attendo passivamente nuovi segnali */
		if (sigsuspend(&suspset) == -1 && errno != EINTR)
		{
			perror("sigsuspend");
			exit(errno);
		}
	}
	
	return 0;
}

void * dispatcher(void * arg)
{
	/* Maschero i segnali che non devo gestire */
	sigset_t dispatcherSigSet;
	SYSCALL_LIKE_NR(sigemptyset(&dispatcherSigSet), "sigemptyset");
	SYSCALL_LIKE_NR(sigaddset(&dispatcherSigSet, SIGUSR1), "sigaddset");
	SYSCALL_LIKE_NR(sigaddset(&dispatcherSigSet, SIGUSR2), "sigaddset");
	SYSCALL_LIKE_NR(sigaddset(&dispatcherSigSet, SIGINT), "sigaddset");
	SYSCALL_LIKE_NR(sigaddset(&dispatcherSigSet, SIGTERM), "sigaddset");
	
	PTHREAD(pthread_sigmask(SIG_BLOCK, &dispatcherSigSet, NULL), "pthread_sigmask");
	
	while (1)
	{
		PTHREAD(pthread_mutex_lock(&dispatcherMutex), "pthread_mutex_lock");
		while (!canDispatch)
		{
			PTHREAD(pthread_cond_wait(&dispatcherCond, &dispatcherMutex), "pthread_cond_wait");
		}
		canDispatch = 0;
		PTHREAD(pthread_mutex_unlock(&dispatcherMutex), "pthread_mutex_unlock");
		
		/* ===== Posso adesso distribuire i job ===== */
		
		PTHREAD(pthread_mutex_lock(&repositoryMutex), "pthread_mutex_lock");
		/* Genero i job relativi al chronon corrente e li inserisco nel deposito */
		job_t singleJob;
		singleJob.rectHeight = N;
		int i;
		for (i = 0; i + N < wator->plan->nrow; i += N)
		{	
			singleJob.rectWidth = K;
			singleJob.topLeftX = i;
			int j;
			for (j = 0; j + K < wator->plan->ncol; j += K)
			{
				singleJob.topLeftY = j;
				SYSCALL_LIKE_NR(putone(&jobsRepository, singleJob), "putone");
			}
			
			/* Avanza qualche cella dal bordo destro del pianeta? */
			if (j < wator->plan->ncol)
			{
				singleJob.topLeftY = j;
				singleJob.rectWidth = wator->plan->ncol - j;
				SYSCALL_LIKE_NR(putone(&jobsRepository, singleJob), "putone");
			}
		}
		
		/* Avanza qualche cella dal bordo inferiore del pianeta? */
		if (i < wator->plan->nrow)
		{
			singleJob.topLeftX = i;
			singleJob.rectWidth = K;
			singleJob.rectHeight = wator->plan->nrow - i;
			
			int l;
			for (l = 0; l + K < wator->plan->ncol; l += K)
			{
				singleJob.topLeftY = l;
				SYSCALL_LIKE_NR(putone(&jobsRepository, singleJob), "putone");
			}
			
			/* Avanza qualche cella dall'area in basso a destra del pianeta? */
			if (l < wator->plan->ncol)
			{
				singleJob.topLeftY = l;
				singleJob.rectWidth = wator->plan->ncol - l;
				SYSCALL_LIKE_NR(putone(&jobsRepository, singleJob), "putone");
			}
		}
		
		PTHREAD(pthread_mutex_unlock(&repositoryMutex), "pthread_mutex_unlock");
		
		/* Avverto i thread 'worker' in attesa sul deposito vuoto */
		PTHREAD(pthread_cond_broadcast(&emptyRepositoryCond), "pthread_cond_broadcast");
	}
	
	return NULL;
}

void * collector(void * arg)
{
	/* Maschero i segnali che non devo gestire (più SIGUSR2)*/
	sigset_t collectorSigSet;
	SYSCALL_LIKE_NR(sigemptyset(&collectorSigSet), "sigemptyset");
	SYSCALL_LIKE_NR(sigaddset(&collectorSigSet, SIGUSR1), "sigaddset");
	SYSCALL_LIKE_NR(sigaddset(&collectorSigSet, SIGUSR2), "sigaddset");
	SYSCALL_LIKE_NR(sigaddset(&collectorSigSet, SIGINT), "sigaddset");
	SYSCALL_LIKE_NR(sigaddset(&collectorSigSet, SIGTERM), "sigaddset");
	
	PTHREAD(pthread_sigmask(SIG_BLOCK, &collectorSigSet, NULL), "pthread_sigmask");
	
	/* Definisco handler per il segnale SIGUSR2 */
	struct sigaction sigactUsr2;
	memset(&sigactUsr2, 0, sizeof(sigactUsr2));
	sigactUsr2.sa_handler = usr2_handler;
	SYSCALL_NR(sigaction(SIGUSR2, &sigactUsr2, NULL), "sigaction");
	
	/* Poi lo smaschero */
	SYSCALL_LIKE_NR(sigemptyset(&collectorSigSet), "sigemptyset");
	SYSCALL_LIKE_NR(sigaddset(&collectorSigSet, SIGUSR2), "sigaddset");
	PTHREAD(pthread_sigmask(SIG_UNBLOCK, &collectorSigSet, NULL), "pthread_sigmask");
	
	/* Definisco la socket di comunicazione con visualizer */
	struct sockaddr_un sun;
	strncpy(sun.sun_path, SOCK_PATH, UNIX_PATH_MAX);
	sun.sun_family = AF_UNIX;
	int csockFd; /* File descriptor della socket privata di collector */	
	
	/* Definisco il sigset per la sospensione */
	sigset_t collectorSuspset;
	SYSCALL_LIKE_NR(sigfillset(&collectorSuspset), "sigfillset");
	SYSCALL_LIKE_NR(sigdelset(&collectorSuspset, SIGUSR2), "sigdelset");
	
	int currentChronon = 1;
	while (1)
	{
		/* Comincio ad attendere che i thread worker portino a termine il lavoro */
		PTHREAD(pthread_mutex_lock(&jobsCompletedMutex), "pthread_mutex_lock");
		while (jobsCompleted < totalJobs)
		{
			PTHREAD(pthread_cond_wait(&jobsCompletedCond, &jobsCompletedMutex), "pthread_cond_wait");
		}
		PTHREAD(pthread_mutex_unlock(&jobsCompletedMutex), "pthread_mutex_unlock");
		
		jobsCompleted = 0;
		
		/* ===== Computazione relativa al chronon corrente completata ===== */
		
		/* Dealloco la matrice di appoggio usata durante la computazione precedente */
		free_int_mat(dirty_cells_mat, wator->plan->nrow);
		dirty_cells_mat = NULL;
		
		/* Inizializzo a zero la matrice di appoggio da usare nella prossima computazione */
		dirty_cells_mat = new_int_mat(wator->plan->nrow, wator->plan->ncol);
		if (dirty_cells_mat == NULL)
		{
			perror("new_int_mat");
			pthread_exit((void*) &errno);
		}
		
		PTHREAD(pthread_mutex_lock(&dispatcherMutex), "pthread_mutex_lock");
		PTHREAD(pthread_mutex_lock(&lastComputationMutex), "pthread_mutex_lock");
		if (!lastComputation)
		{
			canDispatch = 1;
			/* Avverto dispatcher, che può distribuire nuovi job */
			PTHREAD(pthread_cond_signal(&dispatcherCond), "pthread_cond_signal");
		}
		PTHREAD(pthread_mutex_unlock(&lastComputationMutex), "pthread_mutex_lock");
		PTHREAD(pthread_mutex_unlock(&dispatcherMutex), "pthread_mutex_unlock");
		
		PTHREAD(pthread_mutex_lock(&lastComputationMutex), "pthread_mutex_lock");
		
		/* E' stata raggiunta la durata totale della simulazione? */
		if (currentChronon == wator->chronon)
		{
			lastComputation = 1;
			SYSCALL_NR(kill(watorPid, SIGINT), "kill");
		}
		
		/* E' il momento di richiedere il dump del pianeta? */
		/* O in alternativa, era l'ultima computazione da eseguire? */
		if (currentChronon % dumpPeriod == 0 || lastComputation)
		{
			/* Attendo finché visualizer non è pronto ad accogliere connessioni */
			while (!visualizer_listening)
			{
				if (sigsuspend(&collectorSuspset) == -1 && errno != EINTR)
				{
					perror("sigsuspend");
					exit(errno);
				}
			}
			
			/* ===== visualizer si è messo in ascolto sulla socket ===== */
			
			/* Creo la socket */
			SYSCALL(csockFd, socket(AF_UNIX, SOCK_STREAM, 0), "socket(collector)");
			SYSCALL_NR(connect(csockFd, (struct sockaddr*) &sun, sizeof(sun)), "connect(collector)");
			
			/* Scrivo il pianeta sulla socket (riga per riga) */
			for (int i = 0; i < wator->plan->nrow; i++)
			{
				SYSCALL_NR(write(csockFd, wator->plan->w[i], wator->plan->ncol * sizeof(cell_t)), "write(collector)");
			}
			
			SYSCALL_NR(close(csockFd), "close(collector)");
			
			/* Era in particolare l'ultima computazione da eseguire? */
			if (lastComputation)
			{ 
				/* Uccido dispatcher e tutti gli worker */
				PTHREAD(pthread_kill(dispatcherTid, SIGKILL), "pthread_kill(dispatcher)");
				for (int k = 0; k < wator->nwork; k++)
				{
					PTHREAD(pthread_kill(workerTids[k], SIGKILL), "pthread_kill(worker)");
				}
				
				return NULL;
			}
		}
		PTHREAD(pthread_mutex_unlock(&lastComputationMutex), "pthread_mutex_unlock");
		
		currentChronon++;
	}
	
	return NULL;
}

void * worker(void * arg)
{
	/* Maschero i segnali che non devo gestire */
	sigset_t workerSigSet;
	SYSCALL_LIKE_NR(sigemptyset(&workerSigSet), "sigemptyset");
	SYSCALL_LIKE_NR(sigaddset(&workerSigSet, SIGUSR1), "sigaddset");
	SYSCALL_LIKE_NR(sigaddset(&workerSigSet, SIGUSR2), "sigaddset");
	SYSCALL_LIKE_NR(sigaddset(&workerSigSet, SIGINT), "sigaddset");
	SYSCALL_LIKE_NR(sigaddset(&workerSigSet, SIGTERM), "sigaddset");
	
	PTHREAD(pthread_sigmask(SIG_BLOCK, &workerSigSet, NULL), "pthread_sigmask");
	
	int wid = *(int*) arg;
	char * basename = "wator_worker_";
	char fullname[18];
	
	if (snprintf(fullname, 18, "%s%d", basename, wid) < 0)
	{
		exit(EXIT_FAILURE);
	}
	
	/* Creo il file vuoto wator_worker_wid, poi lo chiudo */
	FILE * widfilePtr = fopen(fullname, "w");
	if (widfilePtr == NULL)
	{
		exit(EXIT_FAILURE);
	}
	if (fclose(widfilePtr) != 0)
	{
		perror("fclose");
		exit(errno);
	}
	
	job_t ownJob; /* Dettagli del singolo job prelevato dal deposito */
	while (1)
	{
		/* Attendo finché il deposito non contiene almeno 1 job */
		PTHREAD(pthread_mutex_lock(&repositoryMutex), "pthread_mutex_lock");
		
		while (jobsRepository == NULL)
		{
			PTHREAD(pthread_cond_wait(&emptyRepositoryCond, &repositoryMutex), "pthread_cond_wait");
		}
		
		/* Prelevo un job dal deposito */
		SYSCALL_LIKE_NR(pickone(&jobsRepository, &ownJob), "pickone(worker)");
		PTHREAD(pthread_mutex_unlock(&repositoryMutex), "pthread_mutex_unlock");
		
		/* ===== Applico le regole ===== */
		
		/* Indica se ho acquisito la lock perché nelle vicinanze del bordo inferiore o superiore */
		int lock_acquired_ud = 0;
		/* Indica se ho acquisito la lock perché nelle vicinanze del bordo destro o sinistro */ 
		int lock_acquired_lr = 0;
		/* Indici di riga e colonna (assoluti) da usare per accedere al pianeta */
		int rowIndex, colIndex;
		
		/* Prima le regole degli squali */
		int u, v;
		for (int i = 0; i < ownJob.rectHeight; i++)
		{
			/* Mi trovo a distanza 0 o 1 dal bordo inferiore o superiore del rettangolo? */
			if (i == 0 || i == 1 || i == ownJob.rectHeight - 1 || i == ownJob.rectHeight - 2)
			{
				PTHREAD(pthread_mutex_lock(&bordersMutex), "pthread_mutex_lock");
		
				lock_acquired_ud = 1;
			}
			
			for (int j = 0; j < ownJob.rectWidth; j++)
			{
				/* Se non sono già a distanza 0 o 1 dal bordo inferiore o superiore del rettangolo: */
				if (!lock_acquired_ud)
				{
					/* Mi trovo a distanza 0 o 1 dal bordo destro o sinistro del rettangolo? */
					if (j == 0 || j == 1 || j == ownJob.rectWidth - 1 || j == ownJob.rectWidth -2)
					{
						PTHREAD(pthread_mutex_lock(&bordersMutex), "pthread_mutex_lock");
						
						lock_acquired_lr = 1;
					}
				}
				
				/* Coord. assolute = origine + coord. relative */
				rowIndex = ownJob.topLeftX + i;
				colIndex = ownJob.topLeftY + j;
				
				if (wator->plan->w[rowIndex][colIndex] == SHARK && dirty_cells_mat[rowIndex][colIndex] == 0)
				{
					PTHREAD(pthread_mutex_lock(&fsCountMutex), "pthread_mutex_lock");
					if (shark_rule_1w(wator, rowIndex, colIndex, &u, &v) == -1)
					{
						free_int_mat(dirty_cells_mat, wator->plan->nrow);
						errno = EINVAL;
						pthread_exit((void*) &errno);
					}
					dirty_cells_mat[u][v] = 1;
					
					if (shark_rule_2w(wator, u, v, &u, &v) == -1)
					{
						free_int_mat(dirty_cells_mat, wator->plan->nrow);
						errno = EINVAL;
						pthread_exit((void*) &errno);
					}
					dirty_cells_mat[u][v] = 1;
					PTHREAD(pthread_mutex_unlock(&fsCountMutex), "pthread_mutex_unlock");
				}
				
				/* Rilascio eventualmente la lock */
				if (lock_acquired_lr)
				{
					PTHREAD(pthread_mutex_unlock(&bordersMutex), "pthread_mutex_unlock");
					lock_acquired_lr = 0;
				}
			}
			
			/* Rilascio eventualmente la lock */
			if (lock_acquired_ud)
			{
				PTHREAD(pthread_mutex_unlock(&bordersMutex), "pthread_mutex_unlock");
				lock_acquired_ud = 0;
			}
		}
		
		/* Poi quelle dei pesci */
		int w, z;
		for (int i = 0; i < ownJob.rectHeight; i++)
		{
			/* Mi trovo a distanza 0 o 1 dal bordo inferiore o superiore del rettangolo? */
			if (i == 0 || i == 1 || i == ownJob.rectHeight - 1 || i == ownJob.rectHeight - 2)
			{
				PTHREAD(pthread_mutex_lock(&bordersMutex), "pthread_mutex_lock");
				lock_acquired_ud = 1;
			}
			
			for (int j = 0; j < ownJob.rectWidth; j++)
			{
				/* Se non sono già a distanza 0 o 1 dal bordo inferiore o superiore del rettangolo: */
				if (!lock_acquired_ud)
				{
					/* Mi trovo a distanza 0 o 1 dal bordo destro o sinistro del rettangolo? */
					if (j == 0 || j == 1 || j == ownJob.rectWidth - 1 || j == ownJob.rectWidth -2)
					{
						PTHREAD(pthread_mutex_lock(&bordersMutex), "pthread_mutex_lock");
						lock_acquired_lr = 1;
					}
				}
				
				/* Coord. assolute = origine + coord. relative */
				rowIndex = ownJob.topLeftX + i;
				colIndex = ownJob.topLeftY + j;
				
				if (wator->plan->w[rowIndex][colIndex] == FISH && dirty_cells_mat[rowIndex][colIndex] == 0)
				{
					PTHREAD(pthread_mutex_lock(&fsCountMutex), "pthread_mutex_lock");
					if (fish_rule_4w(wator, rowIndex, colIndex, &w, &z) == -1)
					{
						free_int_mat(dirty_cells_mat, wator->plan->nrow);
						errno = EINVAL;
						pthread_exit((void*) &errno);
					}
					dirty_cells_mat[w][z] = 1;
					PTHREAD(pthread_mutex_unlock(&fsCountMutex), "pthread_mutex_unlock");
					
					if (fish_rule_3w(wator, rowIndex, colIndex, &w, &z) == -1)
					{
						free_int_mat(dirty_cells_mat, wator->plan->nrow);
						errno = EINVAL;
						pthread_exit((void*) &errno);
					}
					dirty_cells_mat[w][z] = 1;	
				}
				
				/* Rilascio eventualmente la lock */
				if (lock_acquired_lr)
				{
					PTHREAD(pthread_mutex_unlock(&bordersMutex), "pthread_mutex_unlock");
					lock_acquired_lr = 0;
				}
			}
			
			/* Rilascio eventualmente la lock */
			if (lock_acquired_ud)
			{
				PTHREAD(pthread_mutex_unlock(&bordersMutex), "pthread_mutex_unlock");
				lock_acquired_ud = 0;
			}
		}
		
		/* Segnalo che ho finito un job */
		PTHREAD(pthread_mutex_lock(&jobsCompletedMutex), "pthread_mutex_lock");
		jobsCompleted++;
		
		/* Era anche l'ultimo job di questo chronon? */
		if (jobsCompleted == totalJobs)
		{
			PTHREAD(pthread_cond_signal(&jobsCompletedCond), "pthread_cond_signal");
		}
		
		PTHREAD(pthread_mutex_unlock(&jobsCompletedMutex), "pthread_mutex_unlock");
	}
	
	return NULL;
}

void main_thread_cleanup(void * arg)
{
	exit(EXIT_FAILURE);
}

void cleanup_checkpoint_1()
{
	free(planfile);
	planfile = NULL;
}

void cleanup_checkpoint_2()
{
	free(dumpfile);
	dumpfile = NULL;
}

void cleanup_checkpoint_3()
{
	free_int_mat(dirty_cells_mat, wator->plan->nrow);
	dirty_cells_mat = NULL;
	free_wator(wator);
	wator = NULL;
}

void cleanup_checkpoint_4()
{
	free(wids);
	wids = NULL;
	free(workerTids);
	workerTids = NULL;
}

void cleanup_checkpoint_5()
{
	if (chkfilePtr != NULL)
	{
		SYSCALL_LIKE_NR(fclose(chkfilePtr), "fclose(cleanup5)");
		chkfilePtr = NULL;
	}
}

void cleanup_checkpoint_D()
{
	free_repository(&jobsRepository);
	jobsRepository = NULL;
}

void usr1_handler(int sig)
{
	checkpoint_requested = 1;
}

void usr2_handler(int sig)
{
	visualizer_listening = 1;
}

void termination_handler(int sig)
{
	termination_requested = 1;
}

