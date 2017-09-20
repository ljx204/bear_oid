#ifndef __BEAR_SOCKET_H__
#define __BEAR_SOCKET_H__
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h> 
#include "oid_common.h"
#define UNIX_DOMAIN "/tmp/UNIX.domain"

typedef struct oid_cmd {
	int  command;
	union {
		int oid_number;
		char data[4];
	      }cmd_msg;

}OID_CMD;

void * bear_socket_task(void * param);

typedef enum {
	SOCKET_MSG_OID_MIN = 0,
	SOCKET_MSG_OID_NUMBER,
	SOCKET_MSG_OID_EXIT,
	SOCKET_MSG_OID_MAX

}SOCK_OID_MSG;

#endif
