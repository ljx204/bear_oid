#ifndef __PCM_ENCODE_H__
#define __PCM_ENCODE_H__
#include "oid_common.h"
//#include "bear_audio_encode_task.h"

typedef struct pcm_encode_info {
        WAVContainer_t wav;

}PCM_ENCODE_INFO;


int PCM_encode_begin(MIXER_RECORD_INFO * p_info);

int PCM_encode_stop(MIXER_RECORD_INFO * p_info);

int PCM_encode_loop(MIXER_RECORD_INFO * p_info);




#endif
