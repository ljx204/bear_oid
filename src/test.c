#include "oid_msg.h"
#include "Thread.h"
#include <stdio.h>
#include <stdlib.h>

#define APP_Q_SIZE	10
#define APP_Q_MSGLEN	3*(sizeof(INT32U))
PMSG_ID_Q   test_id;
oid_mutex_t  print_t;

void k_printf(char * str)
{
	oid_mutex_lock(&print_t);
	printf("%s \r\n", str);
	oid_mutex_unlock(&print_t);

}

static void * Thread(void *data)
{
	int msg_number = 0;
	int msg_kind = 0;
	while(1) {
		msg_number++;
		msg_kind++;
	//	printf( "msg_kind = %d \r\n", msg_kind);
		msgQSend(test_id, (INT32U)msg_kind, &msg_number, sizeof(msg_number), MSG_PRI_NORMAL);
	//	msleep(1);
		
		msg_kind = random();
		msg_kind %= 5;
		
	}

}

int main(char argc, char * argv[])
{
	INT32S  nRet, n;
	oid_thread_t test, test1;
	INT32U  msg_id,wParam[APP_Q_MSGLEN/sizeof(INT32U)];
	test_id = msgQCreate(APP_Q_SIZE, APP_Q_SIZE, APP_Q_MSGLEN);
//	oid_mutex_t print_t;
	oid_mutex_init(&print_t);
//        oid_clone(&test, Thread , NULL, OID_THREAD_PRIORITY_INPUT);
//	oid_clone(&test1, Thread , NULL, OID_THREAD_PRIORITY_INPUT);
//	while(1);
	n = 1;
	msgQSend(test_id, (INT32U)n, &n, sizeof(INT32U),  MSG_PRI_NORMAL);
	while(1) {

		nRet = msgQReceive(test_id, &msg_id, &wParam, sizeof(wParam));
		printf("msg id = 0x%x, %x  \r\n", msg_id, wParam[0]);
		if(msg_id != wParam[0]) {
			while(1);
		}
		n++;
		switch(msg_id) 
		{	
		//	case 0:
	//		    printf("0: get msg %d \r\n", wParam[0]);
		//	break;
		//	case 1:
				
	//		    printf("1:get msg begin \r\n");
			    default:
			    msgQSend(test_id, (INT32U)n, &n, sizeof(INT32U),  MSG_PRI_NORMAL);
	//		    printf("1:get msg end \r\n");
				msleep(10000);
				break;
		//	default:
	//		   fprintf(stderr,"can't open it! \r\n");
			  // k_printf("unknown \r\n");
	//		n++;
		//	break;
		

		}


	}
	
}
