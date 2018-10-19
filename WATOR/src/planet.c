/** 
	\file       planet.c
    \author     Francesco Cariaggi
    \matricola  503146
    
  Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  originale dell' autore. 
 */

#include "wator.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

planet_t * new_planet (unsigned int nrow, unsigned int ncol)
{
	if (nrow <= 0 || ncol <= 0)
	{
		return NULL;
	}
	
	planet_t * planet = (planet_t*) malloc(sizeof(planet_t));
	
	if (planet == NULL)
	{
		return NULL;
	}
	
	planet->nrow = nrow;
	planet->ncol = ncol;
	
	/* Creazione matrici: */
	/* Matrice pianeta */
	planet->w = (cell_t**) malloc(planet->nrow * sizeof(cell_t*));
	if (planet->w == NULL)
	{
		free_planet(planet);
		return NULL;
	}
	/* Matrice btime */
	planet->btime = (int**) malloc(planet->nrow * sizeof(int*));
	if (planet->btime == NULL)
	{
		free_planet(planet);
		return NULL;
	}
	/* Matrice dtime */
	planet->dtime = (int**) malloc(planet->nrow * sizeof(int*));
	if (planet->dtime == NULL)
	{
		free_planet(planet);
		return NULL;
	}
	
	/* Inizializzazione matrici: */
	/* Matrice pianeta */
	for (int i = 0; i < planet->nrow; i++)
	{
		planet->w[i] = (cell_t*) malloc(planet->ncol * sizeof(cell_t));
		if (planet->w[i] == NULL)
		{
			free_planet(planet);
			return NULL;
		}
	}
	for (int i = 0; i < planet->nrow; i++)
	{
		for (int j = 0; j < planet->ncol; j++)
		{
			/* WATER ovunque */
			planet->w[i][j] = WATER;
		}
	}
	/* Matrice btime (con calloc) */
	for (int i = 0; i < planet->nrow; i++)
	{
		planet->btime[i] = (int*) calloc(planet->ncol, sizeof(int));
		if (planet->btime[i] == NULL)
		{
			free_planet(planet);
			return NULL;
		}
	}
	/* Matrice dtime (con calloc) */
	for (int i = 0; i < planet->nrow; i++)
	{
		planet->dtime[i] = (int*) calloc(planet->ncol, sizeof(int));
		if (planet->dtime[i] == NULL)
		{
			free_planet(planet);
			return NULL;
		}
	}
	
	return planet;
}

void free_planet (planet_t * p)
{
	/* Niente da liberare */
	if (p == NULL)
	{
		return;
	}
	
	/* free della matrice pianeta */
	/* Possibile imprevisto dopo la malloc */
	if (p->w != NULL)
	{
		printf("p->w != NULL\n");
		fflush(stdout);
		/* free dei puntatori agli elementi cell_t */
		for (int i = 0; i < p->nrow; i++)
		{
			printf("free_riga %d-start\n", i);
			fflush(stdout);
			if (p->w[i] != NULL)
			{
				free(p->w[i]);
				p->w[i] = NULL;
			}
			printf("free_riga %d-end\n", i);
			fflush(stdout);
		}
		
		/* free dei puntatori (cell_t*) alle righe */
		free(p->w);
		p->w = NULL;
	}
	/* free della matrice 'btime' */
	if (p->btime != NULL)
	{
		/* free dei puntatori agli elementi int */
		for (int i = 0; i < p->nrow; i++)
		{
			free(p->btime[i]);
			p->btime[i] = NULL;
		}
		
		/* free dei puntatori (int*) alle righe */
		free(p->btime);
		p->btime = NULL;
	}
	/* free della matrice 'dtime' */
	if (p->dtime != NULL)
	{
		/* Free dei puntatori agli elementi int */
		for (int i = 0; i < p->nrow; i++)
		{
			free(p->dtime[i]);
			p->dtime[i] = NULL;
		}
	
		/* Free dei puntatori (int*) alle righe */
		free(p->dtime);
		p->dtime = NULL;
	}
	
	/* free dell'intero pianeta */
	free(p);
}

int print_planet (FILE * f, planet_t * p) /* Setta errno */
{
	if (p == NULL)
	{
		SET_RET(EINVAL, -1);
	}
	
	char convChar;
	
	/* Suppongo che f sia già stato aperto in scrittura */
	
	if (fprintf(f, "%u\n", p->nrow) == -1)
	{
		SET_RET(EBADF, -1);
	}
	if (fprintf(f, "%u\n", p->ncol) == -1)
	{
		SET_RET(EBADF, -1);
	}
	
	for (int i = 0; i < p->nrow; i++)
	{
		for (int j = 0; j < p->ncol; j++)
		{
			convChar = cell_to_char(p->w[i][j]);
			if (convChar == '?')
			{
				SET_RET(EINVAL, -1);
			}
			
			/* Stampo il contenuto di ogni cella (dopo l'ultima non stampo lo spazio) */
			if (j == p->ncol - 1)
			{
				fprintf(f, "%c", convChar);
			}
			else
			{
				fprintf(f, "%c ", convChar);
			}
		}
		fprintf(f, "\n");
	}
	
	return 0;
}

planet_t * load_planet (FILE * f) /* Setta errno */
{
	/* File già aperto in lettura */
	
	unsigned int urows;
	unsigned int ucols;
	
	char buf[15];
	char * err;
	int nrow;
	int ncol;
	
/*  --- righe ---  */
	if (fgets(buf, 15, f) == NULL)
	{
		SET_RET(ERANGE, NULL);
	}
	
	// Prendo il numero di righe
	nrow = strtol(buf, &err, 10);
	if (*err != '\0' && *err != '\n')
	{
		SET_RET(ERANGE, NULL);
	}
/* --------------  */
/* --- colonne --- */
	if (fgets(buf, 15, f) == NULL)
	{
		SET_RET(ERANGE, NULL);
	}
	
	/* Prendo il numero di colonne */
	ncol = strtol(buf, &err, 10);
	if (*err != '\0' && *err != '\n')
	{
		SET_RET(ERANGE, NULL);
	}
// --------------- */
	
	if (nrow <= 0 || ncol <= 0)
	{
		/* Problema */
		SET_RET(ERANGE, NULL);
	}
	
	urows = nrow;
	ucols = ncol;
	
	planet_t * planet = new_planet(urows, ucols);
	if (planet == NULL)
	{
		SET_RET(ENOMEM, NULL);
	}
	
	char * longBuf;
	/* (2 * planet->ncol) per ogni cella (separate da spazi) + (2) per '\n' e '\0' */
	unsigned int bufSize = (2 * planet->ncol + 2);
	int index;
	cell_t cell;
	
	/* Riempimento pianeta da file */
	for (int i = 0; i < planet->nrow; i++)
	{
		longBuf = (char*) malloc(bufSize * sizeof(char));
		index = 0;
		
		/* Leggo una riga */
		if (fgets(longBuf, bufSize, f) == NULL)
		{	
			free_planet(planet);
			free(longBuf);
			SET_RET(ERANGE, NULL);
		}
		
		for (int j = 0; index < planet->ncol; j++)
		{
			if (j % 2 == 1) /* Negli indici dispari (1, 3, 5, ...) mi aspetto uno spazio */
			{
				if (longBuf[j] != ' ')
				{
					free_planet(planet);
					free(longBuf);
					SET_RET(ERANGE, NULL);
				}
			}
			else /* Negli indici pari (0, 2, 4, ...) mi aspetto un carattere valido */
			{
				cell = char_to_cell(longBuf[j]);
				if (cell == -1)
				{
					free_planet(planet);
					free(longBuf);
					SET_RET(ERANGE, NULL);
				}
				planet->w[i][index] = cell;
				index++;
			}
		}
		free(longBuf);
	}
	
	char dummyBuf[2];
	
	/* Se esiste ulteriore contenuto nel file dopo le 'nrow' righe lette ho un problema */
	if (fgets(dummyBuf, 2, f) != NULL)
	{
		free_planet(planet);
		SET_RET(ERANGE, NULL);
	}
	
	return planet;
}

int fish_count(planet_t * p)
{
	if (validate_planet(p) == -1)
	{
		SET_RET(EINVAL, -1);
	}
	
	int count = 0;
	for (int i = 0; i < p->nrow; i++)
	{
		for (int j = 0; j < p->ncol; j++)
		{
			if (p->w[i][j] == FISH)
			{
				count++;
			}
		}
	}
	return count;
}

int shark_count(planet_t * p)
{
	if (validate_planet(p) == -1)
	{
		SET_RET(EINVAL, -1);
	}
	
	int count = 0;
	for (int i = 0; i < p->nrow; i++)
	{
		for (int j = 0; j < p->ncol; j++)
		{
			if (p->w[i][j] == SHARK)
			{
				count++;
			}
		}
	}
	return count;
}
