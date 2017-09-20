#include "oid_common.h"

static void * Thread(void *data)
{
	while(1) {

		fprintf(stderr, "1234 \r\n");
		msleep(1000000);
	}

}


int main(char argc, char * argv[])
{
	oid_thread_t test;
	oid_clone(&test, Thread , NULL, OID_THREAD_PRIORITY_INPUT);
	while(1);

}
