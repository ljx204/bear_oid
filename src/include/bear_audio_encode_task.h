#ifndef __BEAR_AUDIO_ENCODE_TASK_H__
#define __BEAR_AUDIO_ENCODE_TASK_H__
#include "oid_common.h"
//#include "pcm_encode.h"
//#include "adpcm_encode.h"
#include "adpcm-lib.h"
#include "lame.h"
typedef struct mixer_record_info {
        audio_information info;
        int   fd_record;
        oid_thread_t  p_encode_thread;
        PMSG_ID_Q     p_encode_event;
	block_fifo_t  * p_audio_input_fifo;
	int (*record_loop)(struct mixer_record_info *p);
	int (*record_start)(struct mixer_record_info *p);
	int (*record_stop)(struct mixer_record_info *p);
	int record_length;
	int record_sample;
	int samples_per_block;
	int encode_state;
	void * private_data;
}MIXER_RECORD_INFO;

typedef enum encode_sate {
	ENCODE_STATE_NULL = 0,
	ENCODE_STATE_BEGIN,
	ENCODE_STATE_PAUSE,
	ENCODE_STATE_STOP,
	ENCODE_STATE_MAX

}ENCODE_STATE;

typedef enum msg_encode {
	MSG_ENCODE_MIN,
	MSG_ENCODE_BEGIN,
	MSG_ENCONE_NEXT_FRAME,
	MSG_ENCODE_STOP,
	MSG_ENCODE_MAX


}MSG_ENCODE_ENUM;

void *  Mixer_RecordStart(const char * pszName, unsigned int  audio_format, unsigned int sample_rate, unsigned int  bit_rate, unsigned short channels);

int Mixer_RecordStop(void * param);

#include "pcm_encode.h"
#include "adpcm_encode.h"
#include "lame_mp3_encode.h"
#endif

