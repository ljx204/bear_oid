#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>


snd_mixer_t * mixer;
snd_mixer_elem_t *pcm_element;
int set_volume(int volume) 
{
    int err;  
    int orig_volume = 0;  
    static snd_ctl_t *handle = NULL;  
    snd_ctl_elem_info_t *info;  
    snd_ctl_elem_id_t *id;  
    snd_ctl_elem_value_t *control;  
    unsigned int count;  
    snd_ctl_elem_type_t type;  

    snd_ctl_elem_info_alloca(&info);  
    snd_ctl_elem_id_alloca(&id);  
    snd_ctl_elem_value_alloca(&control);      
    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_name(id, "Mic Capture Volume");  
          
    if ((err = snd_ctl_open(&handle, "default", 0)) < 0) {  
        printf("open ctl device failed\n");       
        return -1;  
    }   
             
    snd_ctl_elem_info_set_id(info, id);  
    if ((err = snd_ctl_elem_info(handle, info)) < 0) {  
        printf("snd_ctl_elem_info failed\n");
        snd_ctl_close(handle);  
        handle = NULL;  
        return -1;  
    }  
    type = snd_ctl_elem_info_get_type(info);  
    count = snd_ctl_elem_info_get_count(info);  
    snd_ctl_elem_value_set_id(control, id);  
      
    if (!snd_ctl_elem_read(handle, control)) {  
        orig_volume = snd_ctl_elem_value_get_integer(control, 0);  
    }  
      
    if(volume != orig_volume) {  
        //snd_ctl_elem_value_set_integer(control, 0, static_cast<long>(volume));  
        snd_ctl_elem_value_set_integer(control, 0,volume);  
        snd_ctl_elem_value_set_integer(control, 1,volume);  
        if ((err = snd_ctl_elem_write(handle, control)) < 0) 
	{  
            printf("snd_ctl_elem_write failed\n");
            snd_ctl_close(handle);  
            handle = NULL;  
            return -1;  
        }  
    }   
    snd_ctl_close(handle);  
    handle = NULL;  
    return 1;  
}
int main (char argc, char *argv[])
{
	set_volume(10);
	while(1);
	//snd_mixer_open(&mixer, 0);
	snd_mixer_open(&mixer, 0);
	snd_mixer_attach(mixer, "default");
	snd_mixer_selem_register(mixer, NULL, NULL);
	snd_mixer_load(mixer);
	//找到Pcm对应的element,方法比较笨拙
	pcm_element = snd_mixer_first_elem(mixer);
	pcm_element = snd_mixer_elem_next(pcm_element);
	pcm_element = snd_mixer_elem_next(pcm_element);

	long int a, b;
long alsa_min_vol, alsa_max_vol;
///处理alsa1.0之前的bug，之后的可略去该部分代码
snd_mixer_selem_get_playback_volume(pcm_element,
SND_MIXER_SCHN_FRONT_LEFT, &a);
snd_mixer_selem_get_playback_volume(pcm_element,
SND_MIXER_SCHN_FRONT_RIGHT, &b);

snd_mixer_selem_get_playback_volume_range(pcm_element,
&alsa_min_vol,
&alsa_max_vol);
}
