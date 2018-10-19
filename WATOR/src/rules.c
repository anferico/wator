/** 
	\file       rules.c
    \author     Francesco Cariaggi
    \matricola  503146
    
  Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  originale dell' autore. 
 */

#include "wator.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

int shark_rule1 (wator_t * pw, int x, int y, int * k, int * l)
{
/* Alcune situazioni anomale che voglio evitare */
/*----------------------------------*/
	if (validate_wator(pw) == -1)
	{
		SET_RET(EINVAL, -1);
	}
	if (x < 0 || y < 0 || x > pw->plan->nrow || y > pw->plan->ncol)
	{
		SET_RET(EINVAL, -1);
	}
	if (pw->plan->w[x][y] != SHARK)
	{
		SET_RET(EINVAL, -1);
	}
/*----------------------------------*/
	
	/* Controllo i 4 vicini, riempiendo il vettore 'around' */
	cell_t * around[4];
	if (nearby(pw, x, y, around) == -1)
	{
		/* Errore nell'analisi dei 4 vicini (es. alcuni non validi) */
		SET_RET(EINVAL, -1);
	}
	
	srand(time(NULL));
	int sharkCount = 0;
	
	for (int i = 0, index = rand() % 4; i < 4; i++, index = (index + 1) % 4)
	{
		if (*(around[index]) == FISH)
		{
			/* Setto preventivamente 'k' e 'l' */
			switch (index)
			{
				case 0: /* Ho trovato un pesce sopra */
					up(pw, x, y, k, l);
				break;
				case 1: /* Ho trovato un pesce sotto */
					down(pw, x, y, k, l);
				break;
				case 2: /* Ho trovato un pesce a sinistra */
					left(pw, x, y, k, l);
				break;
				case 3: /* Ho trovato un pesce a destra */
					right(pw, x, y, k, l);
				break;
			}
			
			/* Muovo lo squalo, mangiando il pesce */
			pw->plan->w[*k][*l] = SHARK;
			pw->plan->w[x][y] = WATER;
			/* Aggiorno il numero dei pesci */
			pw->nf = pw->nf - 1;
			/* Aggiorno anche le altre matrici (btime e dtime) */
			pw->plan->btime[*k][*l] = pw->plan->btime[x][y];
			pw->plan->dtime[*k][*l] = pw->plan->dtime[x][y];
			pw->plan->btime[x][y] = 0;
			pw->plan->dtime[x][y] = 0;
			
			return EAT;
		}
		if (*(around[index]) == SHARK)
		{
			sharkCount++;
		}
		if (*(around[index]) == WATER)
		{
			/* Setto 'k' e 'l': fino a prova contraria, mi muoverò lì */
			switch (index)
			{
				case 0: /* Ho trovato acqua sopra */
					up(pw, x, y, k, l);
				break;
				case 1: /* Ho trovato acqua sotto */
					down(pw, x, y, k, l);
				break;
				case 2: /* Ho trovato acqua a sinistra */
					left(pw, x, y, k, l);
				break;
				case 3: /* Ho trovato acqua a destra */
					right(pw, x, y, k, l);
				break;
			}
		}
	}
	
	if (sharkCount == 4)
	{
		*k = x;
		*l = y;
		return STOP;
	}
	else /* Se arrivo fin qui devo semplicemente muovere lo squalo */
	{
		/* Muovo lo squalo */
		pw->plan->w[*k][*l] = SHARK;
		pw->plan->w[x][y] = WATER;
		/* Aggiorno anche le altre matrici (btime e dtime) */
		pw->plan->btime[*k][*l] = pw->plan->btime[x][y];
		pw->plan->dtime[*k][*l] = pw->plan->dtime[x][y];
		pw->plan->btime[x][y] = 0;
		pw->plan->dtime[x][y] = 0;
		
		return MOVE;
	}
}

int shark_rule2 (wator_t * pw, int x, int y, int * k, int * l)
{
/* Alcune situazioni anomale che voglio evitare */
/*----------------------------------*/
	if (validate_wator(pw) == -1)
	{
		SET_RET(EINVAL, -1);
	}
	if (x < 0 || y < 0 || x > pw->plan->nrow || y > pw->plan->ncol)
	{
		SET_RET(EINVAL, -1);
	}
	if (pw->plan->w[x][y] != SHARK)
	{
		SET_RET(EINVAL, -1);
	}
/*----------------------------------*/

	if (pw->plan->dtime[x][y] == pw->sd)
	{
		/* Lo squalo deve morire */
		pw->plan->w[x][y] = WATER;
		pw->plan->btime[x][y] = 0;
		pw->plan->dtime[x][y] = 0;
		/* Aggiorno il numero degli squali */
		pw->ns = pw->ns - 1;
		return DEAD;
	}
	/* Lo squalo deperisce un po' */
	pw->plan->dtime[x][y]++;
	
	if (pw->plan->btime[x][y] < pw->sb)
	{
		/* Non è ancora tempo di figliare, lo squalo matura */
		pw->plan->btime[x][y]++;
		return ALIVE;
	}
	
/* --- btime(x,y) è uguale a pw->sb (è tempo di figliare) --- */
	
	/* Controllo i 4 vicini, riempiendo il vettore 'around' */
	cell_t * around[4];
	if (nearby(pw, x, y, around) == -1)
	{
		/* Errore nell'analisi dei 4 vicini (es. alcuni non validi) */
		SET_RET(EINVAL, -1);
	}
	
	srand(time(NULL));
	
	/* In ogni caso */
	pw->plan->btime[x][y] = 0;
	
	for (int i = 0, index = rand() % 4; i < 4; i++, index = (index + 1) % 4)
	{
		if (*(around[index]) == WATER)
		{
			/* Setto preventivamente 'k' e 'l' per lo squalo figlio */
			switch (index)
			{
				case 0: /* Ho trovato acqua sopra */
					up(pw, x, y, k, l);
				break;
				case 1: /* Ho trovato acqua sotto */
					down(pw, x, y, k, l);
				break;
				case 2: /* Ho trovato acqua a sinistra */
					left(pw, x, y, k, l);
				break;
				case 3: /* Ho trovato acqua a destra */
					right(pw, x, y, k, l);
				break;
			}
			
			/* Deposito il nuovo squalo */
			pw->plan->w[*k][*l] = SHARK;
			/* Aggiorno il numero degli squali */
			pw->ns = pw->ns + 1;
			
			return ALIVE;
		}
	}
	/* Se arrivo fin qui significa che non ho spazio per depositare un figlio */
	return ALIVE;
}

int fish_rule3 (wator_t * pw, int x, int y, int * k, int * l)
{
/* Alcune situazioni anomale che voglio evitare */
/*----------------------------------*/
	if (validate_wator(pw) == -1)
	{
		SET_RET(EINVAL, -1);
	}
	if (x < 0 || y < 0 || x > pw->plan->nrow || y > pw->plan->ncol)
	{
		SET_RET(EINVAL, -1);
	}
	if (pw->plan->w[x][y] != FISH)
	{
		SET_RET(EINVAL, -1);
	}
/*----------------------------------*/

	/* Controllo i 4 vicini, riempiendo il vettore 'around' */
	cell_t * around[4];
	if (nearby(pw, x, y, around) == -1)
	{
		/* Errore nell'analisi dei 4 vicini (es. alcuni non validi) */
		SET_RET(EINVAL, -1);
	}
	
	srand(time(NULL));
	
	for (int i = 0, index = rand() % 4; i < 4; i++, index = (index + 1) % 4)
	{
		if (*(around[index]) == WATER)
		{
			/* Setto preventivamente 'k' e 'l' */
			switch (index)
			{
				case 0: /* Ho trovato acqua sopra */
					up(pw, x, y, k, l);
				break;
				case 1: /* Ho trovato acqua sotto */
					down(pw, x, y, k, l);
				break;
				case 2: /* Ho trovato acqua a sinistra */
					left(pw, x, y, k, l);
				break;
				case 3: /* Ho trovato acqua a destra */
					right(pw, x, y, k, l);
				break;
			}
			
			/* Muovo il pesce */
			pw->plan->w[x][y] = WATER;
			pw->plan->w[*k][*l] = FISH;
			/* Aggiorno anche la matrice btime */
			pw->plan->btime[*k][*l] = pw->plan->btime[x][y];
			pw->plan->btime[x][y] = 0;
			
			return MOVE;
		}
	}
	/* Se arrivo fin qui significa che non ho spazio per muovermi */
	return STOP;
}

int fish_rule4 (wator_t * pw, int x, int y, int * k, int * l)
{
/* Alcune situazioni anomale che voglio evitare */
/*----------------------------------*/
	if (validate_wator(pw) == -1)
	{
		SET_RET(EINVAL, -1);
	}
	if (x < 0 || y < 0 || x > pw->plan->nrow || y > pw->plan->ncol)
	{
		SET_RET(EINVAL, -1);
	}
	if (pw->plan->w[x][y] != FISH)
	{
		SET_RET(EINVAL, -1);
	}
/*----------------------------------*/
	
	if (pw->plan->btime[x][y] < pw->fb)
	{
		/* Non è ancora tempo di figliare, il pesce matura */
		pw->plan->btime[x][y]++;
		return 0;
	}
	
/* --- btime(x,y) è uguale a pw->fb (è tempo di figliare) --- */
	
	/* Controllo i 4 vicini, riempiendo il vettore 'around' */
	cell_t * around[4];
	if (nearby(pw, x, y, around) == -1)
	{
		/* Errore nell'analisi dei 4 vicini (es. alcuni non validi) */
		SET_RET(EINVAL, -1);
	}
	
	srand(time(NULL));
	
	/* In ogni caso */
	pw->plan->btime[x][y] = 0;
	
	for (int i = 0, index = rand() % 4; i < 4; i++, index = (index + 1) % 4)
	{
		if (*(around[index]) == WATER)
		{
			/* Setto preventivamente 'k' e 'l' per il pesce figlio */
			switch (index)
			{
				case 0: /* Ho trovato acqua sopra */
					up(pw, x, y, k, l);
				break;
				case 1: /* Ho trovato acqua sotto */
					down(pw, x, y, k, l);
				break;
				case 2: /* Ho trovato acqua a sinistra */
					left(pw, x, y, k, l);
				break;
				case 3: /* Ho trovato acqua a destra */
					right(pw, x, y, k, l);
				break;
			}
			
			/* Deposito il nuovo pesce */
			pw->plan->w[*k][*l] = FISH;
			/* Aggiorno il numero dei pesci */
			pw->nf = pw->nf + 1;
			
			return 0;
		}
	}
	/* Se arrivo fin qui significa che non ho spazio per depositare un figlio */
	return 0;
}
