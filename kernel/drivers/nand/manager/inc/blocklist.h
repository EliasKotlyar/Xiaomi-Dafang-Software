#ifndef __BLOCKLIST_H__
#define __BLOCKLIST_H__

#include "singlelist.h"

typedef struct _BlockList BlockList;
struct _BlockList {
	struct singlelist head;//must be the first member of the struct
	int startBlock;
	int _startBlock;
	int BlockCount;
	int retVal;	
};
#endif
