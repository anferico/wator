/** 
	\file       wator.c
    \author     Francesco Cariaggi
    \matricola  503146
    
  Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  originale dell' autore. 
 */

#include "wator.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

wator_t * new_wator (char * fileplan)
{
	wator_t * wator = (wator_t*) malloc(sizeof(wator_t));
	if (wator == NULL)
	{
		SET_RET(ENOMEM, NULL);
	}
	
	FILE * planFilePtr = fopen(fileplan, "r");
	if (planFilePtr == NULL)
	{
		free_wator(wator);
		fclose(planFilePtr);
		/* errno settata da fopen */
		return NULL;
	}
	
	planet_t * plan = load_planet(planFilePtr);
	if (plan == NULL)
	{
		/* errno settata da load_planet, voglio mantenerne il valore */
		int errnoDup = errno;
		
		free_wator(wator);
		fclose(planFilePtr);
		
		/* fclose potrebbe aver cambiato il valore di errno */
		SET_RET(errnoDup, NULL);
	}
	fclose(planFilePtr);
	
	/* Attacco il pianeta alla struttura di simulazione */
	wator->plan = plan;
	
	/* Comincio a costruire il path del file di configurazione */
	char holder[100];
	strcpy(holder, BASE_PATH);
	
	char * confFilePath = strcat(holder, CONFIGURATION_FILE);
	
	/* A questo punto ho il path del file di configurazione */
	
	if (configure(wator, confFilePath) == -1)
	{
		/* errno settata da configure */
		free_wator(wator);
		return NULL;
	}
	
	/* Setto altri parametri della simulazione */
	int fc = fish_count(wator->plan);
	int sc = shark_count(wator->plan);
	
	if (fc == -1 || sc == -1)
	{
		SET_RET(EINVAL, NULL);
	}
	
	wator->nf = fc;
	wator->ns = sc;
	
	return wator;
}

void free_wator(wator_t * pw)
{
	if (pw == NULL)
	{
		return;
	}
	printf("free_planet-start\n");
	free_planet(pw->plan);
	printf("free_planet-end\n");
	pw->plan = NULL;
	free(pw);
}

int update_wator(wator_t * pw)
{
	if (validate_wator(pw) == -1)
	{
		SET_RET(EINVAL, -1);
	}
	
	/* Matrice di appoggio per evitare di aggiornare i pesci/squali appena nati */
	int ** dirty_cells_mat = new_int_mat(pw->plan->nrow, pw->plan->ncol);
	if (dirty_cells_mat == NULL)
	{
		SET_RET(ENOMEM, -1);
	}
	
	/* Prima gli squali */
	int u, v;
	for (int i = 0; i < pw->plan->nrow; i++)
	{
		for (int j = 0; j < pw->plan->ncol; j++)
		{
			if (pw->plan->w[i][j] == SHARK && dirty_cells_mat[i][j] == 0)
			{
				if (shark_rule1(pw, i, j, &u, &v) == -1)
				{
					free_int_mat(dirty_cells_mat, pw->plan->nrow);
					SET_RET(EINVAL, -1);
				}
				dirty_cells_mat[u][v] = 1;
				
				if (shark_rule2(pw, u, v, &u, &v) == -1)
				{
					free_int_mat(dirty_cells_mat, pw->plan->nrow);
					SET_RET(EINVAL, -1);
				}
				dirty_cells_mat[u][v] = 1;	
			}
		}
	}
	/* Poi i pesci */
	int w, z;
	for (int i = 0; i < pw->plan->nrow; i++)
	{
		for (int j = 0; j < pw->plan->ncol; j++)
		{
			if (pw->plan->w[i][j] == FISH && dirty_cells_mat[i][j] == 0)
			{
				if (fish_rule4(pw, i, j, &w, &z) == -1)
				{
					free_int_mat(dirty_cells_mat, pw->plan->nrow);
					SET_RET(EINVAL, -1);
				}
				dirty_cells_mat[w][z] = 1;
				
				if (fish_rule3(pw, i, j, &w, &z) == -1)
				{
					free_int_mat(dirty_cells_mat, pw->plan->nrow);
					SET_RET(EINVAL, -1);
				}
				dirty_cells_mat[w][z] = 1;
			}
		}
	}
	
	free_int_mat(dirty_cells_mat, pw->plan->nrow);
	
	return 0;
}
