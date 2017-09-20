#include "msg_manage.h"

void  *OSQAccept (OS_EVENT *pevent, INT8U *err)
{
    void      *msg;
    OS_Q      *pq;

	oid_mutex_lock(&pevent->event_mutex);
    pq = (OS_Q *)pevent->OSEventPtr;             /* Point at queue control block                       */
    if (pq->OSQEntries > 0) {                    /* See if any messages in the queue                   */
        msg = *pq->OSQOut++;                     /* Yes, extract oldest message from the queue         */
        pq->OSQEntries--;                        /* Update the number of entries in the queue          */
        if (pq->OSQOut == pq->OSQEnd) {          /* Wrap OUT pointer if we are at the end of the queue */
            pq->OSQOut = pq->OSQStart;
        }
        *err = OS_NO_ERR;
    } else {
        *err = OS_Q_EMPTY;
        msg  = (void *)0;                        /* Queue is empty                                     */
    }
	oid_mutex_unlock(&pevent->event_mutex);

    return (msg);                                /* Return message received (or NULL)                  */
}

OS_EVENT  *OSQCreate (void **start, INT16U size)
{
	OS_EVENT	*pevent;
	OS_Q		*pq;
	

	pevent	= (OS_EVENT *)malloc(sizeof(OS_EVENT));
	if(pevent == NULL){
		return ((OS_EVENT *)0);
	}

	pq = (OS_Q *)malloc(sizeof(OS_Q));
	if(pq == NULL) {
		free(pevent);
		return ((OS_EVENT *)0);
	}
	
 //       printf("size = %d \r\n", size);
	pq->OSQStart           = start;               /*      Initialize the queue                 */
        pq->OSQEnd             = &start[size];
        pq->OSQIn              = start;
        pq->OSQOut             = start;
        pq->OSQSize            = size;
        pq->OSQEntries         = 0;
      //  pevent->OSEventType    = OS_EVENT_TYPE_Q;
        pevent->OSEventCnt     = 0;
        pevent->OSEventPtr     = pq;
	pevent->OSEventWait = 0;

	oid_mutex_init(&pevent->event_mutex);
	oid_cond_init(&pevent->event_cond);
	
//	printf("%d, %d \r\n", pq->OSQSize, pq->OSQEntries);
	return pevent;
}

void  *OSQPend (OS_EVENT *pevent, INT16U timeout, INT8U *err)
{
    void      *msg;
    OS_Q      *pq;


//    printf("OSQPend11 pevent = 0x%x ...................................\r\n", (int)pevent);
    
    oid_mutex_lock(&pevent->event_mutex);

//    printf("OSQPend22 ...................................\r\n");
pend_try_again: 
 //  printf("pend event wait = %d \r\n", pevent->OSEventWait);
   pq = (OS_Q *)pevent->OSEventPtr;             /* Point at queue control block                       */
    if (pq->OSQEntries > 0) {                    /* See if any messages in the queue                   */
        msg = *pq->OSQOut++;                     /* Yes, extract oldest message from the queue         */
        pq->OSQEntries--;                        /* Update the number of entries in the queue          */
        if (pq->OSQOut == pq->OSQEnd) {          /* Wrap OUT pointer if we are at the end of the queue */
            pq->OSQOut = pq->OSQStart;
        }
	oid_mutex_unlock(&pevent->event_mutex);
        *err = OS_NO_ERR;
        return (msg);                            /* Return message received                            */
    }
    pevent->OSEventWait++;
    oid_cond_wait(&pevent->event_cond, &pevent->event_mutex);
    goto pend_try_again;
 //   msg = pevent->msg;
 //   oid_mutex_unlock(&pevent->event_mutex);
    *err = OS_NO_ERR;
    return  msg;		
}


INT8U  OSQPost (OS_EVENT *pevent, void *msg)
{
    OS_Q      *pq;

    //INT32U  test_ken = (int)msg;
	
    oid_mutex_lock(&pevent->event_mutex);
#if 0 
   if (pevent->OSEventWait != 0) {                     /* See if any task pending on queue             */
        pevent->OSEventWait--;                                /* Find highest priority task ready to run      */
	pevent->msg = msg;
	oid_cond_signal(&pevent->event_cond);
	oid_mutex_unlock(&pevent->event_mutex);
	return (OS_NO_ERR);
    }
#endif
//	printf("OSQPost %d, %d \r\n", pq->OSQEntries, pq->OSQSize);
 //     printf("ok2 = 0x%x \r\n", *((INT32U *) test_ken));
      pq = (OS_Q *)pevent->OSEventPtr;                   /* Point to queue control block                 */
 // printf("OSQPost %d, %d \r\n", pq->OSQEntries, pq->OSQSize);  
  if (pq->OSQEntries >= pq->OSQSize) {               /* Make sure queue is not full                  */
	 oid_mutex_unlock(&pevent->event_mutex);
	// printf("111111111111111111 \r\n");
        return (OS_Q_FULL);
    }

   // printf("ok5 = 0x%x \r\n", *((INT32U *) msg));

  //  pq = (OS_Q *)pevent->OSEventPtr;
    *pq->OSQIn++ = msg;                                /* Insert message into queue                    */
   // printf("ok6 = 0x%x ______________\r\n", *((INT32U *) msg));

    pq->OSQEntries++;                                  /* Update the nbr of entries in the queue       */
    if (pq->OSQIn == pq->OSQEnd) {                     /* Wrap IN ptr if we are at end of queue        */
        pq->OSQIn = pq->OSQStart;
//	printf("wrapper ..... \r\n");
    }
    //  printf("post event wait = %d \r\n", pevent->OSEventWait);
     //   printf("q size = %d \r\n", pq->OSQEntries);


      if (pevent->OSEventWait != 0) {                     /* See if any task pending on queue             */
		pevent->OSEventWait--; 
	//	printf("11111 \r\n");
	//	oid_mutex_unlock(&pevent->event_mutex);
		oid_cond_signal(&pevent->event_cond);
	        oid_mutex_unlock(&pevent->event_mutex);
	//	printf("ok2 = 0x%x \r\n", *((INT32U *) test_ken));
		return (OS_NO_ERR);
       }
   // printf("ok4 = 0x%x \r\n", *((INT32U *) test_ken));
    oid_mutex_unlock(&pevent->event_mutex);
    return (OS_NO_ERR);
	
}

INT8U  OSQPostFront (OS_EVENT *pevent, void *msg)
{
    OS_Q      *pq;


	oid_mutex_lock(&pevent->event_mutex);	
    if (pevent->OSEventWait != 0) {                     /* See if any task pending on queue             */
        pevent->OSEventWait--;                                /* Find highest priority task ready to run      */
	pevent->msg = msg;
	oid_cond_signal(&pevent->event_cond);
        oid_mutex_unlock(&pevent->event_mutex);  
	return (OS_NO_ERR);
    }

	pq = (OS_Q *)pevent->OSEventPtr;                  /* Point to queue control block                  */
    if (pq->OSQEntries >= pq->OSQSize) {              /* Make sure queue is not full                   */
	oid_mutex_unlock(&pevent->event_mutex);
        return (OS_Q_FULL);
    }
    if (pq->OSQOut == pq->OSQStart) {                 /* Wrap OUT ptr if we are at the 1st queue entry */
        pq->OSQOut = pq->OSQEnd;
    }
    pq->OSQOut--;
    *pq->OSQOut = msg;                                /* Insert message into queue                     */
    pq->OSQEntries++;                                 /* Update the nbr of entries in the queue        */
    if (pevent->OSEventWait != 0) {                     /* See if any task pending on queue             */
        pevent->OSEventWait--;                                /* Find highest priority task ready to run      */
	 oid_cond_signal(&pevent->event_cond);
        oid_mutex_unlock(&pevent->event_mutex);

	return (OS_NO_ERR);
    }

   	oid_mutex_unlock(&pevent->event_mutex);
	 return (OS_NO_ERR);

}

INT8U  OSQQuery (OS_EVENT *pevent, OS_Q_DATA *p_q_data)
{
    OS_Q      *pq;
    INT8U      i;

    oid_mutex_lock(&pevent->event_mutex);
    pq = (OS_Q *)pevent->OSEventPtr;
    if (pq->OSQEntries > 0) {
        p_q_data->OSMsg = *pq->OSQOut;                 /* Get next message to return if available      */
    } else {
        p_q_data->OSMsg = (void *)0;
    }
    p_q_data->OSNMsgs = pq->OSQEntries;
    p_q_data->OSQSize = pq->OSQSize;

   oid_mutex_unlock(&pevent->event_mutex);
    return (OS_NO_ERR);
}

INT8U  OSQFlush (OS_EVENT *pevent)
{
    OS_Q      *pq;

	oid_mutex_lock(&pevent->event_mutex);
    pq             = (OS_Q *)pevent->OSEventPtr;      /* Point to queue storage structure              */
    pq->OSQIn      = pq->OSQStart;
    pq->OSQOut     = pq->OSQStart;
    pq->OSQEntries = 0;
	oid_mutex_unlock(&pevent->event_mutex);  
  return (OS_NO_ERR);
}

OS_EVENT  *OSQDel (OS_EVENT *pevent, INT8U opt, INT8U *err)
{
    BOOLEAN    tasks_waiting;
    OS_EVENT  *pevent_return;
    OS_Q      *pq;


	oid_mutex_lock(&pevent->event_mutex);
    if (pevent->OSEventWait != 0) {                         /* See if any tasks waiting on queue        */
        tasks_waiting = OS_TRUE;                              /* Yes                                      */
    } else {
        tasks_waiting = OS_FALSE;                             /* No                                       */
    }
    switch (opt) {
        case OS_DEL_NO_PEND:                               /* Delete queue only if no task waiting     */
             if (tasks_waiting == OS_FALSE) {
		 free((OS_Q *)pevent->OSEventPtr);
		 oid_mutex_destroy( &pevent->event_mutex );
	         oid_cond_destroy(&pevent->event_cond);
	
		free(pevent);
                 *err                   = OS_NO_ERR;
                 pevent_return          = (OS_EVENT *)0;   /* Queue has been deleted                   */
             } else {
                oid_mutex_unlock(&pevent->event_mutex);
		 *err                   = OS_ERR_TASK_WAITING;
                 pevent_return          = pevent;
             }
             break;

        case OS_DEL_ALWAYS:                                /* Always delete the queue                  */
             while (pevent->OSEventWait != 0) {             /* Ready ALL tasks waiting for queue        */
                 pevent->OSEventWait--;                                /* Find highest priority task ready to run      */
		 pevent->msg = (void *)0;
		 oid_mutex_unlock(&pevent->event_mutex); 
		 oid_cond_signal(&pevent->event_cond);
		 oid_mutex_lock(&pevent->event_mutex);
             }
	     free((OS_Q *)pevent->OSEventPtr);
	     oid_mutex_destroy( &pevent->event_mutex );
	     oid_cond_destroy(&pevent->event_cond);

	     free(pevent);
             *err                   = OS_NO_ERR;
             pevent_return          = (OS_EVENT *)0;       /* Queue has been deleted                   */
             break;

        default:
	     oid_mutex_unlock(&pevent->event_mutex);
             *err                   = OS_ERR_INVALID_OPT;
             pevent_return          = pevent;
             break;
    }
    return (pevent_return);
}

