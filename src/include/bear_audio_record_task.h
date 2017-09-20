#ifndef __BEAR_AUDIO_RECORD_TASK_H__
#define __BEAR_AUDIO_RECORD_TASK_H__
#include "oid_common.h"
typedef enum {
	MSG_RECORD_MIN = 0,
	MSG_OPEN_ADC,
	MSG_RECORD_DATA,
	MSG_CLOSE_ADC,
	
	MSG_RECORD_MAX,

}MSG_RECORD_ENUM;

typedef enum {
	ADC_STATE_MIN = 0,
	ADC_STATE_OPEN,
	ADC_STATE_RECORD,
	ADC_STATE_CLOSE,
	
	ADC_STATE_MAX

}ADC_STATE_ENUM;

void * Record_Task_Entry(void  *param);

#endif

