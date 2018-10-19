/** 
	\file       util.h
    \author     Francesco Cariaggi
    \matricola  503146
    
  Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  originale dell' autore. 
 */

#ifndef __UTIL__H
#define __UTIL__H

#include "wator.h"
#include <errno.h>
#include <pthread.h>

/** Macro per rendere più leggibile il codice */
#define SET_RET(e, ret) {errno = e; return ret;}

/** Macro per le SC */
#define SYSCALL(res, fun, err) if ((res=fun)==-1) { perror(err);exit(errno); }

/** Macro per le SC (non memorizzo valore di return) */
#define SYSCALL_NR(fun, err) if ((fun)==-1) { perror(err);exit(errno); }

/** Macro per chiamate di libreria con comportamento simile a SC */
#define SYSCALL_LIKE(res, fun, err) SYSCALL(res, fun, err)

/** Macro per le chiamate di libreria con comportamento simile a SC (non memorizzo valore di return) */
#define SYSCALL_LIKE_NR(fun, err) SYSCALL_NR(fun, err)

/** Macro per le chiamate pthread */
#define PTHREAD(fun, errMsg)                 		\
		{                                       	\
			int res;                            	\
			if((res = fun) != 0)                 	\
			{                                   	\
				errno = res;                      	\
				perror(errMsg);                 	\
				pthread_exit((void*) &errno);   	\
			}                                   	\
		}

#define SOCK_PATH "./visual.sck"
#define UNIX_PATH_MAX 108

/** Costante che rappresenta il path del file di configurazione (CONFIGURATION_FILE) */
/** (In questo caso sto supponendo che il file sia nella directory corrente) */
static const char BASE_PATH[] = "./";

/** Calcola il valore di x modulo y (anche per x negativo). Es.: mod(-2, 5) = 3
   \param x valore
   \param y modulo
   
   \retval x modulo y
 */
int mod(int x, int y);

/** Ottiene la sottostringa costituita dai primi 2 caratteri di str, e la memorizza in dest.
   \param str stringa da cui estrarre la sottostringa
   \param dest buffer di destinazione
   
   \retval stringa costituita dai primi 2 caratteri di str (più il terminatore '\0')
   \retval NULL se str è lunga meno di 2 carratteri
 */
char * substring2(char * str, char dest[3]);

/** Crea una nuova matrice di interi inizializzata a zero (memorizzata per righe)
   \param rows numero di righe della matrice
   \param cols numero di colonne della matrice
   
   \retval la nuova matrice appena creata
   \retval NULL se si è verificato un errore
 */
int ** new_int_mat(int rows, int cols);

/** Dealloca una matrice di interi (memorizzata per righe)
   \param mat matrice di interi da deallocare
   \param rows numero di righe della matrice
 */
void free_int_mat(int ** mat, unsigned int rows);

/** Configura la struttura di simulazione in base ai contenuti del file di 
    configurazione.
   \param wator puntatore alla struttura di simulazione da configurare
   \param confFilePath path del file di configurazione

   \retval 0 se è andato tutto bene
   \retval -1 se si è verificato qualche problema (setta errno)
 */
int configure(wator_t * wator, char * confFilePath);

/** Restituisce il puntatore alla cella situata sopra quella di coordinate (x,y).
	Se 'u' e 'v' sono diversi da NULL, in essi vengono memorizzate le coordinate
	di quella cella.
   \param w puntatore alla struttura di simulazione
   \param x coordinata x della cella
   \param y coordinata y della cella
   \param u puntatore alla una zona di memoria in cui viene memorizzata la coordinata x
    della cella situata sopra quella di coordinate (x,y) (solo se u != NULL)
   \param v puntatore alla una zona di memoria in cui viene memorizzata la coordinata y
    della cella situata sopra quella di coordinate (x,y) (solo se v != NULL)
            
   \retval puntatore alla cella situata sopra quella di coordinate (x,y)
 */
cell_t * up(wator_t * w, int x, int y, int * u, int * v);

/** Restituisce il puntatore alla cella situata sotto quella di coordinate (x,y).
	Se 'u' e 'v' sono diversi da NULL, in essi vengono memorizzate le coordinate
	di quella cella.
   \param w puntatore alla struttura di simulazione
   \param x coordinata x della cella
   \param y coordinata y della cella
   \param u puntatore alla una zona di memoria in cui viene memorizzata la coordinata x
    della cella situata sotto quella di coordinate (x,y) (solo se u != NULL)
   \param v puntatore alla una zona di memoria in cui viene memorizzata la coordinata y
    della cella situata sotto quella di coordinate (x,y) (solo se v != NULL)
            
   \retval puntatore alla cella situata sotto quella di coordinate (x,y)
 */
cell_t * down(wator_t * w, int x, int y, int * u, int * v);

/** Restituisce il puntatore alla cella situata a sinistra di quella di coordinate (x,y).
	Se 'u' e 'v' sono diversi da NULL, in essi vengono memorizzate le coordinate
	di quella cella.
   \param w puntatore alla struttura di simulazione
   \param x coordinata x della cella
   \param y coordinata y della cella
   \param u puntatore alla una zona di memoria in cui viene memorizzata la coordinata x
    della cella situata a sinistra di quella di coordinate (x,y) (solo se u != NULL)
   \param v puntatore alla una zona di memoria in cui viene memorizzata la coordinata y
    della cella situata a sinistra di quella di coordinate (x,y) (solo se v != NULL)
            
   \retval puntatore alla cella situata a sinistra di quella di coordinate (x,y)
 */
cell_t * left(wator_t * w, int x, int y, int * u, int * v);

/** Restituisce il puntatore alla cella situata a destra di quella di coordinate (x,y).
	Se 'u' e 'v' sono diversi da NULL, in essi vengono memorizzate le coordinate
	di quella cella.
   \param w puntatore alla struttura di simulazione
   \param x coordinata x della cella
   \param y coordinata y della cella
   \param u puntatore alla una zona di memoria in cui viene memorizzata la coordinata x
    della cella situata a destra quella di coordinate (x,y) (solo se u != NULL)
   \param v puntatore alla una zona di memoria in cui viene memorizzata la coordinata y
    della cella situata a destra di quella di coordinate (x,y) (solo se v != NULL)
            
   \retval puntatore alla cella situata a destra di quella di coordinate (x,y)
 */
cell_t * right(wator_t * w, int x, int y, int * u, int * v);

/** Raccoglie i puntatori alle celle adiacenti a quella di coordinate (x,y).
	
	Data la cella (x,y):
	array slots -> [*|*|*|*]
         sopra(x,y) ^        
           sotto(x,y) ^
          sinistra(x,y) ^
              destra(x,y) ^
	
   \param wator puntatore alla struttura di simulazione
   \param x coordinata x della cella
   \param y coordinata y della cella
   \param slots array in cui vengono memorizzati i puntatori alle celle
          adiacenti a quella situata in (x,y)
   
   \retval 0 se è andato tutto bene
   \retval -1 se si è verificato un problema
 */
int nearby(wator_t * wator, int x, int y, cell_t * slots[4]);

/** Valida un pianeta
   \param p puntatore al pianeta
   
   \retval 0 se il pianeta è valido
   \retval -1 altrimenti
 */
int validate_planet(planet_t * p);

/** Valida una struttura di simulazione
   \param w puntatore alla struttura di simulazione

   \retval 0 se la struttura è valida
   \retval -1 altrimenti
 */
int validate_wator(wator_t * w);

/** Determina l'esistenza di un file (regolare)
   \param filename nome del file

   \retval 1 se il file esiste ed è di tipo 'regular'
   \retval 0 altrimenti
 */
int file_exists_regular(char * filename);

/** Implementazione di strdup come quella di glibc
   \param s stringa da duplicare
   
   \retval una copia della stringa (allocata sullo heap)
 */
char * strdup (const char * s);

/** Versione "alternativa" di 'shark_rule_1' pensata per i thread worker.
   LA SPECIFICA E' LA STESSA DI 'shark_rule_1' DEFINITA ALL'INTERNO DEL FILE wator.h
 */
int shark_rule_1w(wator_t * pw, int x, int y, int * k, int * l);

/** Versione "alternativa" di 'shark_rule_2' pensata per i thread worker.
   LA SPECIFICA E' LA STESSA DI 'shark_rule_2' DEFINITA ALL'INTERNO DEL FILE wator.h
 */
int shark_rule_2w(wator_t * pw, int x, int y, int * k, int * l);

/** Versione "alternativa" di 'fish_rule_3' pensata per i thread worker.
   LA SPECIFICA E' LA STESSA DI 'fish_rule_3' DEFINITA ALL'INTERNO DEL FILE wator.h
 */
int fish_rule_3w(wator_t * pw, int x, int y, int * k, int * l);

/** Versione "alternativa" di 'fish_rule_4' pensata per i thread worker.
   LA SPECIFICA E' LA STESSA DI 'fish_rule_4' DEFINITA ALL'INTERNO DEL FILE wator.h
 */
int fish_rule_4w(wator_t * pw, int x, int y, int * k, int * l);

#endif

