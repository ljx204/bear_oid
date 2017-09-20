#include "bear_audio_encode_task.h"


void audio_send_stop_encode(PMSG_ID_Q hAudioTaskQ)
{
        msgQSend(hAudioTaskQ, MSG_ENCODE_STOP, NULL, 0, MSG_PRI_NORMAL);
}

void audio_send_begin_encode_q(PMSG_ID_Q hAudioTaskQ)
{
        msgQSend(hAudioTaskQ, MSG_ENCODE_BEGIN, NULL, 0, MSG_PRI_NORMAL);
}

void audio_send_next_frame_encode_q(PMSG_ID_Q hAudioTaskQ)
{
        msgQSend(hAudioTaskQ, MSG_ENCONE_NEXT_FRAME, NULL, 0, MSG_PRI_NORMAL);
}



void * audio_encode_thread(void  *param)
{
	INT32S  nRet;

        INT32U  msg_id,wParam[AUDIO_ENCODE_Q_MSGLEN/sizeof(INT32U)];

	MIXER_RECORD_INFO * p_mixer_info = (MIXER_RECORD_INFO *)param;

	
	while(1) 
	{
		nRet = msgQReceive(p_mixer_info->p_encode_event, &msg_id, &wParam, sizeof(wParam));
               // printf("audio_encode_thread msg_id  = %d \r\n", msg_id);
                switch(msg_id)
                {
			case MSG_ENCODE_BEGIN:
			{
#if 0
				audio_send_open_adc(p_mixer_info);
				audio_send_next_frame_encode_q(p_mixer_info->p_encode_event);
				audio_send_record_data();
#endif
				if(p_mixer_info->record_start != NULL )
				{
					p_mixer_info->record_start(p_mixer_info);
				}
				audio_send_open_adc(p_mixer_info);
                                audio_send_next_frame_encode_q(p_mixer_info->p_encode_event);
                                audio_send_record_data();

			}				
			break;

			case MSG_ENCONE_NEXT_FRAME:
				if(p_mixer_info->record_loop != NULL) 
				{
					p_mixer_info->record_loop(p_mixer_info);
				}
			break;

			case MSG_ENCODE_STOP:
				if(p_mixer_info->record_stop != NULL)
                                {
                                        p_mixer_info->record_stop(p_mixer_info);
                                }
				while(1) {
					if(block_FifoCount(p_mixer_info->p_audio_input_fifo) != 0) {
						block_t * p_block;
						p_block = block_FifoGet(p_mixer_info->p_audio_input_fifo);
						block_Release(p_block);
					}
					else {
						break;
					}
				}
				app_end_encode_sound(&p_mixer_info);
				while(1) {
					oid_testcancel();
                                        msleep(1000);
				}

                        break;

			default:
			break;
		}

		
	}

}

//----------------------------------------------PCM -------------------------------------------------
#if 0
int PCM_encode_begin(MIXER_RECORD_INFO * p_info)
{

	PCM_ENCODE_INFO * p_pcm;
	p_info->private_data = (PCM_ENCODE_INFO *)malloc(sizeof(PCM_ENCODE_INFO));
	if(!p_info->private_data) 
	{
		printf("PCM encode begin no memory \r\n");
		return 0;
	}

	p_pcm = (PCM_ENCODE_INFO *)p_info->private_data;
	
	p_pcm->wav.format.channels = LE_SHORT(p_info->info.channels);
        p_pcm->wav.format.sample_rate = LE_INT(p_info->info.sample_rate);
	p_pcm->wav.format.format = LE_SHORT(WAV_FMT_PCM);
	SNDWAV_PrepareWAVParams(&p_pcm->wav);

	WAV_WriteHeader(p_info->fd_record, &p_pcm->wav);
	p_info->record_length = 0;
	p_info->samples_per_block = -1;
	return 1;


}

int PCM_encode_stop(MIXER_RECORD_INFO * p_info)
{
	PCM_ENCODE_INFO * p_pcm;
	p_pcm = (PCM_ENCODE_INFO *)p_info->private_data;

        p_pcm->wav.chunk.length = LE_INT(p_info->record_length);

	printf("chunk length = %d \r\n", p_pcm->wav.chunk.length);
	SNDWAV_PrepareWAVParams(&p_pcm->wav);

	lseek(p_info->fd_record, 0 ,SEEK_SET);
        WAV_WriteHeader(p_info->fd_record, &p_pcm->wav);
	close(p_info->fd_record);

   //     p_info->record_length = 0;
	free(p_info->private_data);
        return 1;

}

int PCM_encode_loop(MIXER_RECORD_INFO * p_info)
{
	block_t *p_block;
        if(p_info != NULL) {
                p_block = block_FifoGet(p_info->p_audio_input_fifo);
        //      printf("audio_dac_thread \r\n");        
                if(p_block > 0)
                {
                        int wcount = p_block->i_buffer;
                        char *p_buf = p_block->p_buffer;
                       // printf("PCM count = %d \r\n", wcount);
			if(write(p_info->fd_record, p_buf, wcount) != wcount) {
				printf("error write file .................\r\n");
				exit(-1);
			}
			p_info->record_length += wcount;			
                        block_Release(p_block);
//just test .........................................................................
			if(p_info->record_length >= 1024*1024) {
				record_stop();
				audio_send_stop_encode(p_info->p_encode_event);
				return;
			}
//---------------------------------------------------------------------------------
                        audio_send_next_frame_encode_q(p_info->p_encode_event);
                }
        }

}
#endif
#if 0
//-------------ADPCM------------------------------------------------------------------------------
int ADPCM_encode_begin(MIXER_RECORD_INFO * p_info)
{
	ADPCM_ENCODE_INFO * p_adpcm;
        p_info->private_data = (ADPCM_ENCODE_INFO *)malloc(sizeof(ADPCM_ENCODE_INFO));
        if(!p_info->private_data)
        {
                printf("PCM encode begin no memory \r\n");
                return 0;
        }
	

        p_adpcm = (ADPCM_ENCODE_INFO *)p_info->private_data;
	p_adpcm->adpcm_cnxt = NULL;	
	{
		int block_size = 256 * p_info->info.channels * (p_info->info.sample_rate < 11000 ? 1 : (p_info->info.sample_rate) / 11000);

        	p_info->samples_per_block = (block_size - p_info->info.channels * 4) * (p_info->info.channels ^ 3) + 1;
		printf("samples_per_block = %d , %d , %d, %d \r\n", p_info->samples_per_block, block_size ,\
					 p_info->info.channels , p_info->info.sample_rate );

	}
/*
        p_adpcm->wavhdr.channels = LE_SHORT(p_info->info.channels);
        p_adpcm->wavhdr.format.sample_rate = LE_INT(p_info->info.sample_rate);
        p_adpcm->wavhdr.format.format = LE_SHORT(WAV_FMT_ADPCM);
*/
//	memset (&p_adpcm->wavhdr, 0, sizeof (p_adpcm->wavhdr));
  	
	
//        SNDWAV_PrepareWAVParams(&p_adpcm->wav);

//        WAV_WriteHeader(p_info->fd_record, &p_adpcm->wav);
	write_adpcm_wav_header(p_info->fd_record, LE_SHORT(p_info->info.channels), \
				10,
				LE_INT(p_info->info.sample_rate), \
				p_info->samples_per_block);

        p_info->record_length = 0;
	p_info->record_sample = 0;
        return 1;


}

int ADPCM_encode_stop(MIXER_RECORD_INFO * p_info)
{
	ADPCM_ENCODE_INFO * p_adpcm;
        p_adpcm = (ADPCM_ENCODE_INFO *)p_info->private_data;
#if 0
        p_adpcm->wav.chunk.length = LE_INT(p_info->record_length);

        printf("chunk length = %d \r\n", p_adpcm->wav.chunk.length);
        SNDWAV_PrepareWAVParams(&p_adpcm->wav);
#endif
        lseek(p_info->fd_record, 0 ,SEEK_SET);
      //  WAV_WriteHeader(p_info->fd_record, &p_adpcm->wav);
	 write_adpcm_wav_header(p_info->fd_record, LE_SHORT(p_info->info.channels), \
                                p_info->record_sample,
                                LE_INT(p_info->info.sample_rate), \
                                p_info->samples_per_block);
        close(p_info->fd_record);

	if(p_adpcm->adpcm_cnxt)
		adpcm_free_context (p_adpcm->adpcm_cnxt);

        free(p_info->private_data);
        return 1;



}


int ADPCM_encode_loop(MIXER_RECORD_INFO * p_info)
{
	ADPCM_ENCODE_INFO * p_adpcm = (ADPCM_ENCODE_INFO *)p_info->private_data;
	block_t *p_block;
	if(p_info != NULL) {
		p_block = block_FifoGet(p_info->p_audio_input_fifo);
        //      printf("audio_dac_thread \r\n");        
                if(p_block > 0) 
		{
                        int wcount = p_block->i_buffer;
                        int16_t *p_buf = p_block->p_buffer;
			int num_channels = p_info->info.channels;
			int sample_rate = p_info->info.sample_rate;
			int num_bytes;
			if(!p_adpcm->adpcm_cnxt) 
			{
				int32_t average_deltas [2];
            			int i;
				int noise_shaping = (sample_rate > 64000 ? NOISE_SHAPING_STATIC : NOISE_SHAPING_DYNAMIC);

            			average_deltas [0] = average_deltas [1] = 0;

            			for (i = wcount/2 - 1; i -= num_channels;) 
				{
                			average_deltas [0] -= average_deltas [0] >> 3;
                			average_deltas [0] += abs ((int32_t) p_buf [i] - p_buf [i - num_channels]);

                			if (num_channels == 2) 
					{
                    				average_deltas [1] -= average_deltas [1] >> 3;
                    				average_deltas [1] += abs ((int32_t) p_buf [i-1] - p_buf [i+1]);
                			}
            			}

            			average_deltas [0] >>= 3;
            			average_deltas [1] >>= 3;

            			p_adpcm->adpcm_cnxt = adpcm_create_context (num_channels, 3, noise_shaping, average_deltas);

			}
			void * p_adpcm_block = malloc(wcount>>1);
			if(num_channels == 1) {
				adpcm_encode_block (p_adpcm->adpcm_cnxt, p_adpcm_block, &num_bytes, p_buf, wcount>>1);
				p_info->record_sample += wcount>>1;
		//		p_info->samples_per_block = wcount>>1;
			}
			else {
				adpcm_encode_block (p_adpcm->adpcm_cnxt, p_adpcm_block, &num_bytes, p_buf, wcount>>2);
				p_info->record_sample += wcount>>2;
		//		p_info->samples_per_block = wcount>>2;
			}
			
			write(p_info->fd_record, p_adpcm_block, num_bytes);
			p_info->record_length += num_bytes;

			printf("adpcm %d , %d , %d, %d \r\n", num_bytes, wcount, p_info->record_length, p_info->record_sample);
			free(p_adpcm_block);
			
		//	printf("wcount = %d ________________________________________________________\r\n", wcount);
//-----------------------------------------------------------------------------------------------
			if(p_info->record_length >= 1024*300) {
                                record_stop();
                                audio_send_stop_encode(p_info->p_encode_event);
                                return;
                        }
//---------------------------------------------------------------------------------------
			block_Release(p_block);
			audio_send_next_frame_encode_q(p_info->p_encode_event);
		}
	}

}
#endif

void *  Mixer_RecordStart(const char * pszName, unsigned int  audio_format, unsigned int sample_rate, unsigned int  bit_rate, unsigned short channels)
{

	MIXER_RECORD_INFO * p_mixer_info = (MIXER_RECORD_INFO *)malloc(sizeof(MIXER_RECORD_INFO));
	
	if(p_mixer_info == NULL) return NULL;
	
	p_mixer_info->encode_state = ENCODE_STATE_NULL;

	p_mixer_info->info.channels = channels;

	p_mixer_info->info.sample_rate = sample_rate;

	p_mixer_info->info.bit_rate = bit_rate;

	switch(audio_format){
		case BEAR_AUD_FORMAT_ADPCM:
//			p_mixer_info->record_loop = PCM_encode_loop;
                        p_mixer_info->record_start = ADPCM_encode_begin;
                        p_mixer_info->record_stop =  ADPCM_encode_stop;
			p_mixer_info->record_loop =  ADPCM_encode_loop;

			break;
		case BEAR_AUD_FORMAT_PCM:
			p_mixer_info->record_loop = PCM_encode_loop;
			p_mixer_info->record_start = PCM_encode_begin;
			p_mixer_info->record_stop = PCM_encode_stop;
			break;
		case BEAR_AUD_FORMAT_MP3:
			p_mixer_info->record_loop = LAME_MP3_encode_loop;
                        p_mixer_info->record_start = LAME_MP3_encode_begin;
                        p_mixer_info->record_stop = LAME_MP3_encode_stop;
			break;
		default:
			
			free(p_mixer_info);
			return NULL;
			break;
	}


	p_mixer_info->p_encode_event = msgQCreate(AUDIO_ENCODE_Q_SIZE, AUDIO_ENCODE_Q_SIZE, AUDIO_ENCODE_Q_MSGLEN);
	if(!p_mixer_info->p_encode_event)
	{
		free(p_mixer_info);
		return NULL;
	
	} 

	if ((p_mixer_info->fd_record = open(pszName, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) 
        {
		msgQDelete(p_mixer_info->p_encode_event);
		free(p_mixer_info);
                fprintf(stderr, "Error open: [%s] \r\n", pszName);  
                return NULL; 
        } 

	p_mixer_info->p_audio_input_fifo = block_FifoNew();
        if(!p_mixer_info->p_audio_input_fifo) 
	{
		close(p_mixer_info->fd_record );
		msgQDelete(p_mixer_info->p_encode_event);
                free(p_mixer_info);
                printf("No memory \r\n");
                return NULL;
        }

	p_mixer_info->private_data = NULL;

	oid_clone(&p_mixer_info->p_encode_thread,  audio_encode_thread , p_mixer_info, OID_THREAD_PRIORITY_INPUT);	

	audio_send_begin_encode_q(p_mixer_info->p_encode_event);

	return p_mixer_info;
}


int Mixer_RecordStop(void * param)
{
	if(param == NULL) return 0; 
	MIXER_RECORD_INFO * p_mixer_info = (MIXER_RECORD_INFO *)param;
	if(p_mixer_info->encode_state == ENCODE_STATE_BEGIN) {
        	p_mixer_info->encode_state = ENCODE_STATE_STOP;
		return 1;	
	}

	return 0;
}


int Mixer_IsRecording(void * param)
{

   	if(param == NULL) return 0;
   
	MIXER_RECORD_INFO * p_mixer_info = (MIXER_RECORD_INFO *)param;

   	if(p_mixer_info->encode_state == ENCODE_STATE_BEGIN)  return 1;
   	else return 0;

}

int Mixer_SetMute(void * param, int bMute)
{

	if(param == NULL) return 0;

        MIXER_RECORD_INFO * p_mixer_info = (MIXER_RECORD_INFO *)param;

	if(p_mixer_info->encode_state == ENCODE_STATE_BEGIN) {
		record_set_mute_state(bMute);	
	}

}

//volume is 0 - 16 , max 16
unsigned int Mixer_MainVolumeCtrl(int volume)
{

	mixer_set_record_volume(volume);

}


