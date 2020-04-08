/** 
	\file       jobsrepo.c
    \author     Francesco Cariaggi
    \matricola  503146
    
  Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  originale dell' autore. 
 */

#include "jobsrepo.h"
#include <errno.h>
#include <stdlib.h>

int putone(jobsrepo_t ** repo, job_t value)
{
	if (repo == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	
	jobsrepo_t * newNode = (jobsrepo_t*) malloc(sizeof(jobsrepo_t));
	
	if (newNode == NULL)
	{
		errno = ENOMEM;
		return -1;
	}
	
	newNode->job = value;
	newNode->next = NULL;
	if (*repo == NULL)
	{
		*repo = newNode;
		return 0;
	}
	
	jobsrepo_t * ptr = *repo;
	while (ptr->next != NULL)
	{
		ptr = ptr->next;
	}
	
	ptr->next = newNode;
	return 0;
}

int pickone(jobsrepo_t ** repo, job_t * jb)
{
	if (repo == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	if (*repo == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	if (jb != NULL)
	{
		*jb = (*repo)->job;
	}
	
	jobsrepo_t * garbage = *repo;
	*repo = (*repo)->next;
	
	free(garbage);
	
	return 0;
}

void free_repository(jobsrepo_t ** repo)
{
	if (*repo == NULL)
	{
		return;
	}
	while (*repo != NULL)
	{
		pickone(repo, NULL);
	}
}

