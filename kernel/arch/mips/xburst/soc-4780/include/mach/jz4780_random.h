#ifndef __JZ4780_RANDOM_H__
#define __JZ4780_RANDOM_H__

#define CPM_RANDEN	(0xd8)
#define CPM_RANDNUM	(0xdc)

unsigned int generate_random(unsigned int *random_num, int num);
unsigned int generate_one_random(void);
#endif
