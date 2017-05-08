#ifndef __LOCKFREESTACK_TEST__H__
#define __LOCKFREESTACK_TEST__H__

struct st_TEST_DATA
{
	volatile LONG64	lData;
	volatile LONG64	lCount;
};

#define dfTHREAD_ALLOC 10000
#define dfTHREAD_MAX 10

#endif