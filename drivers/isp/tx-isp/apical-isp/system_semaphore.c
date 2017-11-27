#include "system_semaphore.h"
#include "linux/semaphore.h"
#include "linux/slab.h"

void init_semaphore(sem_t *sem)
{
	sem->psem = kmalloc(sizeof(struct semaphore), GFP_KERNEL | __GFP_NOFAIL);
	sema_init(sem->psem, 1);
}

void raise_semaphore(sem_t *sem)
{
	up(sem->psem);
}

void wait_semaphore(sem_t *sem, uint32_t timeout_ms)
{
	uint32_t ignore_ret;
	ignore_ret = down_timeout(sem->psem, msecs_to_jiffies(timeout_ms));
}

void destroy_semaphore(sem_t *sem)
{
	kfree(sem->psem);
	sem->psem = NULL;
}
