#include "oid_common.h"
#include "mp3_decode.h"

#define MAX_BUFFER_SIZE		8192
#define MAX_HEAD_LENGTH		50*8192

int MP3_HeaderInfo(int fd,  int * channel, int *samplerate)
{
	//struct mad_header head;
	struct mad_stream 		Stream;
	struct mad_frame 		Frame;
	struct mad_synth 		Synth;
	int length;
	int err, result;
	mad_stream_init(&Stream);
	mad_frame_init(&Frame);
	mad_synth_init(&Synth);

	char * p_temp = (char *)malloc(MAX_HEAD_LENGTH);
	if(p_temp == NULL) return 0;

	length = read(fd, p_temp, MAX_HEAD_LENGTH);
	
	mad_stream_buffer(&Stream, p_temp, length);

	result = 1;
	while(1) 
	{
		if(Stream.buffer == NULL || Stream.error == MAD_ERROR_BUFLEN) 
		{
			result = 0;
			break;
		}
		
		if (err = mad_frame_decode(&Frame, &Stream)) 
		{
			if (MAD_RECOVERABLE(Stream.error)) {
				continue;
			} else {
				if (Stream.error == MAD_ERROR_BUFLEN) 
				{
					continue; /* buffer解码光了, 需要继续填充了 */
				} else if (Stream.error == MAD_ERROR_LOSTSYNC) {
					int tagsize;
					tagsize = id3_tag_query(Stream.this_frame, Stream.bufend - Stream.this_frame);
					if (tagsize > 0) {
						mad_stream_skip(&Stream, tagsize);
					}
					continue;
				} 
				else 
				{
					result = 0;
					break;
				}
			}
		}
		else
		{
			printf("OK !!!!!!!!!!!!!!!!!!!!!! \r\n");
			result = 1;
			break;
		}
	}

	if(result) 
	{
		printf("channel1111 = %d \r\n", Frame.header.mode);

		if(Frame.header.mode == MAD_MODE_SINGLE_CHANNEL)
			*channel = 1;
		else
			*channel = 2;

		*samplerate = Frame.header.samplerate;
		mad_synth_finish(&Synth);
		mad_frame_finish(&Frame);
		mad_stream_finish(&Stream);
		free(p_temp);
		lseek(fd, 0, SEEK_SET);

		return 1;
	}	

	mad_synth_finish(&Synth);
	mad_frame_finish(&Frame);
	mad_stream_finish(&Stream);
	free(p_temp);
	lseek(fd, 0, SEEK_SET);
	return 0;

}

typedef struct mp3_decode 
{
	struct mad_stream               Stream;
        struct mad_frame                Frame;
        struct mad_synth                Synth;
	unsigned char  *		buffer;
}MP3_DECODE;

int   mp3_start(void* param)
{
        
	PLAYSOUND_VAR * p_sound = (PLAYSOUND_VAR *)param;
	
	int  channel, samplerate;


//        if(WAV_ReadHeader(p_sound->fd, &wav) < 0) {
//                printf("It is not a wav file \r\n");
//                return 0;
//        }

	if(!MP3_HeaderInfo(p_sound->fd, &channel, &samplerate))
	{
		printf("It is not a mp3 \r\n");
		return 0;

	}

        p_sound->info.channels = channel;
        p_sound->info.sample_rate = samplerate;
	
	p_sound->p_audio_decode->param = (MP3_DECODE *)malloc(sizeof(MP3_DECODE));
	if(p_sound->p_audio_decode->param == NULL) 
	{
		printf("No memory \r\n");
		return 0;
	}

	MP3_DECODE * p_decode =(MP3_DECODE *)(p_sound->p_audio_decode->param);

	mad_stream_init(&p_decode->Stream);
        mad_frame_init(&p_decode->Frame);
        mad_synth_init(&p_decode->Synth);
	p_decode->buffer = (unsigned char *)malloc(MAX_BUFFER_SIZE);

	if(p_decode->buffer == NULL) {
		free(p_decode);
		return 0;
	}


        oid_clone(&p_sound->p_dac_thread, \
                 audio_dac_thread , p_sound, OID_THREAD_PRIORITY_OUTPUT);

        audio_send_next_frame_q(p_sound->p_decode_event);
 //       p_sound->play_state = AUDIO_PLAY_STATE;
	Sound_set_state(p_sound, AUDIO_PLAY_STATE);
        return 1;


}

/*
 * The following utility routine performs simple rounding, clipping, and
 * scaling of MAD's high-resolution samples down to 16 bits. It does not
 * perform any dithering or noise shaping, which would be recommended to
 * obtain any exceptional audio quality. It is therefore not recommended to
 * use this routine if high-quality output is desired.
 */

static inline signed int mad_scale(mad_fixed_t sample)
{
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

int   mp3_decode_a_frame(void  * param)
{
	int  i, err;

	int  buffer_size;
	block_t * p_block;

	unsigned char	*OutputPtr;

	PLAYSOUND_VAR * p_sound = (PLAYSOUND_VAR *)param;

	MP3_DECODE * p_decode = (MP3_DECODE *)p_sound->p_audio_decode->param;
	
	struct mad_stream * 	p_Stream;
	struct mad_frame  *	p_Frame;
	struct mad_synth  *	p_Synth;

	p_Stream = &p_decode->Stream;
	p_Frame = &p_decode->Frame;
	p_Synth = &p_decode->Synth;

	do {
		/* 如果缓冲区空了或不足一帧数据, 就向缓冲区填充数据 */
		if(p_Stream->buffer == NULL || p_Stream->error == MAD_ERROR_BUFLEN) {
			size_t 			BufferSize;		/* 缓冲区大小 */
			size_t			Remaining;		/* 帧剩余数据 */
			unsigned char	*BufferStart;	/* 头指针 */

			if (p_Stream->next_frame != NULL) {

				/* 把剩余没解码完的数据补充到这次的缓冲区中 */
				Remaining = p_Stream->bufend - p_Stream->next_frame;
				memmove(p_decode->buffer, p_Stream->next_frame, Remaining);
				BufferStart = p_decode->buffer + Remaining;
				BufferSize = MAX_BUFFER_SIZE - Remaining;
			} else {
				/* 设置了缓冲区地址, 但还没有填充数据 */
				BufferSize = MAX_BUFFER_SIZE;
				BufferStart = p_decode->buffer;
				Remaining = 0;
			}

			/* 从文件中读取数据并填充缓冲区 */
			BufferSize = read(p_sound->fd, BufferStart, BufferSize);
			if (BufferSize <= 0) {
				printf("文件读取失败\n");
			//	p_sound->play_state = AUDIO_WATI_PLAY_STOP;
				Sound_set_state(p_sound, AUDIO_WAIT_PLAY_STOP);
				break;
				//exit(-1);
			}

			mad_stream_buffer(p_Stream, p_decode->buffer, BufferSize + Remaining);
			p_Stream->error = 0;
		}

		if (err = mad_frame_decode(p_Frame, p_Stream)) {

			if (MAD_RECOVERABLE(p_Stream->error)) {
				continue;
			} else {
				if (p_Stream->error == MAD_ERROR_BUFLEN) {
					continue; /* buffer解码光了, 需要继续填充了 */
				} else if (p_Stream->error == MAD_ERROR_LOSTSYNC) {
					int tagsize;
					tagsize = id3_tag_query(p_Stream->this_frame, p_Stream->bufend - p_Stream->this_frame);
					if (tagsize > 0) {
						mad_stream_skip(p_Stream, tagsize);
					}
					continue;
				} else {
					printf("严重错误，停止解码\n");
					break;
				}
			}
		}

		mad_synth_frame(p_Synth, p_Frame);
		
		buffer_size = p_Synth->pcm.length*2;
		
		if(p_sound->info.channels == 2) {
			buffer_size *= 2;
		}
		p_block = block_Alloc(buffer_size);

		OutputPtr = p_block->p_buffer;
		#if 1
		/* 解码后的音频数据转换成16位的数据 */
		for (i = 0; i < p_Synth->pcm.length; i++) {
			//unsigned short Sample;
			signed int Sample;
			Sample = mad_scale(p_Synth->pcm.samples[0][i]);
			//Sample = MadFixedToUshort(Synth.pcm.samples[0][i]);
			*(OutputPtr++) = (Sample & 0xff);
			*(OutputPtr++) = (Sample >> 8);
			//*(OutputPtr++) = (Sample >> 16);
			//*(OutputPtr++) = (Sample >> 24);

			//if (MAD_NCHANNELS(&Frame.header) == 2) {
			if(p_sound->info.channels == 2) {
				Sample = mad_scale(p_Synth->pcm.samples[1][i]);
				//Sample = MadFixedToUshort(Synth.pcm.samples[1][i]);
				*(OutputPtr++) = (Sample & 0xff);
				*(OutputPtr++) = (Sample >> 8);
				//*(OutputPtr++) = (Sample >> 16);
				//*(OutputPtr++) = (Sample >> 24);
			}

		}

		block_FifoPut(p_sound->p_audio_output_fifo, p_block);

		if(block_FifoCount(p_sound->p_audio_output_fifo) >= 10)
                {
		//	printf("wait dac ....... \r\n");
			break;
                         //audio_send_next_frame_q(p_sound->p_decode_event);
                }

		#endif
	} while(1);

}


void  mp3_end_play(void * param)
{

	PLAYSOUND_VAR * p_sound = (PLAYSOUND_VAR *)param;
	
	MP3_DECODE * p_decode = (MP3_DECODE * )(p_sound->p_audio_decode->param);

	mad_synth_finish(&p_decode->Synth);
        mad_frame_finish(&p_decode->Frame);
        mad_stream_finish(&p_decode->Stream);	
	
	free(p_decode->buffer);

	free(p_decode);
	//----------------------------
	//p_sound->play_state = AUDIO_PLAY_STOP;
	//printf("sound mp3_end_play 11 \r\n");

	Sound_set_state(p_sound, AUDIO_PLAY_STOP);

	while(block_FifoCount(p_sound->p_audio_output_fifo) != 0)
	{
		msleep(100);
	}

	//msleep(100);
	//printf("sound mp3_end_play 22 \r\n");
	//-------------------------------
	audio_dac_close(p_sound);

	//printf("sound mp3_end_play 33 \r\n");

        oid_cancel(p_sound->p_dac_thread);
        oid_join(p_sound->p_dac_thread, NULL );
        //close(p_sound->fd);   
        //app_end_play_sound(p_sound);
        printf("MP3_end_play... ______________________________________________________________________\r\n");
        p_sound->play_state = AUDIO_PLAY_STOP;
        app_end_play_sound(&param);
}

