#include "oid_common.h"

#define DEFAULT_SAMPLE_LENGTH  16

int SNDWAV_PrepareWAVParams(WAVContainer_t *wav) 
{ 
        assert(wav); 
     
        uint16_t channels = wav->format.channels; //DEFAULT_CHANNELS; 
        uint16_t sample_rate = wav->format.sample_rate; //DEFAULT_SAMPLE_RATE; 
        uint16_t sample_length = DEFAULT_SAMPLE_LENGTH; 
     //   uint32_t duration_time = 0; //DEFAULT_DURATION_TIME; 
     
        /* Const */ 
        wav->header.magic = WAV_RIFF; 
        wav->header.type = WAV_WAVE; 
        wav->format.magic = WAV_FMT; 
        wav->format.fmt_size = LE_INT(16); 
     //   wav->format.format = LE_SHORT(WAV_FMT_PCM); 
        wav->chunk.type = WAV_DATA; 
     
        /* User definition */ 
       // wav->format.channels = LE_SHORT(channels); 
       // wav->format.sample_rate = LE_INT(sample_rate); 
         wav->format.sample_length = LE_SHORT(sample_length); 
     
        /* See format of wav file */ 
    wav->format.blocks_align = LE_SHORT(channels * sample_length / 8); 
    wav->format.bytes_p_second = LE_INT((uint16_t)(wav->format.blocks_align) * sample_rate); 
         
    //wav->chunk.length = LE_INT(duration_time * (uint32_t)(wav->format.bytes_p_second)); 
    wav->header.length = LE_INT((uint32_t)(wav->chunk.length) + \
            sizeof(wav->chunk) + sizeof(wav->format) + sizeof(wav->header) - 8); 
     
        return 0; 
} 


int WAV_WriteHeader(int fd, WAVContainer_t *container) 
{ 
    	assert((fd >=0) && container); 
     
    	if (WAV_P_CheckValid(container) < 0) 
        	return -1; 
 
	if ( write(fd, &container->header, sizeof(container->header)) != sizeof(container->header) \
		|| write(fd,&container->format,sizeof(container->format))!= sizeof(container->format) \
		|| write(fd,&container->chunk,sizeof(container->chunk)) != sizeof(container->chunk)) 
	{ 
        		fprintf(stderr, "Error WAV_WriteHeader \r\n"); 
        		return -1; 
    	} 
 
 
    return 0; 
}


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

	p_info->encode_state = ENCODE_STATE_BEGIN;
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

	p_info->encode_state = ENCODE_STATE_STOP;
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
//                        if(p_info->record_length >= 1024*1024) {
//                                record_stop();
//                                audio_send_stop_encode(p_info->p_encode_event);
//                                return;
//                        }
			if(p_info->encode_state == ENCODE_STATE_STOP) {
                                 record_stop();
                                 audio_send_stop_encode(p_info->p_encode_event);
                                 return;
                        }

//---------------------------------------------------------------------------------
                        audio_send_next_frame_encode_q(p_info->p_encode_event);
                }
        }

}


