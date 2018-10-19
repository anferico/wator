/** 
	\file       cells.c
    \author     Francesco Cariaggi
    \matricola  503146
    
  Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  originale dell' autore. 
 */

#include "wator.h"

char cell_to_char(cell_t a)
{
	if (a == WATER)
	{
		return 'W';
	}
	else if (a == SHARK)
	{
		return 'S';
	}
	else if (a == FISH)
	{
		return 'F';
	}
	else
	{
		return '?';
	}
}

int char_to_cell (char c)
{
	if (c == 'W')
	{
		return WATER;
	}
	else if (c == 'S')
	{
		return SHARK;
	}
	else if (c == 'F')
	{
		return FISH;
	}
	else
	{
		return -1;
	}
}
