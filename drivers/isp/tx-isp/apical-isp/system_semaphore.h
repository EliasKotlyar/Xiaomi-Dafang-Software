#ifndef __SEMAPHORE_H_INCLUDED
#define __SEMAPHORE_H_INCLUDED
//-----------------------------------------------------------------------------
#include <apical-isp/apical_types.h>

struct semaphore;

typedef struct _sem_t {
	struct semaphore *psem;
} sem_t;

//-----------------------------------------------------------------------------
void init_semaphore(sem_t *sem);
void raise_semaphore(sem_t *sem);
void wait_semaphore(sem_t *sem, uint32_t timeout_ms);
void destroy_semaphore(sem_t *sem);
//-----------------------------------------------------------------------------
#endif //__SEMAPHORE_H_INCLUDED
