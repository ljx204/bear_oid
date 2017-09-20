
#include <stdio.h>
#include <alsa/asoundlib.h>

int main(void)
{
    int unmute, chn;
    int al, ar;
    snd_mixer_t *mixer;
    snd_mixer_elem_t *master_element;
    
    snd_mixer_open(&mixer, 0);
    snd_mixer_attach(mixer, "default");
    snd_mixer_selem_register(mixer, NULL, NULL);
    snd_mixer_load(mixer);  /* 取得第一個 element，也就是 Master */
    master_element = snd_mixer_first_elem(mixer);  /* 設定音量的範圍 0 ~ 100 */  
    snd_mixer_selem_set_playback_volume_range(master_element, 0, 100);  /* 取得 Master 是否靜音 */  
    snd_mixer_selem_get_playback_switch(master_element, 0, &unmute);  
    if (unmute)      
      printf("Master is Unmute.\n");  
    else      
      printf("Master is Mute.\n");  /* 取得左右聲道的音量 */  
    snd_mixer_selem_get_playback_volume(master_element, SND_MIXER_SCHN_FRONT_LEFT, &al);  
    snd_mixer_selem_get_playback_volume(master_element, SND_MIXER_SCHN_FRONT_RIGHT, &ar);  /* 兩聲道相加除以二求平均音量 */  
    printf("Master volume is %d\n", (al + ar) >> 1);  /* 設定 Master 音量 */  
    snd_mixer_selem_set_playback_volume(master_element, SND_MIXER_SCHN_FRONT_LEFT, 99);  
    snd_mixer_selem_set_playback_volume(master_element, SND_MIXER_SCHN_FRONT_RIGHT, 99);  /* 將 Master 切換為靜音 */  
    for (chn=0;chn<=SND_MIXER_SCHN_LAST;chn++) {      
      snd_mixer_selem_set_playback_switch(master_element, chn, 0);  
    }  /* 將 Master 切換為非靜音 */  
    for (chn=0;chn<=SND_MIXER_SCHN_LAST;chn++) {      
      snd_mixer_selem_set_playback_switch(master_element, chn, 1);  
    }  return 0;
}

