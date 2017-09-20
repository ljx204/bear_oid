#ifndef __BEAR_AUDIO_DAC_TASK_H__
#define __BEAR_AUDIO_DAC_TASK_H__
#include "oid_common.h"
#if 0
typedef struct  SNDPCMContainer { 
        snd_pcm_t *handle; 
        snd_output_t *log; 
        snd_pcm_uframes_t chunk_size; 
        snd_pcm_uframes_t buffer_size; 
        snd_pcm_format_t format; 
        uint16_t channels; 
        size_t chunk_bytes; 
        size_t bits_per_sample; 
        size_t bits_per_frame; 
     
        uint8_t *data_buf; 
    } SNDPCMContainer; 

#endif
void * audio_dac_thread(void *param);

void audio_dac_close(void *param);










#endif


