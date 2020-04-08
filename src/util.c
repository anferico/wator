/** 
	\file       util.c
    \author     Francesco Cariaggi
    \matricola  503146
    
  Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  originale dell' autore. 
 */

#include "wator.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

int mod(int x, int y)
{	
	if (x >= 0)
		return x % y;
	else
	{
		while (x < 0)
			x += y;
		return x;
	}
}

char * substring2(char * str, char dest[3])
{
	if (strlen(str) < 2)
		return NULL;
		
	dest[0] = str[0];
	dest[1] = str[1];
	dest[2] = '\0';
	
	return dest;
}

int ** new_int_mat(int rows, int cols)
{
	int ** mat = (int**) malloc(rows * sizeof(int*));
	if (mat == NULL)
	{
		errno = ENOMEM;
		return NULL;
	}
	for (int i = 0; i < rows; i++)
	{
		mat[i] = (int*) calloc(cols, sizeof(int));
		if (mat[i] == NULL)
		{
			free_int_mat(mat, rows);
			errno = ENOMEM;
			return NULL;
		}
	}
	return mat;
}

void free_int_mat(int ** mat, unsigned int rows)
{
	if (mat != NULL)
	{
		for (int i = 0; i < rows; i++)
		{
			free(mat[i]);
			mat[i] = NULL;
		}
		free(mat);
	}
}

int configure(wator_t * wator, char * confFilePath)
{
	FILE * confFile;
	
	if ((confFile = fopen(confFilePath, "r")) == NULL)
	{
		/* fclose(confFile); */
		/* errno settata da fopen */
		return -1;
	}
	
	char buf[20];
	char * err;
	char subs[3];
	int param;
	
	for (int i = 0; i < 3; i++)
	{
		/* leggo una riga */
		if (fgets(buf, 20, confFile) == NULL)
		{
			fclose(confFile);
			SET_RET(EBADF, -1);
		}
		
		/* estraggo i primi due caratteri (+ \0) e li metto in 'subs' */
		if (substring2(buf, subs) == NULL)
		{
			fclose(confFile);
			SET_RET(ERANGE, -1);
		}
		
		/* Cerco di convertire in intero la stringa buf (eccettuati i primi 2 caratteri)*/
		param = strtol(buf + 2, &err, 10);
		if (*err != '\0' && *err != '\n')
		{
			fclose(confFile);
			SET_RET(ERANGE, -1);
		}
		
		/* Analizzo i primi 2 caratteri della stringa buf, quindi agisco opportunamente*/
		if (strncmp(subs, "sd", 2) == 0)
		{
			wator->sd = param;
		}
		else if (strncmp(subs, "sb", 2) == 0)
		{
			wator->sb = param;
		}
		else if (strncmp(subs, "fb", 2) == 0)
		{
			wator->fb = param;
		}
		else
		{
			fclose(confFile);
			SET_RET(ERANGE, -1);
		}
	}
	
	/* Non tollero ulteriore contenuto nel file */
	if (fgets(buf, 20, confFile) != NULL)
	{
		fclose(confFile);
		SET_RET(EBADF, -1);
	}
	
	fclose(confFile);
	return 0;
}

cell_t * up(wator_t * w, int x, int y, int * u, int * v)
{
	/* Coordinate di sopra(x,y) */
	int upX = mod(x-1, (w->plan->nrow));
	int upY = y;

	if (u != NULL)
		*u = upX;
	if (v != NULL)
		*v = upY;
	
	return &(w->plan->w[upX][upY]);
}

cell_t * down(wator_t * w, int x, int y, int * u, int * v)
{
	/* Coordinate di sotto(x,y) */
	int downX = mod(x+1, (w->plan->nrow));
	int downY = y;

	if (u != NULL)
		*u = downX;
	if (v != NULL)
		*v = downY;
	
	return &(w->plan->w[downX][downY]);
}

cell_t * left(wator_t * w, int x, int y, int * u, int * v)
{
	/* Coordinate di sinistra(x,y) */
	int leftX = x;
	int leftY = mod(y-1, (w->plan->ncol));

	if (u != NULL)
		*u = leftX;
	if (v != NULL)
		*v = leftY;
	
	return &(w->plan->w[leftX][leftY]);
}

cell_t * right(wator_t * w, int x, int y, int * u, int * v)
{
	/* Coordinate di destra(x,y) */
	int rightX = x;
	int rightY = mod(y+1, (w->plan->ncol));

	if (u != NULL)
		*u = rightX;
	if (v != NULL)
		*v = rightY;
	
	return &(w->plan->w[rightX][rightY]);
}

int nearby(wator_t * wator, int x, int y, cell_t * slots[4])
{	
	cell_t * upPtr = up(wator, x, y, NULL, NULL);
	cell_t * downPtr = down(wator, x, y, NULL, NULL);
	cell_t * leftPtr = left(wator, x, y, NULL, NULL);
	cell_t * rightPtr = right(wator, x, y, NULL, NULL);
	
	if (cell_to_char(*upPtr) == '?' || 
	    cell_to_char(*downPtr) == '?' || 
	    cell_to_char(*leftPtr) == '?' || 
	    cell_to_char(*rightPtr) == '?')
	{
		return -1;
	}
	
	slots[0] = upPtr;
	slots[1] = downPtr;
	slots[2] = leftPtr;
	slots[3] = rightPtr;
	
	return 0;
}

int validate_planet(planet_t * p)
{
	if (p == NULL)
	{
		return -1;
	}
	if (p->nrow <= 0 || p->ncol <= 0)
	{
		return -1;
	}
	if (p->w == NULL || p->btime == NULL || p->dtime == NULL)
	{
		return -1;
	}
	for (int i = 0; i < p->nrow; i++)
	{
		if (p->w[i] == NULL || p->btime[i] == NULL || p->dtime[i] == NULL)
		{
			return -1;
		}
		for (int j = 0; j < p->ncol; j++)
		{
			if (cell_to_char(p->w[i][j]) == '?'
			    || p->btime[i][j] < 0
			    || p->dtime[i][j] < 0)
			{
				return -1;
			}
		}
	}
	return 0;
}

int validate_wator(wator_t * w)
{
	if (w == NULL)
	{
		return -1;
	}
	if (validate_planet(w->plan) == -1)
	{
		return -1;
	}
	if (w->ns != shark_count(w->plan) || w->nf != fish_count(w->plan))
	{
		return -1;
	}
	
	return 0;
}

int file_exists_regular(char * filename)
{
	struct stat st;
	if (stat(filename, &st) == -1 && errno != ENOENT)
	{
		perror("stat");
		exit(errno);
	}
	return S_ISREG(st.st_mode);
}

char * strdup (const char * s) /* (Implementazione di glibc) */
{
  size_t len = strlen (s) + 1;
  void * new = malloc (len);
 
  if (new == NULL)
    return NULL;
  
  return (char *) memcpy (new, s, len);
}

int shark_rule_1w(wator_t * pw, int x, int y, int * k, int * l)
{
	if (x < 0 || y < 0 || x > pw->plan->nrow || y > pw->plan->ncol)
	{
		SET_RET(EINVAL, -1);
	}
	if (pw->plan->w[x][y] != SHARK)
	{
		SET_RET(EINVAL, -1);
	}
	
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

int shark_rule_2w(wator_t * pw, int x, int y, int * k, int * l)
{
	if (x < 0 || y < 0 || x > pw->plan->nrow || y > pw->plan->ncol)
	{
		SET_RET(EINVAL, -1);
	}
	if (pw->plan->w[x][y] != SHARK)
	{
		SET_RET(EINVAL, -1);
	}
	
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
	
	/* ===== btime(x,y) è uguale a pw->sb (è tempo di figliare) ===== */
	
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

int fish_rule_3w(wator_t * pw, int x, int y, int * k, int * l)
{
	if (x < 0 || y < 0 || x > pw->plan->nrow || y > pw->plan->ncol)
	{
		SET_RET(EINVAL, -1);
	}
	if (pw->plan->w[x][y] != FISH)
	{
		SET_RET(EINVAL, -1);
	}

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

int fish_rule_4w(wator_t * pw, int x, int y, int * k, int * l)
{
	if (x < 0 || y < 0 || x > pw->plan->nrow || y > pw->plan->ncol)
	{
		SET_RET(EINVAL, -1);
	}
	if (pw->plan->w[x][y] != FISH)
	{
		SET_RET(EINVAL, -1);
	}
	
	if (pw->plan->btime[x][y] < pw->fb)
	{
		/* Non è ancora tempo di figliare, il pesce matura */
		pw->plan->btime[x][y]++;
		return 0;
	}
	
	/* ===== btime(x,y) è uguale a pw->fb (è tempo di figliare) ===== */
	
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

