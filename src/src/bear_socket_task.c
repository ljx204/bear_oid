#include "bear_socket_task.h"

typedef struct bear_socket{
	PMSG_ID_Q    	p_send_socket_event;
	int          	socket_state;   //0 no connet 1 connect
	oid_mutex_t     socket_mutex;

}BEAR_SOCKET;

static  BEAR_SOCKET    p_bear_socket;


void bear_send_data_to_socket(void * param)
{
	if(p_bear_socket.socket_state)
	{
		msgQSend(p_bear_socket.p_send_socket_event, SOCKET_MSG_OID_NUMBER, param, sizeof(param), MSG_PRI_NORMAL);
	}
}

void bear_end_send_data_to_socket(void)
{
	msgQSend(p_bear_socket.p_send_socket_event, SOCKET_MSG_OID_EXIT, NULL, 0, MSG_PRI_NORMAL);
}
//void bear_send_data_to_socket(

static void * bear_receive_socket(void * param);
static void * bear_send_socket(void * param);

void * bear_socket_task(void * param)
{
	socklen_t clt_addr_len;
    	int listen_fd;
    	int com_fd;
    	int ret;
    	int i;
  
    	int len;
    	//OID_CMD  oid_data;
    	//OID_CMD  recv_data;
	
	oid_thread_t sendPthread; 
	oid_thread_t recvPthread;

	struct sockaddr_un clt_addr;
    	struct sockaddr_un srv_addr;
    	listen_fd=socket(PF_UNIX,SOCK_STREAM,0);
    	if(listen_fd<0)
    	{
        	perror("cannot create communication socket");
        	return NULL;
	}  
	    
	//set server addr_param
	srv_addr.sun_family=AF_UNIX;
	strncpy(srv_addr.sun_path,UNIX_DOMAIN,sizeof(srv_addr.sun_path)-1);
	unlink(UNIX_DOMAIN);
	    //bind sockfd & addr
	ret=bind(listen_fd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));
	if(ret==-1)
	{
		perror("cannot bind server socket");
		close(listen_fd);
		unlink(UNIX_DOMAIN);
		return NULL;
	}
        //listen sockfd 
       ret=listen(listen_fd,1);
       if(ret==-1)
       {
        	perror("cannot listen the client connect request");
        	close(listen_fd);
        	unlink(UNIX_DOMAIN);
        	return NULL;
    	}

	oid_mutex_init(&p_bear_socket.socket_mutex);
	p_bear_socket.socket_state = 0;
	p_bear_socket.p_send_socket_event = msgQCreate(SOCKET_Q_SIZE, SOCKET_Q_SIZE, SOCKET_Q_MSGLEN);

	while(1)
	{
		
		len=sizeof(clt_addr);
    		com_fd=accept(listen_fd,(struct sockaddr*)&clt_addr,&len);
    		if(com_fd<0)
    		{
        		perror("cannot accept client connect request");
        		close(listen_fd);
        		unlink(UNIX_DOMAIN);
        		return 1;
    		}
		oid_mutex_lock(&p_bear_socket.socket_mutex);
		p_bear_socket.socket_state = 1;
		oid_mutex_unlock(&p_bear_socket.socket_mutex);
		
		printf("new client connect \r\n");
		oid_clone(&sendPthread,  bear_receive_socket , &com_fd , OID_THREAD_PRIORITY_INPUT + 1);
		oid_clone(&recvPthread,  bear_send_socket , &com_fd , OID_THREAD_PRIORITY_OUTPUT - 1);

		oid_join(sendPthread, NULL );
		oid_join(recvPthread, NULL );

		oid_mutex_lock(&p_bear_socket.socket_mutex);
		p_bear_socket.socket_state = 0;
		oid_mutex_unlock(&p_bear_socket.socket_mutex);

		//pthread_create(&sendPthread, NULL, sendMsg, &com_fd);  
		//pthread_create(&recvPthread, NULL, recvMsg, &com_fd); 
		//pthread_join(sendPthread, 0);  
		//printf("11111 \r\n");
		//pthread_join(recvPthread, 0);  
		printf("client disconnect \r\n");
	


	}




}


static void * bear_receive_socket(void * param)
{
	int com_fd = *(int *)param;
	OID_CMD recv_data;
	int num;
	while(1)
	{
		num=read(com_fd,&recv_data,sizeof(OID_CMD));
		if(num < 0)
		{
			printf("recvMsg num = %d \r\n", num);
			if(errno==EINTR) 
  				num=0; 
			else 
  				break;
		}
		else if(num == 0)
		{
			close(com_fd);
			break;
		}
	}
	//close(com_fd);
	bear_end_send_data_to_socket();
	printf("recvMsg.... \r\n");
	pthread_exit(NULL);

}


static void * bear_send_socket(void * param)
{
	int com_fd = *(int *)param;
	OID_CMD p_oid_data;
	int num;
	INT32S  nRet;
	INT32U  msg_id; 
	INT32U  wParam[SOCKET_Q_MSGLEN/sizeof(INT32U)];
	INT32U  loop_flag = 1;
	
	while(loop_flag)
	{
		nRet = msgQReceive(p_bear_socket.p_send_socket_event, &msg_id, &wParam, sizeof(wParam));

		switch(msg_id)
		{
			case SOCKET_MSG_OID_NUMBER:
			{
				int oid_number = wParam[0];

				p_oid_data.command = 0x0001;
				p_oid_data.cmd_msg.oid_number = oid_number;

				num = write(com_fd, &p_oid_data, sizeof(OID_CMD)); 


				if(num<=0) /* 出错了*/ 
				{       
					printf("sendMsg num = %d \r\n", num); 
					if(errno == EINTR) /* 中断错误 我们继续写*/ 
				 		num = 0; 
					else             /* 其他错误 没有办法,只好撤退了*/ 
					{
						loop_flag = 0;
				 		break; 
					}
				} 
				break;
			}
			case SOCKET_MSG_OID_EXIT:
			{
				loop_flag = 0;
				break;
			}
			default:
				break;

		}
		//sleep(1);
	}
	printf("sendMsg.... \r\n");
	pthread_exit(NULL);


}

