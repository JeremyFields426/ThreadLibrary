#include "thread.h"

void testFunc(int id);

int main()
{
	t_init();

	t_create(testFunc, 1, 1);
	t_create(testFunc, 2, 1);
	t_create(testFunc, 3, 1);
	
	for (int i = 0; i < 4; i++)
	{
		printf("This is main [%d]...\n", i);
		t_yield();
	}

	printf("Begin shutdown...\n");
	t_shutdown();
	printf("Done with shutdown...\n");

	return 0;
}

void testFunc(int id)
{
	printf("Thread %d has begun...\n", id);

	for (int i = 0; i < 3; i++)
	{		
		printf("This is thread %d [%d]...\n", id, i);
		t_yield();
	}
	
	printf("Thread %d has ended...\n", id);
	t_terminate();
}
