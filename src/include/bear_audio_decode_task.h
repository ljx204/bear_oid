#ifndef __BEAR_AUDIO_DECODE_TASK_H__
#define __BEAR_AUDIO_DECODE_TASK_H__
#include "oid_common.h"
#include "wav_decode.h"
#include "mp3_decode.h"
#include "ima_adpcm_decode.h"
AUDIO_TYPE audio_get_type(const char * file_ext_name) ;
void * audio_decode_thread(void *param);




#endif
