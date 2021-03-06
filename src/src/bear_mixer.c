#include "oid_common.h"


typedef struct voice_value{
	snd_mixer_t *mixer;
    	snd_mixer_elem_t *master_element;
	snd_mixer_elem_t *capture_element;

	int  volume_left;
	int  voulme_right;
	int  mute;

	int  record_mix_min;
	int  record_mix_max;

}VOICE_CTRL;


static VOICE_CTRL * p_voice_ctrl;

int mixer_init(void)
{
	int  found_capture = 0;

	long min, max;

	p_voice_ctrl = (VOICE_CTRL *)malloc(sizeof(VOICE_CTRL));
	
	if(p_voice_ctrl == NULL) return 0;
   
	 snd_mixer_open(&p_voice_ctrl->mixer, 0);
    
	snd_mixer_attach(p_voice_ctrl->mixer, "default");
    
	snd_mixer_selem_register(p_voice_ctrl->mixer, NULL, NULL);
    
	snd_mixer_load(p_voice_ctrl->mixer);  /* 取得第一個 element，也就是 Master */
    
	p_voice_ctrl->master_element = snd_mixer_first_elem(p_voice_ctrl->mixer);  /* 設定音量的範圍 0 ~ 100 */
    
	snd_mixer_selem_set_playback_volume_range(p_voice_ctrl->master_element, 0, 100);  /* 取得 Master 是否靜音 */
    
	snd_mixer_selem_get_playback_switch(p_voice_ctrl->master_element, 0, &p_voice_ctrl->mute);

	for(p_voice_ctrl->capture_element = snd_mixer_first_elem(p_voice_ctrl->mixer); p_voice_ctrl->capture_element; \
		p_voice_ctrl->capture_element = snd_mixer_elem_next(p_voice_ctrl->capture_element))
	{
		if (snd_mixer_elem_get_type(p_voice_ctrl->capture_element) == SND_MIXER_ELEM_SIMPLE && \
				snd_mixer_selem_is_active(p_voice_ctrl->capture_element))
        	{
            		if(!strcmp(snd_mixer_selem_get_name(p_voice_ctrl->capture_element), "Capture"))
            		{
				printf("capture ..... \r\n");
                		//snd_mixer_selem_get_playback_volume_range(elem, &minVolume, &maxVolume);
                		//snd_mixer_selem_set_playback_volume_all(elem, (long)50); // 设置音量为50
				snd_mixer_selem_get_capture_volume_range(p_voice_ctrl->capture_element, &min, &max);
				found_capture = 1;
				p_voice_ctrl->record_mix_min = min;
				p_voice_ctrl->record_mix_max = max;
				printf("min = %d , max = %d \r\n", min, max);
				break;
            		}

		}
	}
	
	if(!found_capture) p_voice_ctrl->capture_element = NULL;
}


void mixer_get_volume(long *left_value, long * right_value)
{
	snd_mixer_selem_get_playback_volume(p_voice_ctrl->master_element, SND_MIXER_SCHN_FRONT_LEFT, (long *)left_value);
    	snd_mixer_selem_get_playback_volume(p_voice_ctrl->master_element, SND_MIXER_SCHN_FRONT_RIGHT, (long *)right_value);  
}

void mixer_mute(void)
{
	int chn;

	for (chn=0;chn<=SND_MIXER_SCHN_LAST;chn++) 
	{      
      		snd_mixer_selem_set_playback_switch(p_voice_ctrl->master_element, chn, 0);  
    	}
}



void mixer_unmute(void)
{
	int chn;

	for (chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++) 
        {      
                snd_mixer_selem_set_playback_switch(p_voice_ctrl->master_element, chn, 1);  
        }  

}


//volume 0 -  99
void mixer_set_volume(int volume)
{
	snd_mixer_selem_set_playback_volume(p_voice_ctrl->master_element, SND_MIXER_SCHN_FRONT_LEFT, volume);  
    	snd_mixer_selem_set_playback_volume(p_voice_ctrl->master_element, SND_MIXER_SCHN_FRONT_RIGHT, volume);
}

void mixer_set_record_volume(int step)
{
	if(p_voice_ctrl->capture_element)
	{
		long volume;
		if(step >= 16) {
			volume = p_voice_ctrl->record_mix_max;
		}
		else
		{
			volume = p_voice_ctrl->record_mix_min + step * ((p_voice_ctrl->record_mix_max - p_voice_ctrl->record_mix_min) >> 4);
		}
		snd_mixer_selem_set_capture_switch_all(p_voice_ctrl->capture_element, 1); 
		snd_mixer_selem_set_capture_volume_all(p_voice_ctrl->capture_element, (long)volume); 
	}
}





