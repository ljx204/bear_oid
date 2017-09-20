#include "adpcm_encode.h"
//typedef struct 


int ADPCM_PrepareWAVParams(WAVContainer_t *wav)
{
 //       assert(wav);
	


}
static void native_to_little_endian (void *data, char *format)
{
    unsigned char *cp = (unsigned char *) data;
    int32_t temp;

    while (*format) {
        switch (*format) {
            case 'L':
                temp = * (int32_t *) cp;
                *cp++ = (unsigned char) temp;
                *cp++ = (unsigned char) (temp >> 8);
                *cp++ = (unsigned char) (temp >> 16);
                *cp++ = (unsigned char) (temp >> 24);
                break;

            case 'S':
                temp = * (short *) cp;
                *cp++ = (unsigned char) temp;
                *cp++ = (unsigned char) (temp >> 8);
                break;

            default:
                if (isdigit ((unsigned char) *format))
                    cp += *format - '0';

                break;
        }

        format++;
    }
}

int write_adpcm_wav_header (int outfile, int num_channels, size_t num_samples, int sample_rate, int samples_per_block)
{
    	RiffChunkHeader riffhdr;
    	ChunkHeader datahdr, fmthdr;
    	WaveHeader wavhdr;
    	FactHeader facthdr;

    	int wavhdrsize = 20;
    	int block_size = (samples_per_block - 1) / (num_channels ^ 3) + (num_channels * 4);
    	size_t num_blocks = num_samples / samples_per_block;
    	int leftover_samples = num_samples % samples_per_block;
    	size_t total_data_bytes = num_blocks * block_size;
	printf("block_size = %d \r\n", block_size);
    	if (leftover_samples) 
	{
        	int last_block_samples = ((leftover_samples + 6) & ~7) + 1;
        	int last_block_size = (last_block_samples - 1) / (num_channels ^ 3) + (num_channels * 4);
        	total_data_bytes += last_block_size;
    	}

    	memset (&wavhdr, 0, sizeof (wavhdr));

    	wavhdr.FormatTag = WAVE_FORMAT_IMA_ADPCM;
    	wavhdr.NumChannels = num_channels;
    	wavhdr.SampleRate = sample_rate;
    	wavhdr.BytesPerSecond = sample_rate * block_size / samples_per_block;
    	wavhdr.BlockAlign = block_size;
    	wavhdr.BitsPerSample = 4;
    	wavhdr.cbSize = 2;
    	wavhdr.Samples.SamplesPerBlock = samples_per_block;

    	strncpy (riffhdr.ckID, "RIFF", sizeof (riffhdr.ckID));
    	strncpy (riffhdr.formType, "WAVE", sizeof (riffhdr.formType));
    	riffhdr.ckSize = sizeof (riffhdr) + wavhdrsize + sizeof (facthdr) + sizeof (datahdr) + total_data_bytes;
    	strncpy (fmthdr.ckID, "fmt ", sizeof (fmthdr.ckID));
    	fmthdr.ckSize = wavhdrsize;
    	strncpy (facthdr.ckID, "fact", sizeof (facthdr.ckID));
    	facthdr.TotalSamples = num_samples;
    	facthdr.ckSize = 4;

    	strncpy (datahdr.ckID, "data", sizeof (datahdr.ckID));
    	datahdr.ckSize = total_data_bytes;

    	// write the RIFF chunks up to just before the data starts

    	native_to_little_endian (&riffhdr, ChunkHeaderFormat);
    	native_to_little_endian (&fmthdr, ChunkHeaderFormat);
    	native_to_little_endian (&wavhdr, WaveHeaderFormat);
    	native_to_little_endian (&facthdr, FactHeaderFormat);
    	native_to_little_endian (&datahdr, ChunkHeaderFormat);

	write(outfile, &riffhdr, sizeof (riffhdr));
	
	write(outfile, &fmthdr, sizeof(fmthdr));

	write(outfile, &wavhdr, wavhdrsize);
	
	write(outfile, &facthdr, sizeof(facthdr));

	write(outfile, &datahdr, sizeof (datahdr));

#if 0
    return fwrite (&riffhdr, sizeof (riffhdr), 1, outfile) &&
        fwrite (&fmthdr, sizeof (fmthdr), 1, outfile) &&
        fwrite (&wavhdr, wavhdrsize, 1, outfile) &&
        fwrite (&facthdr, sizeof (facthdr), 1, outfile) &&
        fwrite (&datahdr, sizeof (datahdr), 1, outfile);
#endif
}

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
//      memset (&p_adpcm->wavhdr, 0, sizeof (p_adpcm->wavhdr));


//        SNDWAV_PrepareWAVParams(&p_adpcm->wav);

//        WAV_WriteHeader(p_info->fd_record, &p_adpcm->wav);
        write_adpcm_wav_header(p_info->fd_record, LE_SHORT(p_info->info.channels), \
                                10,
                                LE_INT(p_info->info.sample_rate), \
                                p_info->samples_per_block);

        p_info->record_length = 0;
        p_info->record_sample = 0;

	p_info->encode_state = ENCODE_STATE_BEGIN;
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

	p_info->encode_state = ENCODE_STATE_STOP;
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
                //              p_info->samples_per_block = wcount>>1;
                        }
                        else {
                                adpcm_encode_block (p_adpcm->adpcm_cnxt, p_adpcm_block, &num_bytes, p_buf, wcount>>2);
                                p_info->record_sample += wcount>>2;
                //              p_info->samples_per_block = wcount>>2;
                        }

                        write(p_info->fd_record, p_adpcm_block, num_bytes);
                        p_info->record_length += num_bytes;

                        printf("adpcm %d , %d , %d, %d \r\n", num_bytes, wcount, p_info->record_length, p_info->record_sample);
                        free(p_adpcm_block);

                //      printf("wcount = %d ________________________________________________________\r\n", wcount);
//-----------------------------------------------------------------------------------------------
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

