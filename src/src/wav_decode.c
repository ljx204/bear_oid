#include "oid_common.h"
#include "wav_decode.h"

int WAV_P_CheckValid(WAVContainer_t *container) 
{ 
    if (container->header.magic != WAV_RIFF || 
        container->header.type != WAV_WAVE || 
        container->format.magic != WAV_FMT || 
        container->format.fmt_size != LE_INT(16) || \
	 (container->format.channels != LE_SHORT(1) && container->format.channels != LE_SHORT(2)) || container->chunk.type != WAV_DATA) 
    {
        fprintf(stderr, "non standard wav file. \r\n"); 
        return -1; 
    } 
 
    return 0; 
} 

int WAV_ReadHeader(int fd, WAVContainer_t *container) 
{ 
    	assert((fd >=0) && container); 
 
	if (read(fd,&container->header,sizeof(container->header))!=sizeof(container->header) || \
		read(fd,&container->format,sizeof(container->format))!=sizeof(container->format) || \
		read(fd,&container->chunk,sizeof(container->chunk))!=sizeof(container->chunk))
	{
 
          	 fprintf(stderr, "Error WAV_ReadHeader \r\n"); 
       		 return -1; 
        } 

	if(container->chunk.type == WAV_FACT) {
		if(lseek(fd,container->chunk.length ,SEEK_CUR) < 0) {
			return -1;
		}
		if(read(fd,&container->chunk,sizeof(container->chunk))!=sizeof(container->chunk))
		{
			return -1;
		}
	}
 
    	if (WAV_P_CheckValid(container) < 0) 
        	return -1; 
 
       return 0; 
}

int wav_start(void* param)
{

	WAVContainer_t  wav;
	PLAYSOUND_VAR * p_sound = (PLAYSOUND_VAR *)param;
	
	if(WAV_ReadHeader(p_sound->fd, &wav) < 0) {
		printf("It is not a wav file \r\n");
		return 0;
	}

	p_sound->info.channels = wav.format.channels;
	p_sound->info.sample_rate = wav.format.sample_rate;
#if 1	
	oid_clone(&p_sound->p_dac_thread, \
                 audio_dac_thread , p_sound, OID_THREAD_PRIORITY_OUTPUT);
#endif
	audio_send_next_frame_q(p_sound->p_decode_event);
//	p_sound->play_state = AUDIO_PLAY_STATE;
	Sound_set_state(p_sound, AUDIO_PLAY_STATE);
	return 1;
}

static ssize_t wav_read_data(int fd, void *buf, size_t count) 
{ 
      ssize_t result = 0, res; 
   
      while (count > 0) { 
          if ((res = read(fd, buf, count)) == 0) 
              break; 
          if (res < 0) 
              return result > 0 ? result : res; 
          count -= res; 
          result += res; 
          buf = (char *)buf + res; 
      } 
      return result; 
} 


int   wav_decode_a_frame(void  * param)
{
	block_t * p_block;
	PLAYSOUND_VAR * p_sound = (PLAYSOUND_VAR *)param;
	int  buffer_size, ret;
	

	buffer_size = p_sound->info.sample_rate*p_sound->info.channels*2;
	printf("wav_decode_a_frame %d, %d, %d \r\n", buffer_size, p_sound->info.sample_rate, p_sound->info.channels);
	p_block = block_Alloc(buffer_size); 
		
	if(p_block) 
	{
		ret = wav_read_data(p_sound->fd, p_block->p_buffer, buffer_size); 
		if(ret > 0) {
			p_block->i_buffer = ret;
			block_FifoPut(p_sound->p_audio_output_fifo, p_block);

			if(block_FifoCount(p_sound->p_audio_output_fifo) < 5) 
			{
			      audio_send_next_frame_q(p_sound->p_decode_event);
			}
			return 1;
		}
		else
		{
			block_Release(p_block);
		//	p_sound->play_state = AUDIO_WATI_PLAY_STOP;
			Sound_set_state(p_sound, AUDIO_WAIT_PLAY_STOP);
		}
	}
	
	return 0;
}


void  wav_end_play(void  * param)
{
        PLAYSOUND_VAR * p_sound = (PLAYSOUND_VAR *)param;
//-----------------------------------------------------------------------       
	 Sound_set_state(p_sound, AUDIO_PLAY_STOP);

        while(block_FifoCount(p_sound->p_audio_output_fifo) != 0)
                {
                        msleep(100);
                }
//-------------------------------------------------------------

	audio_dac_close(p_sound);
	oid_cancel(p_sound->p_dac_thread);
        oid_join(p_sound->p_dac_thread, NULL );
	printf("wav_end_play... ______________________________________________________________________\r\n");
	p_sound->play_state = AUDIO_PLAY_STOP;
	app_end_play_sound(&param);
}



















 
