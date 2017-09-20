#include "oid_common.h"

#define WAV_BUFFER_SIZE		8192
int LAME_MP3_encode_begin(MIXER_RECORD_INFO * p_info)
{
	LAME_MP3_INFO * p_lame_mp3;
        p_info->private_data = (LAME_MP3_INFO *)malloc(sizeof( LAME_MP3_INFO ));
        if(!p_info->private_data)
        {
                printf("LAME MP3  encode begin no memory \r\n");
                return 0;
        }

	
	p_info->samples_per_block = WAV_BUFFER_SIZE;
	
        p_lame_mp3 = (LAME_MP3_INFO *)p_info->private_data;

	p_lame_mp3->handle =  lame_init();  
  
    	lame_set_in_samplerate(p_lame_mp3->handle, p_info->info.sample_rate);  
    	
	lame_set_out_samplerate(p_lame_mp3->handle, p_info->info.sample_rate);  
    	
	lame_set_num_channels(p_lame_mp3->handle, p_info->info.channels );  
    	
	if(p_info->info.channels == 1)
		lame_set_mode(p_lame_mp3->handle, MONO);  
	else
		lame_set_mode(p_lame_mp3->handle, STEREO);	//STEREO
    	
    	
	lame_set_VBR(p_lame_mp3->handle, vbr_default);  
    	
	lame_set_brate(p_lame_mp3->handle, (p_info->info.bit_rate/8)/1000);  
    	
	lame_init_params(p_lame_mp3->handle); 
	
	p_info->record_length = 0;
        p_info->record_sample = 0;
	
	p_info->encode_state = ENCODE_STATE_BEGIN;
	return 1;

}

int LAME_MP3_encode_stop(MIXER_RECORD_INFO * p_info)
{
	LAME_MP3_INFO * p_lame_mp3;

        p_lame_mp3 = (LAME_MP3_INFO *)p_info->private_data;
        
        close(p_info->fd_record);

	lame_close(p_lame_mp3->handle);

        free(p_info->private_data);

	p_info->encode_state = ENCODE_STATE_STOP;

        return 1;


}


int LAME_MP3_encode_loop(MIXER_RECORD_INFO * p_info)
{
	LAME_MP3_INFO * p_lame_mp3 = (LAME_MP3_INFO *)p_info->private_data;
        block_t *p_block;
        if(p_info != NULL) {
                p_block = block_FifoGet(p_info->p_audio_input_fifo);
                
		if(p_block > 0)
                {
                        int wcount = p_block->i_buffer;
                        
			int16_t *p_buf = p_block->p_buffer;
                        
			int num_channels = p_info->info.channels;
                        
			int num_bytes;
			
			void * mp3_buffer = malloc(wcount>>1);

			if(num_channels == 1) 
			{
				num_bytes = lame_encode_buffer(p_lame_mp3->handle, p_buf, NULL, wcount>>1, mp3_buffer, wcount>>1); //WAV_BUFFER_SIZE); 
				//num_bytes = lame_encode_buffer_interleaved(p_lame_mp3->handle, p_buf, wcount>>1, mp3_buffer, WAV_BUFFER_SIZE);

			}
			else
			{
				//num_bytes = lame_encode_buffer(p_lame_mp3->handle, p_buf, NULL, wcount>>2, mp3_buffer, wcount>>1); //WAV_BUFFER_SIZE);
				num_bytes = lame_encode_buffer_interleaved(p_lame_mp3->handle, p_buf, wcount>>2, mp3_buffer, wcount>>1); //WAV_BUFFER_SIZE);
			}
			write(p_info->fd_record, mp3_buffer, num_bytes);
                        p_info->record_length += num_bytes;

                        printf("lame mp3  %d , %d , %d, %d \r\n", num_bytes, wcount, p_info->record_length, p_info->record_sample);
                     
			free(mp3_buffer);
//____________________________________________test ----------------------------------
//                        if(p_info->record_length >= 1024*300) {
//                                record_stop();
//                                audio_send_stop_encode(p_info->p_encode_event);
//                                return;
//                        }
			if(p_info->encode_state == ENCODE_STATE_STOP) {
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

