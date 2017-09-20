#ifndef _OID_MSG_H__
#define _OID_MSG_H__
#include"msg_manage.h"
#include<string.h>
#define MSG_PRI_NORMAL		0
#define MSG_PRI_URGENT		1

typedef struct 
{
//	oid_mutex_t	msg_mutex;
	INT32U		maxMsgs;      /* max messages that can be queued */
	INT32U		maxMsgLength;	/* max bytes in a message */
	OS_EVENT*	pEvent;
	void*		pMsgQ;
	INT8U*		pMsgPool;
	INT32U		msg_index;
	oid_mutex_t     msg_mutex;
} MSG_ID_Q,*PMSG_ID_Q;

//========================================================
//Function Name:msgQCreate
//Syntax:		PMSG_ID_Q msgQCreate(INT32U maxQSize, INT32U maxMsgs, INT32U maxMsgLength)
//Purpose:		create and initialize a message queue
//Note:			
//Parameters:   INT32U maxQSize			/* max queue can be creat */
//				INT32U	maxMsgs			/* max messages that can be queued */
//				INT32U	maxMsgLength	/* max bytes in a message */
//Return:		NULL if faile
//=======================================================
extern PMSG_ID_Q msgQCreate(INT32U maxQSize, INT32U maxMsgs, INT32U maxMsgLength);

//========================================================
//Function Name:msgQDelete
//Syntax:		void msgQDelete (PMSG_ID_Q msgQId)
//Purpose:		delete a message queue
//Note:			
//Parameters:   PMSG_ID_Q msgQId		/* message queue to delete */
//Return:		
//=======================================================
extern void msgQDelete (PMSG_ID_Q msgQId);

//========================================================
//Function Name:msgQSend
//Syntax:		INT32S msgQSend(PMSG_ID_Q msgQId, INT32U msg_id, void *para, INT32U nParaByte, INT32U priority)
//Purpose:		send a message to a message queue
//Note:			
//Parameters:   PMSG_ID_Q msgQId			/* message queue on which to send */
//				INT32U msg_id			/* message id */
//				void *para				/* message to send */
//				INT32U nParaByte		/* byte number of para buffer */
//				INT32U priority			/* MSG_PRI_NORMAL or MSG_PRI_URGENT */
//Return:		-1 if faile 
//				0 success
//=======================================================
extern INT32S msgQSend(PMSG_ID_Q msgQId, INT32U msg_id, void *para, INT32U nParaByte, INT32U priority);

//========================================================
//Function Name:msgQReceive
//Syntax:		INT32S msgQReceive(PMSG_ID_Q msgQId, INT32U *msg_id, void *para, INT32U maxParaNByte)
//Purpose:		receive a message from a message queue
//Note:			
//Parameters:   PMSG_ID_Q msgQId			/* message queue on which to send */
//				INT32U *msg_id			/* message id */
//				void *para				/* message and type received */
//				INT32U maxNByte			/* message size */
//Return:		-1: if faile
//				0: success
//=======================================================
extern INT32S msgQReceive(PMSG_ID_Q msgQId, INT32U *msg_id, void *para, INT32U maxParaNByte);

//========================================================
//Function Name:msgQAccept
//Syntax:		INT32S msgQAccept(PMSG_ID_Q msgQId, INT32U *msg_id, void *para, INT32U maxParaNByte)
//Purpose:		Check whether a message is available from a message queue
//Note:
//Parameters:   PMSG_ID_Q msgQId			/* message queue on which to send */
//				INT32U *msg_id			/* message id */
//				void *para				/* message and type received */
//				INT32U maxNByte			/* message size */
//Return:		-1: queue is empty or fail
//				0: success
//=======================================================
extern INT32S msgQAccept(PMSG_ID_Q msgQId, INT32U *msg_id, void *para, INT32U maxParaNByte);

//========================================================
//Function Name:msgQFlush
//Syntax:		void msgQFlush(PMSG_ID_Q msgQId)
//Purpose:		flush message queue
//Note:			
//Parameters:   PMSG_ID_Q msgQId
//Return:		
//=======================================================
extern void msgQFlush(PMSG_ID_Q msgQId);

//========================================================
//Function Name:msgQQuery
//Syntax:		INT8U msgQQuery(PMSG_ID_Q msgQId, OS_Q_DATA *pdata)
//Purpose:		get current Q message information
//Note:			
//Parameters:   PMSG_ID_Q msgQId, OS_Q_DATA *pdata
//Return:		
//=======================================================
extern INT8U msgQQuery(PMSG_ID_Q msgQId, OS_Q_DATA *pdata);

//========================================================
//Function Name:msgQSizeGet
//Syntax:		INT32U msgQSizeGet(PMSG_ID_Q msgQId)
//Purpose:		get current Q message number
//Note:			
//Parameters:   PMSG_ID_Q msgQId
//Return:		
//=======================================================
extern INT32U msgQSizeGet(PMSG_ID_Q msgQId);



#endif
