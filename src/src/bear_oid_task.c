#include "oid_common.h"

void OID_Task_Entry(void * param)
{
	int fd;
	unsigned int oid_number;

	while(1)
	{
		fd = open("/dev/sn9p702_oid", O_RDONLY);

		if(fd < 0) {
			printf("Error ...... \r\n");
			msleep(1000);
			continue;
			
		}
		else break;
	}

	while(1)
	{
		if(read(fd, &oid_number, sizeof(oid_number)))
		{
			printf("ooooo: 0x%x \r\n", oid_number);
			oid_send_number(&oid_number);
		}

	}


}
