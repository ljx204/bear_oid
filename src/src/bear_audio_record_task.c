#include "bear_audio_record_task.h"

int SNDWAV_SetParams(SNDPCMContainer_t *sndpcm, unsigned short channel, int sample_rate) 
{ 
    snd_pcm_hw_params_t *hwparams; 
    snd_pcm_format_t format; 
    uint32_t exact_rate; 
    uint32_t buffer_time, period_time; 
//    printf("SNDWAV_SetParams ...... \r\n");
    /* Allocate the snd_pcm_hw_params_t structure on the stack. */ 
    snd_pcm_hw_params_alloca(&hwparams); 
     
    /* Init hwparams with full configuration space */ 
    if (snd_pcm_hw_params_any(sndpcm->handle, hwparams) < 0) { 
        fprintf(stderr, "Error snd_pcm_hw_params_any/n"); 
        goto ERR_SET_PARAMS; 
    } 
 
    if (snd_pcm_hw_params_set_access(sndpcm->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) { 
        fprintf(stderr, "Error snd_pcm_hw_params_set_access/n"); 
        goto ERR_SET_PARAMS; 
    } 
 
#if 0
    /* Set sample format */ 
    if (SNDWAV_P_GetFormat(wav, &format) < 0) { 
        fprintf(stderr, "Error get_snd_pcm_format/n"); 
        goto ERR_SET_PARAMS; 
    } 
#endif
    format = SND_PCM_FORMAT_S16_LE;

    if (snd_pcm_hw_params_set_format(sndpcm->handle, hwparams, format) < 0) { 
        fprintf(stderr, "Error snd_pcm_hw_params_set_format/n"); 
        goto ERR_SET_PARAMS; 
    } 
    sndpcm->format = format; 
 
    /* Set number of channels */ 
    if (snd_pcm_hw_params_set_channels(sndpcm->handle, hwparams, LE_SHORT(channel)) < 0) { 
        fprintf(stderr, "Error snd_pcm_hw_params_set_channels/n"); 
        goto ERR_SET_PARAMS; 
    } 
    sndpcm->channels = LE_SHORT(channel); 
 
    /* Set sample rate. If the exact rate is not supported */ 
    /* by the hardware, use nearest possible rate.         */  
    exact_rate = LE_INT(sample_rate); 
	if (snd_pcm_hw_params_set_rate_near(sndpcm->handle, hwparams, &exact_rate, 0) < 0) { 
        fprintf(stderr, "Error snd_pcm_hw_params_set_rate_near/n"); 
        goto ERR_SET_PARAMS; 
    } 
    if (LE_INT(sample_rate) != exact_rate) { 
        fprintf(stderr, "The rate %d Hz is not supported by your hardware./n ==> Using %d Hz instead./n",  
            LE_INT(sample_rate), exact_rate); 
    } 
 
    if (snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0) < 0) { 
        fprintf(stderr, "Error snd_pcm_hw_params_get_buffer_time_max/n"); 
        goto ERR_SET_PARAMS; 
    } 
    if (buffer_time > 500000) buffer_time = 500000; 
    period_time = buffer_time / 4; 
 
    if (snd_pcm_hw_params_set_buffer_time_near(sndpcm->handle, hwparams, &buffer_time, 0) < 0) { 
        fprintf(stderr, "Error snd_pcm_hw_params_set_buffer_time_near/n"); 
        goto ERR_SET_PARAMS; 
    } 
 
    if (snd_pcm_hw_params_set_period_time_near(sndpcm->handle, hwparams, &period_time, 0) < 0) { 
        fprintf(stderr, "Error snd_pcm_hw_params_set_period_time_near/n"); 
        goto ERR_SET_PARAMS; 
    } 
 
    /* Set hw params */ 
    if (snd_pcm_hw_params(sndpcm->handle, hwparams) < 0) { 
        fprintf(stderr, "Error snd_pcm_hw_params(handle, params)/n"); 
        goto ERR_SET_PARAMS; 
    } 
 
    snd_pcm_hw_params_get_period_size(hwparams, &sndpcm->chunk_size, 0);     
    snd_pcm_hw_params_get_buffer_size(hwparams, &sndpcm->buffer_size); 
    if (sndpcm->chunk_size == sndpcm->buffer_size) {         
        fprintf(stderr, ("Can't use period equal to buffer size (%lu == %lu) \r\nn"), sndpcm->chunk_size, sndpcm->buffer_size);      
        goto ERR_SET_PARAMS; 
    } 
 
    sndpcm->bits_per_sample = snd_pcm_format_physical_width(format); 
    sndpcm->bits_per_frame = sndpcm->bits_per_sample * LE_SHORT(channel); 
     
    sndpcm->chunk_bytes = sndpcm->chunk_size * sndpcm->bits_per_frame / 8; 
 
#if 0
    /* Allocate audio data buffer */ 
    sndpcm->data_buf = (uint8_t *)malloc(sndpcm->chunk_bytes); 
    if (!sndpcm->data_buf) { 
        fprintf(stderr, "Error malloc: [data_buf]/n"); 
        goto ERR_SET_PARAMS; 
    } 
#endif
  //  printf("EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE \r\n");
    return 0; 
 
ERR_SET_PARAMS: 
    return -1; 
}

typedef struct record_info{
	SNDPCMContainer_t record;
	unsigned int record_state;
	int sample_per_block;
}RECORD_INFO;

//static SNDPCMContainer_t record;
//static RECORD_INFO       g_record_info;

static void record_open_adc(void * param)
{
	char *devicename = "default"; 
	
	RECORD_INFO * p_info = (RECORD_INFO *)param;	

	memset(p_info, 0x0, sizeof(RECORD_INFO)); 
	
	if (snd_pcm_open(&p_info->record.handle, devicename, SND_PCM_STREAM_CAPTURE, 0) < 0) { 
            fprintf(stderr, "Error snd_pcm_open [ %s] \r\n", devicename); 
            return;
        } 

}

static void record_close_adc(void * param)
{
	RECORD_INFO * p_info = (RECORD_INFO *)param;

	if(p_info->record.handle != NULL)
		snd_pcm_close(p_info->record.handle);

	memset(p_info, 0x0, sizeof(RECORD_INFO));
}


typedef struct record_state {
	int state;
	int real_state;
	int bmute;
	oid_mutex_t   record_mutex;
}RECORD_STATE;

typedef enum {
	RECORD_STATE_MIN = 0,
	RECORD_STATE_BEGIN,
	RECORD_STATE_STOP,
	RECORD_STATE_MAX

}RECORD_STATE_ENUM;


static RECORD_STATE record_state;

void record_init_state(void)
{
	record_state.state = RECORD_STATE_MIN;
	oid_mutex_init( &record_state.record_mutex );
	record_state.bmute = 0;
}

void record_set_state(int state)
{
	oid_mutex_lock(&record_state.record_mutex );
	record_state.state = state;
	oid_mutex_unlock(&record_state.record_mutex );
}

int record_get_state(void)
{
	int tmp;
	oid_mutex_lock(&record_state.record_mutex );
        tmp = record_state.state;
        oid_mutex_unlock(&record_state.record_mutex );
	return tmp;
}

void record_stop(void)
{
	record_set_state(RECORD_STATE_STOP);
	while(1) {
		if(record_get_real_state()!= RECORD_STATE_STOP) msleep(100);
		else break;

	}
}

void record_set_real_state(int real_state)
{
	oid_mutex_lock(&record_state.record_mutex );
        record_state.real_state = real_state;
        oid_mutex_unlock(&record_state.record_mutex );

}

int record_get_real_state(void)
{
	int tmp;
        oid_mutex_lock(&record_state.record_mutex );
        tmp = record_state.real_state;
        oid_mutex_unlock(&record_state.record_mutex );
        return tmp;
}

int record_set_mute_state(int mute)
{
	oid_mutex_lock(&record_state.record_mutex );
        record_state.bmute = mute;
        oid_mutex_unlock(&record_state.record_mutex );


}

int record_get_mute_state(void)
{
	int tmp;
        oid_mutex_lock(&record_state.record_mutex );
        tmp = record_state.bmute;
        oid_mutex_unlock(&record_state.record_mutex );
        return tmp;
}


static void record_loop(void * param, block_fifo_t  * input_fifo)
{
	size_t r; 
    	size_t result = 0; 
    	size_t count  ; //= rcount;

 	int  buffer_size;

	int test_state;

	block_t * p_block;

	RECORD_INFO * p_info = (RECORD_INFO *)param;

	SNDPCMContainer_t *sndpcm = &p_info->record;

	//buffer_size = p_sound->info.sample_rate*p_sound->info.channels*2;

 //	p_block = block_Alloc(buffer_size);

	if(p_info->sample_per_block < 0)
		count = sndpcm->chunk_size;
	else
		count = p_info->sample_per_block;
//	p_block = block_Alloc(count*sndpcm->bits_per_frame / 8);
	
    	while (1) 
	{ 
		if(record_get_state() == RECORD_STATE_STOP)
		{
			 p_block = block_Alloc(count*sndpcm->bits_per_frame / 8);
			 memset(p_block->p_buffer, 0, count*sndpcm->bits_per_frame / 8);
			 block_FifoPut(input_fifo, p_block);
			 break;

		}

//		if(record_get_mute_state() == 1)
		
  		p_block = block_Alloc(count*sndpcm->bits_per_frame / 8);
      		
		r = snd_pcm_readi(sndpcm->handle, p_block->p_buffer, count);
		 
        	if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) { 
			snd_pcm_wait(sndpcm->handle, 1000); 
        	} 
		else if (r == -EPIPE) 
		{ 
			snd_pcm_prepare(sndpcm->handle); 
			fprintf(stderr, "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n"); 
        	} 
		else if (r == -ESTRPIPE) 
		{ 
			fprintf(stderr, "<<<<<<<<<<<<<<< Need suspend >>>>>>>>>>>>>>>\n"); 
        	} 
		else if (r < 0) 
		{ 
			fprintf(stderr, "Error snd_pcm_writei: [%s]", snd_strerror(r)); 
			//exit(-1); 
			break;
        	} 
         
        	if (r > 0) 
		{
			p_block->i_buffer = r * sndpcm->bits_per_frame / 8; 
			if(record_get_mute_state() == 1)
			{
			    memset(p_block->p_buffer, 0, r*sndpcm->bits_per_frame / 8);					
			}
			block_FifoPut(input_fifo, p_block);
			//printf("record .......................................................... \r\n");	 
        	}
		else 
		{
			block_Release(p_block);
			break;
		} 
    	}
 
    //	return rcount; 

}

#if 0
void audio_send_open_dac(void * param)
{
	if(global_app_var.p_record_event != NULL)
        	msgQSend(global_app_var.p_record_event, MSG_OPEN_ADC, param, sizeof(param), MSG_PRI_NORMAL);
}

#endif



//void audio_send_record_begin(PMSG_ID_Q hAudioTaskQ)
//{
//        msgQSend(hAudioTaskQ, MSG_OPEN_ADC, NULL, 0, MSG_PRI_NORMAL);
//}


void * Record_Task_Entry(void  *param)
{
	
	INT32S  nRet;
	
        INT32U  msg_id,wParam[AUDIO_RECORD_Q_MSGLEN/sizeof(INT32U)];
	
	block_fifo_t  * p_audio_input_fifo;

	PMSG_ID_Q record_event = (PMSG_ID_Q)param; 
	
	RECORD_INFO * p_record_info = (RECORD_INFO *)malloc(sizeof(RECORD_INFO));
	
	if(p_record_info == NULL) return;
	
	record_init_state();
	
	while(1) 
	{
		nRet = msgQReceive(record_event, &msg_id, &wParam, sizeof(wParam));
		printf("Record_Task_Entry msg_id  = %d \r\n", msg_id);
		switch(msg_id)
		{
			case MSG_OPEN_ADC:
			{
			//	audio_information * p_info = (audio_information *)wParam[0];
				MIXER_RECORD_INFO * p_info = (MIXER_RECORD_INFO *)wParam[0];
				record_open_adc(p_record_info);
				SNDWAV_SetParams(&p_record_info->record, p_info->info.channels, p_info->info.sample_rate);
				printf("channel = %d, samplerate = %d \r\n", p_info->info.channels, p_info->info.sample_rate);
				p_record_info->record_state = ADC_STATE_OPEN;  //???????
				p_record_info->sample_per_block = p_info->samples_per_block*p_info->info.channels;
				p_audio_input_fifo = p_info->p_audio_input_fifo;
				record_set_state(RECORD_STATE_BEGIN);
				
			}
				break;
			case MSG_RECORD_DATA:
			{
				record_set_real_state(RECORD_STATE_BEGIN);
				//block_fifo_t  * p_audio_input_fifo = (block_fifo_t  * )wParam[0];
				//printf("START MSG_RECORD_DATA........................................ \r\n");
				record_loop(p_record_info, p_audio_input_fifo);
				//printf("END MSG_RECORD_DATA........................................ \r\n");
				record_set_real_state(RECORD_STATE_STOP);
			}
				break;
			case MSG_CLOSE_ADC:
				printf("MSG_CLOSE_ADC ................................... \r\n");
				record_close_adc(p_record_info);
				break;
			default:
				break;


		}

	}
	

}
