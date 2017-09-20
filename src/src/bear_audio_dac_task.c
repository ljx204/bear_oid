
#include "bear_audio_dac_task.h"
#if 0
typedef struct audio_information {
	unsigned short channels;
	int sample_rate;
}audio_information;
#endif

static int Sound_SetParams(SNDPCMContainer_t *sndpcm, audio_information *info) 
{ 
    snd_pcm_hw_params_t *hwparams; 
    snd_pcm_format_t format; 
    uint32_t exact_rate; 
    uint32_t buffer_time, period_time; 
 
    /* Allocate the snd_pcm_hw_params_t structure on the stack. */ 
    snd_pcm_hw_params_alloca(&hwparams); 
     
    /* Init hwparams with full configuration space */ 
    if (snd_pcm_hw_params_any(sndpcm->handle, hwparams) < 0) { 
        goto ERR_SET_PARAMS; 
    } 
 
    if (snd_pcm_hw_params_set_access(sndpcm->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) { 
        goto ERR_SET_PARAMS; 
    } 
 
   format = SND_PCM_FORMAT_S16_LE;
 
    if (snd_pcm_hw_params_set_format(sndpcm->handle, hwparams, format) < 0) { 
        goto ERR_SET_PARAMS; 
    }
 
    sndpcm->format = format; 
 
    /* Set number of channels */ 
    if (snd_pcm_hw_params_set_channels(sndpcm->handle, hwparams, LE_SHORT(info->channels)) < 0) { 
        goto ERR_SET_PARAMS; 
    } 
    sndpcm->channels = LE_SHORT(info->channels); 
    printf("channels = %d \r\n", sndpcm->channels);	
    /* Set sample rate. If the exact rate is not supported */ 
    /* by the hardware, use nearest possible rate.         */  
    exact_rate = LE_INT(info->sample_rate); 
	if (snd_pcm_hw_params_set_rate_near(sndpcm->handle, hwparams, &exact_rate, 0) < 0) { 
        goto ERR_SET_PARAMS; 
    } 
    if (LE_INT(info->sample_rate) != exact_rate) { 
       // fprintf(stderr, "The rate %d Hz is not supported by your hardware./n ==> Using %d Hz instead./n",  
        //    LE_INT(wav->format.sample_rate), exact_rate); 
    } 

    printf("sample rate = %d \r\n", info->sample_rate);
 
    if (snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0) < 0) { 
       // fprintf(stderr, "Error snd_pcm_hw_params_get_buffer_time_max/n"); 
        goto ERR_SET_PARAMS; 
    } 
    if (buffer_time > 500000) buffer_time = 500000; 
    period_time = buffer_time / 4; 
 
    if (snd_pcm_hw_params_set_buffer_time_near(sndpcm->handle, hwparams, &buffer_time, 0) < 0) { 
        goto ERR_SET_PARAMS; 
    } 
 
    if (snd_pcm_hw_params_set_period_time_near(sndpcm->handle, hwparams, &period_time, 0) < 0) { 
        goto ERR_SET_PARAMS; 
    } 
 
    /* Set hw params */ 
    if (snd_pcm_hw_params(sndpcm->handle, hwparams) < 0) { 
        goto ERR_SET_PARAMS; 
    } 
 
    snd_pcm_hw_params_get_period_size(hwparams, &sndpcm->chunk_size, 0);     
    snd_pcm_hw_params_get_buffer_size(hwparams, &sndpcm->buffer_size); 

//    printf("chunk_size = %d, buff_size = %d \r\n", sndpcm->chunk_size, sndpcm->buffer_size);

    if (sndpcm->chunk_size == sndpcm->buffer_size) {         
        goto ERR_SET_PARAMS; 
    } 
 
    sndpcm->bits_per_sample = snd_pcm_format_physical_width(format); 
    sndpcm->bits_per_frame = sndpcm->bits_per_sample * LE_SHORT(info->channels); 
     
    sndpcm->chunk_bytes = sndpcm->chunk_size * sndpcm->bits_per_frame / 8; 
 
 #if 0
    /* Allocate audio data buffer */ 
    sndpcm->data_buf = (uint8_t *)malloc(sndpcm->chunk_bytes); 
    if (!sndpcm->data_buf) { 
        goto ERR_SET_PARAMS; 
    } 
 #endif
    return 0; 
 
ERR_SET_PARAMS: 
    return -1; 
}

#define AUDIO_DAC_QUEUE_MAX   6
typedef struct audio_play_buffer {
	OS_EVENT * hAudioDacTaskQ;
	void *AudioDacTaskQ_area[AUDIO_DAC_QUEUE_MAX];
	

}audio_play_buffer;

void audio_dac_close(void *param)
{

	PLAYSOUND_VAR * p_sound = (PLAYSOUND_VAR *)param;
	SNDPCMContainer_t  *p_playback;

	if(p_sound) {
		p_playback = &p_sound->playback;
		snd_pcm_drain  (p_playback->handle);
		snd_pcm_close (p_playback->handle);  		
	}

}

static void PauseDummy (snd_pcm_t *  pcm, bool pause)
{
 
    /* Stupid device cannot pause. Discard samples. */
    if (pause)
        snd_pcm_drop (pcm);
    else
        snd_pcm_prepare (pcm);
    
}
static void audio_dac_pause (void *param, bool pause)
{
    	PLAYSOUND_VAR * p_sound = (PLAYSOUND_VAR *)param;
	SNDPCMContainer_t  *p_playback;
	int val;
	
	if(p_sound) {
		p_playback = &p_sound->playback;
		val = snd_pcm_pause (p_playback->handle, pause); 
		if(val < 0) {
			printf("No Pause ...\r\n");
			PauseDummy (p_playback->handle, pause);
		}		
	}
}

void * audio_dac_thread(void *param)
{
//	int n;
	block_t *p_block;
	char *devicename = "plug:dmix";
        SNDPCMContainer_t  *p_playback;
        PLAYSOUND_VAR * p_sound = (PLAYSOUND_VAR *)param;
	audio_information audio_info;
	p_playback = &p_sound->playback;
        if (snd_pcm_open(&p_playback->handle, devicename, SND_PCM_STREAM_PLAYBACK, 0) < 0) 
        { 
                return;
        } 
	audio_info = p_sound->info;
	Sound_SetParams(p_playback, &audio_info);

	while(1) {

		if(p_sound->play_state == AUDIO_PLAY_STOP)
		{
			while(1) {
				if(block_FifoCount(p_sound->p_audio_output_fifo) != 0)  {
					p_block = block_FifoGet(p_sound->p_audio_output_fifo);
					if(p_block > 0) {
						block_Release(p_block);
					}
				}
				else{
					while(1) {
						oid_testcancel();
						msleep(1000);
					}
				}
			}
		}
		else if(p_sound->play_state == AUDIO_PLAY_SET_PAUSE)
		{
			audio_dac_pause(p_sound, 1);
			p_sound->play_state = AUDIO_PLAY_PAUSE;
			continue;
		}

		else if(p_sound->play_state == AUDIO_PLAY_PAUSE)
		{
			msleep(1000);
			continue;
		}
		else if(p_sound->play_state == AUDIO_PLAY_SET_RESUME)
		{
			audio_dac_pause(p_sound, 0);
			p_sound->play_state = AUDIO_PLAY_STATE;
		}

		p_block = block_FifoGet(p_sound->p_audio_output_fifo);
	//	printf("audio_dac_thread \r\n");	
		if(p_block > 0) {
			int wcount = p_block->i_buffer;
			char *p_buf = p_block->p_buffer;
	//		printf("wcount  = %d \r\n", wcount);
			wcount >>= 1;
			if(audio_info.channels == 2) {
				wcount >>= 1;
			}
			
			while(wcount > 0)
                	{
                        	snd_pcm_sframes_t frames;
                	//        printf("222222222222222 \r\n");
                        	frames = snd_pcm_writei(p_playback->handle, p_buf,  wcount); 
                        	if (frames >= 0)
                        	{       
                                //	printf("frames = %d \r\n", frames);
                                	size_t bytes = snd_pcm_frames_to_bytes (p_playback->handle, frames);
                                	wcount -= frames;
                                	p_buf += bytes;
                          	  //      printf("byte = %d \r\n", bytes);
                        //block->i_buffer -= bytes;
                        // pts, length
                        	}
                        	else  
                        	{
                                	int val = snd_pcm_recover (p_playback->handle, frames, 1);
                                	if (val)
                                	{
                                        	printf( "cannot recover playback stream: %s", snd_strerror (val));
                        //      DumpDeviceStatus (aout, pcm);
                                        	break;
                                	}
                                	printf("cannot write samples: %s", snd_strerror (frames));
                        	}
                      //  	printf("wcount = %d \r\n", wcount);
                	}
			block_Release(p_block);

			if(p_sound->play_state == AUDIO_PLAY_STATE) 
			{

				if(block_FifoCount(p_sound->p_audio_output_fifo) < 5)
                        	{
                              		audio_send_next_frame_q(p_sound->p_decode_event);
                        	}

			}
			else if(p_sound->play_state == AUDIO_WAIT_PLAY_STOP) 
			{
				if(block_FifoCount(p_sound->p_audio_output_fifo) == 0) 
				{
					audio_send_stop_play(p_sound->p_decode_event);
				}
			}

		}


	}
}
