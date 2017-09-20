//c_unix.c
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#define UNIX_DOMAIN "/tmp/UNIX.domain"

typedef struct oid_cmd {
	int  command;
	union {
		int oid_number;
		char data[4];
	      }cmd_msg;

}OID_CMD;

int main(void)
{
    int connect_fd;
    int ret;
    char snd_buf[1024];
    int i;
    static struct sockaddr_un srv_addr;
	OID_CMD recv_cmd, send_cmd;
	int n = 0;
//creat unix socket
    connect_fd=socket(PF_UNIX,SOCK_STREAM,0);
    if(connect_fd<0)
    {
        perror("cannot create communication socket");
        return 1;
    }   
    srv_addr.sun_family=AF_UNIX;
    strcpy(srv_addr.sun_path,UNIX_DOMAIN);
//connect server
    ret=connect(connect_fd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));
    if(ret==-1)
    {
        perror("cannot connect to the server");
        close(connect_fd);
        return 1;
    }
	
	while(1) {
			int num=read(connect_fd,&recv_cmd,sizeof(OID_CMD));
			if(num < 0)
			{
				if(errno==EINTR) 
          				num=0; 
       				else 
          				break;
			}
			else
			{
				printf("app_user recv data %x , %x \r\n", recv_cmd.command, recv_cmd.cmd_msg.oid_number);
			}
		#if 0	
			send_cmd.command = 0x0001;
			send_cmd.cmd_msg.oid_number = 0x2b59;
			num = write(connect_fd, &send_cmd, sizeof(OID_CMD)); 

         		if(num<=0) /* 出错了*/ 
         		{        
                 		if(errno==EINTR) /* 中断错误 我们继续写*/ 
                         		num = 0; 
                 		else             /* 其他错误 没有办法,只好撤退了*/ 
                         		break; 
         		} 
			n++;
			if(n >= 10) break;
		#endif
	}
#if 0
    memset(snd_buf,0,1024);
    strcpy(snd_buf,"message from client");
//send info server
    while(1)
    {
	i = getchar();
	if(i != '\n') {
	sprintf(snd_buf, "message ken %x \r\n", i);
        write(connect_fd,snd_buf,sizeof(snd_buf));
	}
	//sleep(2);
    }
#endif
    close(connect_fd);
    return 0;
}

