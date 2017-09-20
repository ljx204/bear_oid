#include "oid_common.h"

AUDIO_TYPE audio_get_type(const char * file_ext_name) // except '.'
{
        char    temp[5];
        int     i;

        strcpy(temp,(const char *)file_ext_name);

        for (i=0;i<strlen(temp);i++) {
                temp[i] = toupper(temp[i]);
        }


        if(strcmp(temp, (const char *)"WAV")==0)
                return AUDIO_TYPE_WAV;

        if(strcmp(temp, (const char *)"MP3")==0)
                return AUDIO_TYPE_MP3;


        return AUDIO_TYPE_NONE;

}

AUDIO_TYPE audio_get_wav_format(int fd)
{
	RiffChunkHeader riff_chunk_header;
	ChunkHeader chunk_header;
    	WaveHeader WaveHeader;
	int format, bits_per_sample;

	if(read(fd, &riff_chunk_header, sizeof(riff_chunk_header) )!= sizeof(riff_chunk_header))
		return AUDIO_TYPE_NONE;

	if(strncmp (riff_chunk_header.ckID, "RIFF", 4) || strncmp (riff_chunk_header.formType, "WAVE", 4))
		return AUDIO_TYPE_NONE;

	
		if(read(fd, &chunk_header, sizeof(chunk_header)) != sizeof(chunk_header))
			return AUDIO_TYPE_NONE;
		if (!strncmp (chunk_header.ckID, "fmt ", 4)) 
		{
			if (chunk_header.ckSize < 16 || chunk_header.ckSize > sizeof (WaveHeader))
					return AUDIO_TYPE_NONE;
			if (read(fd, &WaveHeader, chunk_header.ckSize) != chunk_header.ckSize)
					return AUDIO_TYPE_NONE;

			if (WaveHeader.NumChannels < 1 || WaveHeader.NumChannels > 2) 
					return AUDIO_TYPE_NONE; 
			format = (WaveHeader.FormatTag == WAVE_FORMAT_EXTENSIBLE && chunk_header.ckSize == 40) ? \
					 WaveHeader.SubFormat : WaveHeader.FormatTag;
			bits_per_sample = (chunk_header.ckSize == 40 && WaveHeader.Samples.ValidBitsPerSample) ? \
					WaveHeader.Samples.ValidBitsPerSample : WaveHeader.BitsPerSample;

			if(format == WAVE_FORMAT_PCM) {
				if (bits_per_sample < 9 || bits_per_sample > 16)
                   				 return AUDIO_TYPE_NONE;

                		if (WaveHeader.BlockAlign != WaveHeader.NumChannels * 2)
                    				return AUDIO_TYPE_NONE;

				return AUDIO_TYPE_WAV;
			}
			else if(format == WAVE_FORMAT_IMA_ADPCM)
			{
				if (bits_per_sample != 4)
                   			return AUDIO_TYPE_NONE;

		                if (WaveHeader.Samples.SamplesPerBlock != (WaveHeader.BlockAlign - WaveHeader.NumChannels * 4) * (WaveHeader.NumChannels ^ 3) + 1) 
				{
					return AUDIO_TYPE_NONE;
                		}

				return AUDIO_TYPE_IMA_ADCPM;
			}
            	}
		return AUDIO_TYPE_NONE;
		

	 
		
	

	//lseek(fd, 0, SEEK_SET);

}

static void * sound_table[MAX_PLAYSOUND_NUMBER+1];
//static int   g_sound_index;
int Sound_play_init(void)
{
	int i;
	for(i = 1; i < MAX_PLAYSOUND_NUMBER+1; i++) {
		sound_table[i] = NULL;
	}
//	g_sound_index = 1;
	return 1;

}

int Sound_play(const char * pName)
{
        AUDIO_TYPE format;
        char * pstr;
        int  fd;
	int i;
        PLAYSOUND_VAR * p_sound_info;
#if 0
	i = 1;

	while(i < (MAX_PLAYSOUND_NUMBER+1))
	{
		if(sound_table[g_sound_index] == NULL) break;
		else {
			g_sound_index++;
			if(g_sound_index >= (MAX_PLAYSOUND_NUMBER+1))
			g_sound_index = 1;
		}
		i++
	}
#endif

#if 1
	for(i = 1; i < MAX_PLAYSOUND_NUMBER+1; i++ )
	{
		if(sound_table[i] == NULL)
		break;
	}
#endif
	if( i >= (MAX_PLAYSOUND_NUMBER+1)) {
		printf("Too many sound is playing \r\n");
		return 0;
	}

#if 0
	i = g_sound_index;
	g_sound_index++;
	
	if(g_sound_index >= (MAX_PLAYSOUND_NUMBER+1))
        	 g_sound_index = 1;
#endif

        p_sound_info = (PLAYSOUND_VAR *)malloc(sizeof(PLAYSOUND_VAR));
        if(p_sound_info == NULL) {
        //      printf("Error: No Memory \r\n");
                return 0;
        }



        format = AUDIO_TYPE_NONE;
        pstr = strrchr(pName,'.');

        if(pstr) {
                format = audio_get_type((const INT8S *)&pstr[1]);
        }
        if(format == AUDIO_TYPE_NONE) {
                free(p_sound_info);
                printf("Can not play the file \r\n");
                return 0;
        }
        p_sound_info->audio_format = format;

        fd = open(pName, O_RDONLY);

        if(fd < 0) {
                free(p_sound_info);
                return 0;
        }
        p_sound_info->fd = fd;
	
	if(format == AUDIO_TYPE_WAV) {
		format = audio_get_wav_format(fd);
		lseek(fd, 0, SEEK_SET);
		if(format == AUDIO_TYPE_NONE) 
		{
			close(fd);
                	free(p_sound_info);
                	printf("Can not play the file \r\n");
                	return 0;
        	}

	}

	p_sound_info->audio_format = format;

	p_sound_info->p_audio_decode =(Audio_decode_t *)malloc(sizeof(Audio_decode_t));

	memset(p_sound_info->p_audio_decode , 0 , sizeof(Audio_decode_t));

        if(p_sound_info->p_audio_decode == NULL) {
                close(fd);
                free(p_sound_info);
                return 0;
        }

        p_sound_info->p_audio_output_fifo = block_FifoNew();
        if(!p_sound_info->p_audio_output_fifo) {
                close(fd);
                free(p_sound_info->p_audio_decode);
                free(p_sound_info);
                return 0;

        }

        p_sound_info->p_decode_event = msgQCreate(AUDIO_DECODE_Q_SIZE, AUDIO_DECODE_Q_SIZE, AUDIO_DECODE_Q_MSGLEN);

        if(!p_sound_info->p_decode_event) {
                close(fd);
                free(p_sound_info->p_audio_decode);
                block_FifoRelease(p_sound_info->p_audio_output_fifo);
                free(p_sound_info);
                return 0;

        }
	
	oid_mutex_init(&p_sound_info->play_mutex);
        p_sound_info->play_state = AUDIO_PLAY_NULL;

        oid_clone(&p_sound_info->p_decode_thread,  audio_decode_thread , p_sound_info, OID_THREAD_PRIORITY_INPUT);

        msgQSend(p_sound_info->p_decode_event, (INT32U)MSG_AUD_PLAY, NULL, 0, MSG_PRI_NORMAL);
	
	sound_table[i] = p_sound_info;

	p_sound_info->play_sound_table_index = i;

	return i;
}

void Waiting_sound_play_stop(void * param)
{
	int sound_index;
        MSG_INFORMATION   * p_msg_info = (MSG_INFORMATION *)malloc(sizeof(MSG_INFORMATION));
        PLAYSOUND_VAR * p_sound = (PLAYSOUND_VAR *)param;

        msg_info_init(p_msg_info);

        audio_send_stop_decode(p_sound->p_decode_event, &p_msg_info);

        msg_info_wait(p_msg_info);

        msg_info_destroy(p_msg_info);

        free(p_msg_info);


        oid_cancel(p_sound->p_decode_thread);
        oid_join(p_sound->p_decode_thread, NULL );

        close(p_sound->fd);
        msgQDelete(p_sound->p_decode_event);
        free(p_sound->p_audio_decode);
        block_FifoRelease(p_sound->p_audio_output_fifo);
        sound_index = p_sound->play_sound_table_index;
	
	oid_mutex_destroy(&p_sound->play_mutex);
        free(p_sound);
                // Sound_play("/home/ken/oid_pen/alarm.wav");
        printf("free play sound ................... \r\n");

	sound_table[sound_index] = NULL;


}

int Sound_set_state(PLAYSOUND_VAR * p_sound, int state)
{
	oid_mutex_lock(&p_sound->play_mutex);
	p_sound->play_state = state;
	oid_mutex_unlock(&p_sound->play_mutex);
}

int Sound_stop(int fd)
{

	PLAYSOUND_VAR * p_sound_info;
	
	if(fd >= (MAX_PLAYSOUND_NUMBER+1)) return 0;

	if(!sound_table[fd]) return 0;

	p_sound_info = (PLAYSOUND_VAR *)sound_table[fd];

	if((p_sound_info->play_state == AUDIO_PLAY_STATE) || (p_sound_info->play_state == AUDIO_PLAY_PAUSE)) //AUDIO_PLAY_PAUSE,
		audio_send_stop_play(p_sound_info->p_decode_event);
	else
		return 0;
	//while(1) {
	//	if(sound_table[fd] != NULL ) msleep(10);
	//	else break;
	//}

	return 1;

}

int Sound_Stop_All(void)
{
	int i;
	int ret;
	for(i = 1; i < MAX_PLAYSOUND_NUMBER+1; i++) {
		if(sound_table[i] != NULL)
		{
			while(!(ret = Sound_stop(i))) msleep(10);
		}
	}
	return 1;

}
int Sound_Pause(int handle)
{
	PLAYSOUND_VAR * p_sound_info;
	if(handle < 1) return 0;
	if(handle >= (MAX_PLAYSOUND_NUMBER+1)) return 0;
	if(sound_table[handle] == NULL) return 0;

	p_sound_info = (PLAYSOUND_VAR *)sound_table[handle] ;

	if(p_sound_info->play_state == AUDIO_PLAY_STATE) {
		Sound_set_state(p_sound_info, AUDIO_PLAY_SET_PAUSE);
		//p_sound_info->play_state = AUDIO_PLAY_SET_PAUSE;

	}

}

int Sound_Resume(int handle)
{
	PLAYSOUND_VAR * p_sound_info;
	if(handle < 1) return 0;
	if(handle >= (MAX_PLAYSOUND_NUMBER+1)) return 0;
	if(sound_table[handle] == NULL) return 0;

	p_sound_info = (PLAYSOUND_VAR *)sound_table[handle] ;
	if(p_sound_info->play_state == AUDIO_PLAY_PAUSE) {
	//	p_sound_info->play_state = AUDIO_PLAY_SET_RESUME;
		Sound_set_state(p_sound_info, AUDIO_PLAY_SET_RESUME);
	}
}
#if 0
int Sound_Pause(int fd)
{

	PLAYSOUND_VAR * p_sound_info;
	
	if(fd >= (MAX_PLAYSOUND_NUMBER+1)) return 0;

	if(!sound_table[fd]) return 0;

	p_sound_info = (PLAYSOUND_VAR *)sound_table[fd];

	
	
	

}

int Sound_Resume(int fd)
{

	PLAYSOUND_VAR * p_sound_info;
	
	if(fd >= (MAX_PLAYSOUND_NUMBER+1)) return 0;

	if(!sound_table[fd]) return 0;

	p_sound_info = (PLAYSOUND_VAR *)sound_table[fd];

	
	
	

}
#endif



