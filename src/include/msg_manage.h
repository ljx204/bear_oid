#ifndef __MSG_MANNAGE_H__
#define __MSG_MANNAGE_H__
#include <unistd.h> /* _POSIX_SPIN_LOCKS */
#include <pthread.h>
#include <semaphore.h>
#include "Thread.h"
#include <stdlib.h>

#ifndef  OS_FALSE
#define  OS_FALSE                     0u
#endif

#ifndef  OS_TRUE
#define  OS_TRUE                         1u
#endif


#define OS_NO_ERR                     0u

#define OS_ERR_EVENT_TYPE             1u
#define OS_ERR_PEND_ISR               2u
#define OS_ERR_POST_NULL_PTR          3u
#define OS_ERR_PEVENT_NULL            4u
#define OS_ERR_POST_ISR               5u
#define OS_ERR_QUERY_ISR              6u
#define OS_ERR_INVALID_OPT            7u
#define OS_ERR_TASK_WAITING           8u
#define OS_ERR_PDATA_NULL             9u

#define OS_TIMEOUT                   10u
#define OS_TASK_NOT_EXIST            11u
#define OS_ERR_EVENT_NAME_TOO_LONG   12u
#define OS_ERR_FLAG_NAME_TOO_LONG    13u
#define OS_ERR_TASK_NAME_TOO_LONG    14u
#define OS_ERR_PNAME_NULL            15u
#define OS_ERR_TASK_CREATE_ISR       16u
#define OS_ERR_PEND_LOCKED           17u

#define OS_MBOX_FULL                 20u

#define OS_Q_FULL                    30u
#define OS_Q_EMPTY                   31u


#define  OS_DEL_NO_PEND               0u
#define  OS_DEL_ALWAYS                1u


typedef unsigned char  BOOLEAN;                
typedef unsigned char  INT8U;                   
typedef signed   char  INT8S;                  
typedef unsigned short INT16U;                  
typedef signed   short INT16S;                  
typedef unsigned int   INT32U;                 
typedef signed   int   INT32S;                 
typedef float          FP32;                    
typedef double         FP64;                   
typedef	signed   long long      INT64S;	        
typedef	unsigned long long      INT64U;	        


typedef struct os_event {
    INT8U    OSEventType;                    /* Type of event control block (see OS_EVENT_TYPE_xxxx)    */
    void    *OSEventPtr;                     /* Pointer to message or queue structure                   */
    INT16U   OSEventCnt;                     /* Semaphore Count (not used if other EVENT type)          */

    void		*msg;
    INT16U		OSEventWait;
    oid_mutex_t 	event_mutex;
    oid_cond_t		event_cond;	 
//   oid_sem_t 		wait_sem;

} OS_EVENT;

typedef struct os_q {                   /* QUEUE CONTROL BLOCK                                         */
    struct os_q   *OSQPtr;              /* Link to next queue control block in list of free blocks     */
    void         **OSQStart;            /* Pointer to start of queue data                              */
    void         **OSQEnd;              /* Pointer to end   of queue data                              */
    void         **OSQIn;               /* Pointer to where next message will be inserted  in   the Q  */
    void         **OSQOut;              /* Pointer to where next message will be extracted from the Q  */
    INT16U         OSQSize;             /* Size of queue (maximum number of entries)                   */
    INT16U         OSQEntries;          /* Current number of entries in the queue                      */
} OS_Q;

typedef struct os_q_data {
    void          *OSMsg;               /* Pointer to next message to be extracted from queue          */
    INT16U         OSNMsgs;             /* Number of messages in message queue                         */
    INT16U         OSQSize;             /* Size of message queue                                       */

} OS_Q_DATA;



#endif
