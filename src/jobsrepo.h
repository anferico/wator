/** 
	\file       jobsrepo.h
    \author     Francesco Cariaggi
    \matricola  503146
    
  Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  originale dell' autore. 
 */

#ifndef __JOBSREPO__H
#define __JOBSREPO__H

/** Contiene le informazioni necessarie ad ogni thread worker per svolgere il prorio lavoro */
typedef struct job
{
	/** Coordinata X dell'origine di questo rettangolo */
	int topLeftX;
	/** Coordinata Y dell'origine di questo rettangolo */
	int topLeftY;
	/** Lunghezza del rettangolo */
	int rectWidth;
	/** Altezza del rettangolo */
	int rectHeight;
} job_t;

/** Lista concatenata di job_t su cui 'dispatcher' inserisce i vari job */
typedef struct jobsrepo
{
	/** Valore informativo */
	job_t job;
	/** Puntatore all'elemento successivo nella lista */
	struct jobsrepo * next;
} jobsrepo_t;

/** Inserisce un job nel deposito dei job
   \param repo puntatore al deposito su cui inserire il job
   \param value il valore da inserire

   \retval 0 se è andato tutto bene 
   \retval -1 se si è verificato un errore
 */
int putone(jobsrepo_t ** repo, job_t value);

/** Estrae (rimuovendolo) un job dal deposito
   \param repo puntatore al deposito da cui estrarre un job
   \param jb puntatore alla struttura job_t sulla quale si vuole che sia memorizzato il valore estratto

   \retval 0 se è andato tutto bene
   \retval -1 in caso di errore
 */
int pickone(jobsrepo_t ** repo, job_t * jb);

/** Dealloca dall'heap un intero deposito
   \param repo il deposito da deallocare
 */
void free_repository(jobsrepo_t ** repo);

#endif

