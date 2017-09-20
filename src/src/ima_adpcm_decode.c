#include "oid_common.h"

typedef struct ima_adpcm_info {
	int channels;
	int sample_rate;
	int BlockAlign;
	int num_samples;

}IMA_ADPCM_INFO;

int ADPCM_Header_reader(int fd , IMA_ADPCM_INFO * p_adpcm)
{
	RiffChunkHeader riff_chunk_header;
        ChunkHeader chunk_header;
        WaveHeader WaveHeader;

	int fact_samples;
	size_t num_samples = 0;

	read(fd, &riff_chunk_header, sizeof(riff_chunk_header) );

	while(1) {

		if(read(fd, &chunk_header, sizeof(chunk_header)) != sizeof(chunk_header))
			return 0;

		if (!strncmp (chunk_header.ckID, "fmt ", 4)) {
			read(fd, &WaveHeader, chunk_header.ckSize);
		}
		else if (!strncmp (chunk_header.ckID, "fact", 4)) 
		{
			if (chunk_header.ckSize < 4 || read (fd, &fact_samples, sizeof (fact_samples)) != sizeof (fact_samples) ) {
                		//	fprintf (stderr, "\"%s\" is not a valid .WAV file!\n", infilename);
                			return 0;
            		}

            		if (chunk_header.ckSize > 4) {
                			int bytes_to_skip = chunk_header.ckSize - 4;
                			char dummy;

                			while (bytes_to_skip--)
                    			if (read(fd, &dummy, 1) != 1) 
					{
                        			//fprintf (stderr, "\"%s\" is not a valid .WAV file!\n", infilename);
                        			return 0;
                    			}
            		}
		}

		else if (!strncmp (chunk_header.ckID, "data", 4)) {
			
			break;
		}
		else {
			return 0;
		}
	}
	
	{
		int complete_blocks = chunk_header.ckSize / WaveHeader.BlockAlign;
                int leftover_bytes = chunk_header.ckSize % WaveHeader.BlockAlign;
                int samples_last_block;

                num_samples = complete_blocks * WaveHeader.Samples.SamplesPerBlock;

                if (leftover_bytes) {
                    if (leftover_bytes % (WaveHeader.NumChannels * 4)) {
                        fprintf (stderr, " is not a valid .WAV file!\n");
                        return 0;
                    }
                    //if (verbosity > 0) fprintf (stderr, "data chunk has %d bytes left over for final ADPCM block\n", leftover_bytes);
                    samples_last_block = (leftover_bytes - (WaveHeader.NumChannels * 4)) * (WaveHeader.NumChannels ^ 3) + 1;
                    num_samples += samples_last_block;
                }
                else
                    samples_last_block = WaveHeader.Samples.SamplesPerBlock;

                if (fact_samples) {
                    if (fact_samples < num_samples && fact_samples > num_samples - samples_last_block) {
                      //  if (verbosity > 0) fprintf (stderr, "total samples reduced %lu by FACT chunk\n", (unsigned long) (num_samples - fact_samples));
                        num_samples = fact_samples;
                    }
                    else if (WaveHeader.NumChannels == 2 && (fact_samples >>= 1) < num_samples && fact_samples > num_samples - samples_last_block) {
                       // if (verbosity > 0) fprintf (stderr, "num samples reduced %lu by [incorrect] FACT chunk\n", (unsigned long) (num_samples - fact_samples));
                        num_samples = fact_samples;
                    }
                }

		p_adpcm->channels = WaveHeader.NumChannels;
		p_adpcm->sample_rate = WaveHeader.SampleRate;
		p_adpcm->BlockAlign = WaveHeader.BlockAlign;
		p_adpcm->num_samples = num_samples;


	}

	return 1;
	


}

int ima_adpcm_start(void* param)
{
//	printf("ima_adpcm_start------------------ \r\n");

	
	PLAYSOUND_VAR * p_sound = (PLAYSOUND_VAR *)param;
	IMA_ADPCM_INFO adpcm_wav;

	if(!ADPCM_Header_reader(p_sound->fd, &adpcm_wav)) return 0;
	
	printf("%d, %d, %d, %d \r\n", adpcm_wav.channels, adpcm_wav.sample_rate, adpcm_wav.BlockAlign, adpcm_wav.num_samples);

	p_sound->info.channels = adpcm_wav.channels;
        p_sound->info.sample_rate = adpcm_wav.sample_rate;

	p_sound->info.BlockAlign = adpcm_wav.BlockAlign;
        p_sound->info.num_samples = adpcm_wav.num_samples;


        oid_clone(&p_sound->p_dac_thread, audio_dac_thread , p_sound, OID_THREAD_PRIORITY_OUTPUT);

        audio_send_next_frame_q(p_sound->p_decode_event);
      //  p_sound->play_state = AUDIO_PLAY_STATE;
	Sound_set_state(p_sound, AUDIO_PLAY_STATE); 
       return 1;

}

static ssize_t adpcm_read_data(int fd, void *buf, size_t count)
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

int  ima_adpcm_decode_a_frame(void  * param)
{
//	printf("ima_adpcm_decode_a_frame ------------------ \r\n");
	block_t * p_block;
        PLAYSOUND_VAR * p_sound = (PLAYSOUND_VAR *)param;
        int  buffer_size, ret;

	int  samples_per_block = ( p_sound->info.BlockAlign -  p_sound->info.channels * 4) * ( p_sound->info.channels  ^ 3) + 1;
	    
	buffer_size = samples_per_block * p_sound->info.channels * 2;
	

	void * p_adpcm_buf = malloc(p_sound->info.BlockAlign);
	if(p_adpcm_buf == NULL) return 0;


   
	p_block = block_Alloc(buffer_size);

        if(p_block)
        {
                ret = adpcm_read_data(p_sound->fd, p_adpcm_buf, p_sound->info.BlockAlign);
        
	        if(ret > 0) 
		{
        		if (adpcm_decode_block (p_block->p_buffer, p_adpcm_buf, p_sound->info.BlockAlign, p_sound->info.channels) != samples_per_block) 
			{
				free(p_adpcm_buf);
            			fprintf (stderr, "adpcm_decode_block() did not return expected value!\n");
            			return 0;
        		}

	                block_FifoPut(p_sound->p_audio_output_fifo, p_block);

                        if(block_FifoCount(p_sound->p_audio_output_fifo) < 5)
                        {
                              audio_send_next_frame_q(p_sound->p_decode_event);
                        }
			free(p_adpcm_buf);
                        return 1;
                }
                else
                {
                        block_Release(p_block);
                       // p_sound->play_state = AUDIO_WATI_PLAY_STOP;
			Sound_set_state(p_sound, AUDIO_WAIT_PLAY_STOP);
                }
        }

	free(p_adpcm_buf);
        return 0;


}


void  ima_adpcm_end_play(void  * param)
{
        PLAYSOUND_VAR * p_sound = (PLAYSOUND_VAR *)param;

	 Sound_set_state(p_sound, AUDIO_PLAY_STOP);

        while(block_FifoCount(p_sound->p_audio_output_fifo) != 0)
                {
                        msleep(100);
                }


        audio_dac_close(p_sound);
        oid_cancel(p_sound->p_dac_thread);
        oid_join(p_sound->p_dac_thread, NULL );
        
	printf("adpcm_end_play... ______________________________________________________________________\r\n");
        p_sound->play_state = AUDIO_PLAY_STOP;
        app_end_play_sound(&param);
}

