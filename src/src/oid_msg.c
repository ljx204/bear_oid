#include "oid_msg.h"

#if 1

#define MSG_Q_LOCK()		msgQ_LOCK()
#define MSG_Q_UNLOCK()		msgQ_UNLOCK()

static int gjmsg_sem = 0;
static oid_mutex_t  msg_mutex;

int msgQ_lock_Init(void) 
{
	if(gjmsg_sem == 0)
	{
    	     gjmsg_sem = 1;
	     oid_mutex_init(&msg_mutex);
	}
        return 1; 
}

int msgQ_lock_Exit(void) 
{
 	INT8U err;
	if(gjmsg_sem)
	{
		oid_mutex_destroy(&msg_mutex);
		gjmsg_sem = 0;
	}
    return (err != OS_NO_ERR);
}

static void msgQ_LOCK(void)
{
	INT8U err;
	if(!gjmsg_sem) 
	{
		msgQ_lock_Init();
	}
	if(gjmsg_sem) {
		oid_mutex_lock(&msg_mutex);
	}
}

static void msgQ_UNLOCK(void)
{
	
	if(gjmsg_sem) 
	  oid_mutex_unlock(&msg_mutex);
}
#endif

#if 0
typedef struct tagMsgPosObj {
	struct tagMsgPosObj *pNext;
} MSGQPosOBJ_DEF;
typedef struct tagMsgQPosFinder{
	MSGQPosOBJ_DEF EmptyListHeader;
} MSGQPosFinder_DEF;
/* Prototypes */

INT8U msgQQuery(PMSG_ID_Q msgQId, OS_Q_DATA *pdata);
INT32U msgQSizeGet(PMSG_ID_Q msgQId);

//========================================================

static void msgQBufMngInit(MSGQPosFinder_DEF *pMng,INT32U maxMsgs,INT32U maxMsgLength)
{
	MSGQPosOBJ_DEF *p0,*p1;
	MSGQPosOBJ_DEF *pObj = (MSGQPosOBJ_DEF *)&pMng[1];
	maxMsgLength += sizeof(MSGQPosOBJ_DEF);
	p0 = pObj;
	p1 = (MSGQPosOBJ_DEF *)((INT32U)(p0) + maxMsgLength);
	while(maxMsgs--) {
		p0->pNext = p1;
		if(maxMsgs)	{
			p0 = p1;
			p1 = (MSGQPosOBJ_DEF *)((INT32U)(p0) + maxMsgLength);
		}
	}
	p0->pNext = NULL;
	pMng->EmptyListHeader.pNext = pObj;
}

static void * msgQGetBuf(MSGQPosFinder_DEF *pMng)
{
	MSGQPosOBJ_DEF *p;
	if(pMng->EmptyListHeader.pNext) {
		p = pMng->EmptyListHeader.pNext;
		pMng->EmptyListHeader.pNext = p->pNext;
		return (void *)(&p[1]);
	}
	return NULL;
}
static void msgQFreeBuf(MSGQPosFinder_DEF *pMng,void *pBuf)
{
	MSGQPosOBJ_DEF *pObj = (MSGQPosOBJ_DEF *)((INT32U)(pBuf)-sizeof(MSGQPosOBJ_DEF));
	pObj->pNext = pMng->EmptyListHeader.pNext;
	pMng->EmptyListHeader.pNext = pObj;
}
#endif
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
PMSG_ID_Q msgQCreate(INT32U maxQSize, INT32U maxMsgs, INT32U maxMsgLength)
{
	PMSG_ID_Q	msgQId;
	OS_EVENT*	pEvent;
	void*		pMsgQ;
	INT32U		msg_length = (maxMsgLength + 4 - 1) & (~(4 - 1));
	INT32U		size = sizeof(*msgQId) + 						/* PMSG_ID_Q struct size */
					maxQSize * sizeof(void*) + 					/* OS Q size */
					maxMsgs * (msg_length + 4) ;  // +				/* type + message size */
					//sizeof(MSGQPosFinder_DEF) + maxMsgs * sizeof(MSGQPosOBJ_DEF);



	msgQId = (PMSG_ID_Q)malloc(size);
	if(msgQId == NULL)
	{
		return NULL;
	}

	memset((INT8S *)msgQId, 0, size);	/* clear out msg q structure */

	pMsgQ = (void*)(msgQId + 1);
	pEvent = OSQCreate(pMsgQ, maxQSize);
	if(pEvent == NULL)						/* creat q faile */
	{
		free(msgQId);
		return NULL;
	}

	oid_mutex_init(&msgQId->msg_mutex);
	msgQId->pEvent = pEvent;
	msgQId->pMsgQ = pMsgQ;
	msgQId->maxMsgs = maxMsgs;
	msgQId->maxMsgLength = msg_length;
	msgQId->pMsgPool = (INT8U*)((INT32U*)pMsgQ + maxQSize);
	msgQId->msg_index = 0;
	return ((PMSG_ID_Q) msgQId);
}

//========================================================
//Function Name:msgQDelete
//Syntax:		void msgQDelete (PMSG_ID_Q msgQId)
//Purpose:		delete a message queue
//Note:
//Parameters:   PMSG_ID_Q msgQId		/* message queue to delete */
//Return:
//=======================================================
void msgQDelete (PMSG_ID_Q msgQId)
{
	INT8U err;

//	oid_mutex_lock(&msgQId->msg_mutex);
	MSG_Q_LOCK();

	OSQDel(msgQId->pEvent, OS_DEL_ALWAYS, &err);

//	oid_mutex_unlock(&msgQId->msg_mutex);

	free((void *)msgQId);

	MSG_Q_UNLOCK();
//	oid_mutex_unlock(&msgQId->msg_mutex);
}

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
INT32S msgQSend(PMSG_ID_Q msgQId, INT32U msg_id, void *para, INT32U nParaByte, INT32U priority)
{
	INT8U	ret;
	INT32U	i;
	INT8U	*pMsg = msgQId->pMsgPool;
	INT32U	maxMsgLength = msgQId->maxMsgLength;
	INT32U	maxMsgs = msgQId->maxMsgs;
//	INT32U  test_ken;
	if(nParaByte > maxMsgLength)		/* too many parameter */
	{
		//printf("msgQSend .................. \r\n");
		return -1;
	}

	MSG_Q_LOCK();
//	oid_mutex_lock(&msgQId->msg_mutex);
#if 1
	i = 0;
	while( i < maxMsgs)
	{
		pMsg = msgQId->pMsgPool;

		pMsg += (maxMsgLength + 4) * msgQId->msg_index ;
		msgQId->msg_index++;
		if(msgQId->msg_index >= maxMsgs) {
			msgQId->msg_index = 0;
		}
		i++;
		
		if(*(INT32U*)pMsg == 0)
                        break;

	}	
#else
	/* find a free message */
	for(i = 0; i < maxMsgs; i++)
	{
		if(*(INT32U*)pMsg == 0)
			break;

		pMsg += (maxMsgLength + 4);
	}
#endif

	//printf(" i = %d \r\n", i);
	if(i == maxMsgs)					/* not enough message */
	{
		MSG_Q_UNLOCK();
		return -1;
	}

//	test_ken = (int)pMsg;

//	printf("address = %x \r\n", test_ken);

	*((INT32U*)pMsg) = msg_id;
	if(nParaByte)
	{
	      memcpy((INT8S*)(pMsg + 4), (INT8S*)para, nParaByte);
	}
	//printf("id = %d  %d \r\n", msg_id, nParaByte);



//	oid_mutex_unlock(&msgQId->msg_mutex);
//	printf("ok1 = 0x%x \r\n", *((INT32U *) test_ken));

	if(priority == MSG_PRI_NORMAL)
		ret = OSQPost(msgQId->pEvent, (void *)pMsg);
	else
		ret = OSQPostFront(msgQId->pEvent, (void *)pMsg);


//	oid_mutex_unlock(&msgQId->msg_mutex);
	MSG_Q_UNLOCK();

//	printf("ok3 = 0x%x \r\n", *((INT32U *) test_ken));
	return (INT32S)ret;
}

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
INT32S msgQReceive(PMSG_ID_Q msgQId, INT32U *msg_id, void *para, INT32U maxParaNByte)
{
	INT8U err;
	INT32U test_ken;
	void *pMsg;
//	printf("msgQReceive 11 \r\n");
	pMsg = OSQPend(msgQId->pEvent, 0, &err);
//	printf("msgQReceive 22 \r\n");
	if (err!=0 || !pMsg) {
		//printf("error1 \r\n");
		return -1;
	}

//	printf("Get msg address %x \r\n", (int)pMsg);

//	oid_mutex_lock(&msgQId->msg_mutex);
	MSG_Q_LOCK();

	if(maxParaNByte > msgQId->maxMsgLength)
	{
//		printf("2222222222222222 \r\n");
		//oid_mutex_unlock(&msgQId->msg_mutex);
		MSG_Q_UNLOCK();
		return -1;
	}
//	printf("1 \r\n");
//	test_ken = *(INT32U *)pMsg;
//	printf("test_ken = %x \r\n", test_ken);
	*msg_id = *((INT32U*)pMsg);
//	printf("2 \r\n");
	if(maxParaNByte && para)
	{
		memcpy((INT8S*)para, (INT8S*)pMsg + 4, maxParaNByte);
	}
	/* free message */
	*(INT32U*)pMsg = 0;	
//	oid_mutex_unlock(&msgQId->msg_mutex);
	MSG_Q_UNLOCK();
//	printf("3333333333333333 \r\n");
	return 0;
}

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
INT32S msgQAccept(PMSG_ID_Q msgQId, INT32U *msg_id, void *para, INT32U maxParaNByte)
{
	INT8U err;
	void *pMsg;

	pMsg = OSQAccept(msgQId->pEvent, &err);
	if (err!=OS_NO_ERR || !pMsg) {
		return -1;
	}

//	oid_mutex_lock(&msgQId->msg_mutex);
	MSG_Q_LOCK();

	if (maxParaNByte > msgQId->maxMsgLength) {
	  	MSG_Q_UNLOCK();

	//	oid_mutex_unlock(&msgQId->msg_mutex);
		return -1;
	}
	*msg_id = *((INT32U *) pMsg);
	if (maxParaNByte && para) {
		memcpy((INT8S *) para, (INT8S *) pMsg + 4, maxParaNByte);
	}
	*(INT32U *) pMsg = 0;				/* free message */

//	oid_mutex_unlock(&msgQId->msg_mutex);
	MSG_Q_UNLOCK();

	return 0;
}

//========================================================
//Function Name:msgQFlush
//Syntax:		void msgQFlush(PMSG_ID_Q msgQId)
//Purpose:		flush message queue
//Note:
//Parameters:   PMSG_ID_Q msgQId
//Return:
//=======================================================
void msgQFlush(PMSG_ID_Q msgQId)
{
	MSG_Q_LOCK();
//	oid_mutex_lock(&msgQId->msg_mutex);
	OSQFlush(msgQId->pEvent);
	memset( (INT8S *)msgQId->pMsgPool, 0, msgQId->maxMsgs * (msgQId->maxMsgLength + 4) );
	MSG_Q_UNLOCK();
//	oid_mutex_unlock(&msgQId->msg_mutex);
}

INT8U msgQQuery(PMSG_ID_Q msgQId, OS_Q_DATA *pdata)
{
    INT8U ret;

	//OSSchedLock();
    ret = OSQQuery(msgQId->pEvent, pdata);
	//OSSchedUnlock();
    return ret;
}

INT32U msgQSizeGet(PMSG_ID_Q msgQId)
{
    INT32U ret;
    OS_Q_DATA query_data;

    msgQQuery(msgQId, &query_data);
    ret = (INT32U) query_data.OSNMsgs;
    return ret;
}
